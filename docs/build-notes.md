# Build notes

## 2026-09-16 macOS baseline

- Xcode: `Xcode 26.6` / `Build version 17F113` (output of `xcodebuild -version`), macOS arm64 (Apple Silicon)
- Command: `scripts/build_macos.sh Release Vial`
- Result: standalone, VST3 and AU built with `NO_AUTH=1`. Three `** BUILD SUCCEEDED **`.
  - `plugin/builds/osx/build/Release/Vial.vst3`
  - `plugin/builds/osx/build/Release/Vial.component`
  - `plugin/builds/osx/build/Release/libVial.a`
  - `standalone/builds/osx/build/Release/Vial.app`
- Fixes applied: four, all in `scripts/build_macos.sh` (xcodebuild settings). No source change was
  needed: `third_party/JUCE` and both `Vial.xcodeproj/project.pbxproj` files are untouched, and the
  firebase link fallback from the task brief's Step 3 was not needed (with `NO_AUTH=1` nothing
  references firebase and neither project lists a firebase framework in a Frameworks build phase).

### Environment prerequisite (machine setup, not a repo change)

`xcodebuild` refused to build anything on this machine:

```
[MT] DVTPlugInLoading: Failed to load code for plug-in com.apple.dt.IDESimulatorFoundation ...
Library not loaded: /Library/Developer/PrivateFrameworks/CoreSimulator.framework/Versions/A/CoreSimulator
xcodebuild failed to load a required plug-in. Ensure your system frameworks are up-to-date by
running 'xcodebuild -runFirstLaunch'
```

`/Library/Developer/PrivateFrameworks` did not exist (Xcode's first-launch components had never been
installed). Fixed once, machine-wide, with:

```bash
xcodebuild -runFirstLaunch      # "Install Succeeded"; creates /Library/Developer/PrivateFrameworks
```

This also explains why `xcodebuild -list` was failing on this machine; it works now.

### Fix 1 - code signing

Error:

```
Vial.xcodeproj: error: No signing certificate "Mac Development" found: No "Mac Development"
signing certificate matching team ID "EFXDM6K3KJ" with a private key was found.
(in target 'Vial - VST3' from project 'Vial')
```

Upstream signs with Vital Audio's Apple team. Build ad-hoc signed instead (still a valid signature,
which arm64 requires); the products are for local development only.

```diff
-FLAGS=(ARCHS=arm64 ONLY_ACTIVE_ARCH=YES GCC_TREAT_WARNINGS_AS_ERRORS=NO
-       GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1')
+FLAGS=(ARCHS=arm64 ONLY_ACTIVE_ARCH=YES GCC_TREAT_WARNINGS_AS_ERRORS=NO
+       GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1'
+       CODE_SIGN_IDENTITY=- CODE_SIGN_STYLE=Manual DEVELOPMENT_TEAM=
+       PROVISIONING_PROFILE_SPECIFIER=)
```

Result: `codesign -dv` on `Vial.app` reports `Signature=adhoc`, `flags=0x10002(adhoc,runtime)`.

### Fix 2 - build-system dependency cycle on "/"

Error:

```
error: Cycle inside a single target; building could produce unreliable results.
...
node: .../plugin/builds/osx/build/EagerLinkingTBDs/Release ->
command: P0:::CreateBuildDirectory .../plugin/builds/osx/build ->
CYCLE POINT ->
node: / ->
```

The plugin project sets `DEPLOYMENT_LOCATION = YES` with `DSTROOT = /` so that built plug-ins are
installed straight into `$(HOME)/Library/Audio/Plug-Ins/...`. The current Xcode build system turns
that into a `CreateBuildDirectory /` task and deadlocks against the project's own build directory.
Turning deployment off keeps the products in `build/$CONFIG`, which is where the plan expects them.

```diff
        PROVISIONING_PROFILE_SPECIFIER=
+       DEPLOYMENT_LOCATION=NO)
```

(The standalone project does not set `DEPLOYMENT_LOCATION`, so this is a no-op there.)

### Fix 3 - VST2 SDK headers missing

Error:

```
third_party/JUCE/modules/juce_audio_plugin_client/VST3/juce_VST3_Wrapper.cpp:69:11:
fatal error: 'pluginterfaces/vst2.x/vstfxstore.h' file not found
```

JUCE 6.0.5 defaults `JUCE_VST3_CAN_REPLACE_VST2` to 1, which pulls in the VST2 SDK. Only
`third_party/VST_SDK/VST3_SDK` is vendored (no VST2 SDK, and no VST2 build is shipped), so the
feature is switched off rather than patching JUCE:

```diff
-       GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1'
+       GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1 JUCE_VST3_CAN_REPLACE_VST2=0'
```

Behaviour difference: the VST3 no longer advertises itself as a replacement for a VST2 build of the
same plug-in. Nothing else changes. The Linux and test builds need the same define.

### Fix 4 - none needed in third_party/JUCE

JUCE 6.0.5 compiled against the macOS 26.5 SDK with warnings only (deprecated `_Master` CoreAudio
element names, deprecated `operator""` spacing, `MACOSX_DEPLOYMENT_TARGET` 10.12 below the supported
10.13 floor). No JUCE source file needed editing.

### Authentication is compiled out

`NO_AUTH=1` makes `#if NDEBUG && !NO_AUTH` false in `src/common/authentication.h`,
`src/interface/editor_sections/authentication_section.{h,cpp}` and `full_interface.cpp:213`, so the
`AuthenticationSection` is never constructed. Verified on the built binaries:

```
Vial.app/Contents/MacOS/Vial          authentication: 0   account.vital.audio: 0   undefined firebase symbols: 0
Vial.vst3/Contents/MacOS/Vial         authentication: 0   account.vital.audio: 0
Vial.component/Contents/MacOS/Vial    authentication: 0   account.vital.audio: 0
```

### Standalone launch check

Run headlessly (this session has no screen to look at), instead of `open ...Vial.app`:

```bash
standalone/builds/osx/build/Release/Vial.app/Contents/MacOS/Vial &   # pid 72193
sleep 8; pgrep -fl "Vial.app"                                        # still alive
pkill -f "Vial.app/Contents/MacOS/Vial"
```

Observed: the process started, stayed alive for ~10 s, wrote nothing to stdout/stderr, and exited
only when killed. Combined with the string checks above, no login / "work offline" prompt can be
shown. The on-screen keyboard and About panel were not clicked - that still needs a human with a
display.

## Task 3 - rename to Audial Synth (brief sed adaptations)

The rename seds in the task brief are recorded here where the tree's actual text differed and an
equivalent edit was made by hand.

- `PRODUCT_NAME = Vial\b` / `PRODUCT_NAME = Vital\b` never matched: BSD `sed` has no `\b`, and the
  pbxproj lines are quoted (`PRODUCT_NAME = "Vial";`). Replaced `PRODUCT_NAME = "Vial"` /
  `"Vital"` / `"VitalTests"` literally instead.
- The pbxproj files carry more shipped names than the brief's list: product paths (`Vial.vst`,
  `Vial.vst3`, `Vial.component`, `Vial.appex`, `Vial.app`, `Vital.app`), the shared static library
  (`libVial.a`, `-lVial`), `name = Vial;` / `name = Vital;`, `ORGANIZATIONNAME`, and the AU
  post-build `auval -v aumu Vita Tyte` on `Components/Vital.component`. All renamed, otherwise the
  Step 2 grep does not come back clean and the AU post-build step validates the wrong component.
- `s/Vial/Audial Synth/g` over the Info plists would have corrupted the AudioComponents block: the
  AU `subtype` must stay a four-character code and the `factoryFunction` must match
  `JucePlugin_AUExportPrefix`. Set explicitly instead: `manufacturer` `Open` -> `Audi`, `subtype`
  `Vial` -> `AuSy`, `factoryFunction` `vialFactory` -> `AudialSynthAUFactory` (and
  `vialFactoryAUv3` -> `AudialSynthAUFactoryAUv3`). `auval -v aumu AuSy Audi` fails without this.
- Bundle identifiers the brief's sed did not cover: `audio.vital.standalone` ->
  `ai.audialmusic.synth.standalone`, `audio.vital.tests` -> `ai.audialmusic.synth.tests`,
  `org.tytel.vital` (headless) -> `ai.audialmusic.synth.headless`.
- `JucePlugin_LV2URI` is not in `plugin/JuceLibraryCode/JucePluginDefines.h`; it is a `-D` flag in
  `plugin/builds/linux_lv2/Makefile.binary`. Set there to `https://audialmusic.ai/synth`.
- `standalone/builds/linux/Makefile` has `JUCE_TARGET_APP := vial`, not `vital`; `tests` has
  `JUCE_TARGET_CONSOLEAPP := vital_tests`. Both renamed (`audialsynth`, `audialsynth_tests`).
  `plugin/builds/linux_lv2/Makefile.binary` also carried `Vial.so` / `Vial.a` targets.
- The `.jucer` sed order matters: `pluginName="Vial"` contains `name="Vial"`, so the plugin-specific
  keys must be replaced before the bare `name=` ones. Also renamed in the jucers, beyond the brief:
  most `targetName=` attributes, `companyCopyright`, the `tytel.org` NSAppTransportSecurity exception
  domain (now `audialmusic.ai`, also in the generated osx plists), and the JACK/ALSA client-name
  defines. The original pass missed the lowercase `LINUX_MAKE` `<CONFIGURATION>` `targetName=`
  attributes in `headless/vital.jucer`, `standalone/vital.jucer` and `tests/vital.jucer`
  (`targetName="vital"`, `targetName="vial"`, `targetName="vital_tests"`); those were fixed in Task 3
  fix round 1 to `targetName="audialsynth"` / `targetName="audialsynth_tests"`, matching the
  `JUCE_TARGET_APP` / `JUCE_TARGET_CONSOLEAPP` values already used by the hand-maintained Linux
  Makefiles.
- Extra source strings the brief's list missed but its own grep flags:
  `download_section.cpp` install folder `"Vial"`, `full_interface.cpp` OpenGL warning `"Vial
  requires OpenGL version: "`, plus (found by a wider grep) `load_save.cpp`
  `kLinuxUserDataDirectory` / `XDG_DATA_HOME` child `vital`, the `"Vial Auth Init Thread"` name in
  `authentication_section.h`, and `handleVitalCrash` in `src/standalone/main.cpp`.
- The brief's Step 3 grep lists a root `*.jucer`; there is no `.jucer` at the repo root, so only
  `*/vital.jucer` was grepped.

### Deliberately left alone

- `.vital` / `.vitalbank` / `.vitaltable` / `.vitalskin` file extensions and the `clm ` wavetable
  marker: file-format identifiers, not brand names; renaming them breaks existing preset files.
- `icons/vital_*.svg`, `images/vital*.png|xpm` source file names. The Linux `make install` copies
  them out under `$(PROGRAM)` (now `audialsynth.png` / `audialsynth.xpm`), so nothing ships as
  "vital"; only the in-repo source names remain.
- `plugin|standalone|headless|tests/vital.jucer` file names (the brief says these stay).
- `plugin/builds/{vs17,vs19,iOS}` and `standalone|tests/builds/vs*`: Windows and iOS exporters that
  no task builds. They still carry `Vial`/`Vital` names and will need the same rename before any
  Windows or iOS build is shipped.
- `headless/builds/osx/Vital.entitlements`: orphaned, referenced by nothing.

### Task 3 verification

```
scripts/build_macos.sh Release AudialSynth   -> ** BUILD SUCCEEDED **, products:
  plugin/builds/osx/build/Release/{AudialSynth.component,AudialSynth.vst3,libAudialSynth.a}
  standalone/builds/osx/build/Release/AudialSynth.app
plutil VST3   CFBundleIdentifier ai.audialmusic.synth            CFBundleName "Audial Synth"
plutil AU     manufacturer Audi  subtype AuSy  factoryFunction AudialSynthAUFactory
plutil App    CFBundleIdentifier ai.audialmusic.synth.standalone CFBundleName "Audial Synth"
standalone launched headlessly, stayed alive past 8 s, killed
auval -v aumu AuSy Audi -> AU VALIDATION SUCCEEDED.
```

The header/about logo now draws the Audial wave mark (`Paths::vitalV()` parses the concatenated
`d` data of `txt2vox_gui/vst3/Resources/brand/wave.svg`); this was not looked at on screen.

## Task 4 - Linux VST3 in Docker (Ubuntu 22.04, headless load check)

- `plugin/JuceLibraryCode/AppConfig.h` now defines `JUCE_VST3_CAN_REPLACE_VST2 0` (it was left
  commented out, so JUCE 6.0.5 defaulted it to 1 and `juce_VST3_Wrapper.cpp` tried to include the
  VST2 SDK, which is not vendored - only `third_party/VST_SDK/VST3_SDK` exists). Setting it in
  `AppConfig.h` fixes it for every exporter instead of per build script; the equivalent
  `JUCE_VST3_CAN_REPLACE_VST2=0` flag stays in `scripts/build_macos.sh` as a harmless duplicate.

### Linux Makefile changes (Step 1)

`plugin/builds/linux_vst/Makefile` and `standalone/builds/linux/Makefile`: `-DREQUIRE_AUTH=1` ->
`-DNO_AUTH=1` (Debug and Release), and the firebase/libsecret include path, library path and
`-lfirebase_auth -lfirebase_app -lsecret-1 -lglib-2.0` link flags dropped.

```
grep -c "NO_AUTH=1"  -> plugin 2, standalone 2
grep -c "firebase"   -> plugin 0, standalone 0
```

`-I../../../third_party/VST_SDK/VST2_SDK` is left in place: the directory does not exist, and a
missing `-I` directory is not an error for gcc.

### Fix 5 - LTO link dies with "write jobserver: Bad file descriptor" (GNU make 4.3 + GCC 11)

Error from the first Docker build (`make vst3 CONFIG=Release -j"$(nproc)"`, Ubuntu 22.04):

```
make[2]: *** write jobserver: Bad file descriptor.  Stop.
lto-wrapper: fatal error: make returned 2 exit status
compilation terminated.
/usr/bin/ld: error: lto-wrapper failed
collect2: error: ld returned 1 exit status
make[1]: *** [Makefile:176: build/AudialSynth.vst3/Contents/x86_64-linux/AudialSynth.so] Error 1
```

The Release Linux configs link with `-flto`, so GCC's `lto-wrapper` runs its own `make` for the
LTRANS phase. GNU make 4.3 (jammy) only hands the jobserver file descriptors to recipes it considers
recursive; for every other recipe it still exports `--jobserver-auth=R,W` in `MAKEFLAGS` but closes
the descriptors, so `lto-wrapper`'s sub-make fails. The fix is the documented one: mark the final
link recipes recursive with a leading `+`.

```diff
-	$(V_AT)$(CXX) -o $(JUCE_OUTDIR)/$(JUCE_TARGET_VST3) ...
+	+$(V_AT)$(CXX) -o $(JUCE_OUTDIR)/$(JUCE_TARGET_VST3) ...
```

Applied to all four `$(CXX) -o` link recipes in the two Linux Makefiles (VST, VST3, Standalone
plug-in, and the standalone app) so a parallel `make` works for every target, not just `vst3`.
Nothing else changes for a real build: the same command line is run, with the jobserver descriptors
kept open. The one documented side effect of `+` is that `make -n` now actually runs those link
commands instead of only printing them (GNU make always executes `+` lines, even in dry-run mode);
no build path uses `make -n`.

### Fix 6 - verify stage runtime packages

`python /verify_vst3.py` failed twice before the plug-in was even reached:

```
ImportError: libatomic.so.1: cannot open shared object file: No such file or directory
xvfb-run: error: xauth command not found
```

`pedalboard`'s native extension needs `libatomic1`, and `xvfb-run` needs `xauth` (Debian's `xvfb`
package does not pull it in). Both added to the verify stage's `apt-get install` list in
`docker/Dockerfile.linux-vst3`.

### Fix 7 - out-of-bounds parameter-text lookup crashed every host that enumerates parameters

With the plug-in built, `load_plugin` segfaulted:

```
Fatal Python error: Segmentation fault
Current thread 0x00007ffffee86740 (most recent call first):
  File ".../pedalboard/_pedalboard.py", line 256 in get_text_for_raw_value
  File ".../pedalboard/_pedalboard.py", line 301 in __init__
  File ".../pedalboard/_pedalboard.py", line 669 in _get_parameters
  File ".../pedalboard/_pedalboard.py", line 656 in parameters
  File ".../pedalboard/_pedalboard.py", line 637 in __set_initial_parameter_values__
  File ".../pedalboard/_pedalboard.py", line 818 in load_plugin
```

Cause (upstream, not something Tasks 1-3 introduced). `ValueBridge::getText`
(`src/plugin/value_bridge.h`) answers a host's "what does this value read as?" with

```cpp
result = details_.string_lookup[std::max<int>(0, std::min(adjusted, details_.max))];
```

i.e. it assumes every `string_lookup` table has `max + 1` entries. pedalboard probes each parameter
at 1001 raw values from 0.0 to 1.0, so it asks for the text at `max` for all 2852 parameters, and
four parameters break that assumption - the read runs past the end of a `const std::string[]` and
dereferences garbage. The first one hit was `Filter 1 Style`. An audit of every indexed parameter
against its table found exactly four, all fixed at the data end so that no parameter range and no
existing display string changes:

| parameter | max | table | entries | fix |
| --- | --- | --- | --- | --- |
| `filter_*_style` | 9 | `kFilterStyleNames` | 5 | padded the table to 10 (`Style 6` … `Style 10`) |
| `osc_*_view_2d` | 2 | `kOffOnNames` | 2 | table changed to `kWavetableDimensionNames` (`3D`/`2D`/`SP`), which is what the control actually means |
| `view_spectrogram` | 2 | `kOffOnNames` | 2 | max 2 -> 1; it is a toggle (`HeaderSection` reads it through a `SynthButton`) |
| `destination`, `sample_destination` | `kNumSourceDestinations + kNumEffects` (14) | `kDestinationNames` | 14 (0-13) | max -> `... - 1`; off-by-one, index 14 named nothing and routed nowhere |

The style range stays 0-9 because it has to cover every filter model (comb uses style 5); the
per-model names the GUI shows (`FilterSection::getStyleName`) are unaffected, as they index the
model's own table. Nothing in `src/synthesis/` was touched. After the fix, sweeping all 2852
parameters x 1001 raw values through `get_text_for_raw_value` completes cleanly:

```
params: 2852
ALL 2852 PARAMS SWEPT OK
```

### Task 4 result

```
scripts/build_linux_vst3.sh
...
#18 [verify 6/6] RUN python /verify_vst3.py ... || xvfb-run -a python /verify_vst3.py ...
#18 2.454 rendered shape=(2, 66150) peak=0.3618
#18 2.454 OK
#13 exporting to client directory
#13 copying files 13.09MB done
AudialSynth.so
```

- `xvfb` was **not** needed: the first (no-display) `python /verify_vst3.py` succeeds, so the `||`
  fallback never runs. `xvfb` and `xauth` stay in the image as a safety net for other hosts.
- Exported bundle: `docker/out/AudialSynth.vst3/Contents/x86_64-linux/AudialSynth.so`,
  13,086,448 bytes, `ELF 64-bit LSB shared object, x86-64 ... not stripped`.
- `ldd` lists `libasound`, `libfreetype`, `libcurl`, `libGL`, `libstdc++`, `libm`, `libgcc_s`,
  `libc` - no `libsecret`, `libglib` or firebase. `grep -ac` on the binary: `authentication` 0,
  `account.vital.audio` 0, `firebase` 0, `libsecret` 0, `Audial Synth` 3.
- Emulated amd64 on this Apple Silicon Mac runs under Rosetta, so the whole clean pipeline (apt,
  compile, LTO link, pip, verify) takes about 5 minutes, not the 30-90 the brief expected.

### Candidate PLUGIN_COMMIT

`33720828f50324c7330cd2751e82cc83fabb21d8` (`build: Linux VST3 without auth libraries, Docker build
and headless load check`) is the first candidate `PLUGIN_COMMIT` for the microservice image
(`genetic_vital` plan Task 8).
