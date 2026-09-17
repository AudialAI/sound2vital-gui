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
