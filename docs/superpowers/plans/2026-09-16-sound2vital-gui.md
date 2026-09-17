# sound2vital-gui (Audial Synth) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fork Vital into "Audial Synth": a renamed GPLv3 synth with a Resynth section that uploads a dropped sample to the Audial API, waits for the `sound2vital` job, and loads the returned preset into the running instance; and produce the Linux VST3 that the microservice uses as its render backend.

**Architecture:** Upstream `mtytel/vital` 1.0.6 is the base. New code is three units: `AudialClient` (JUCE `URL` networking plus pure JSON helpers), credential persistence in `LoadSave`, and `ResynthSection` (an `Overlay` with a drop zone, a job thread and preset loading), wired through a header button. Auth, update-check and factory-download code is compiled out with `NO_AUTH=1`. Sources join the existing unity build files, so no Projucer regeneration is needed.

**Tech Stack:** C++17, JUCE 6.0.5 (vendored), nlohmann json (vendored), Projucer-generated Xcode projects and Makefiles, Docker (ubuntu:22.04) for the Linux VST3, Python 3.12 + pedalboard 0.9.22 for verification tooling.

**Spec:** `docs/superpowers/specs/2026-09-16-sound2vital-gui-design.md`

## Global Constraints

- GPLv3 stays; `LICENSE` is untouched and source is published with every binary.
- No shipped name, bundle id, window title, thread name, or logo may contain "Vital", "Vital Audio", "Tytel"; no request to any vital.audio host.
- DSP sources under `src/synthesis/` are never edited.
- Every new `.cpp` is added through a unity file: sections via `src/unity_build/interface_editor_sections.cpp`, common classes via `src/unity_build/common.cpp`.
- Network calls never run on the message thread; UI updates from threads go through `MessageManager::callAsync` with `Component::SafePointer`.
- Inputs longer than 20.0 s are refused client-side before upload.
- macOS builds pass `GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1' ARCHS=arm64 ONLY_ACTIVE_ARCH=YES GCC_TREAT_WARNINGS_AS_ERRORS=NO`. Linux Makefiles define `NO_AUTH=1` permanently.
- Working names: product "Audial Synth", bundle `AudialSynth`, section "Resynth". Renaming later is a find-and-replace over Task 3's list.
- Cross-repo: the microservice plan's Task 8 consumes this plan's Tasks 1, 3 and 4 (repo at the assumed URL, renamed, Linux VST3 building). `tools/parity_check.py` (Task 10) imports the engine from `../genetic_vital/src`.

---

### Task 1: Bootstrap the fork repository from upstream

**Files:**
- Create: `.gitignore` additions, `docs/README.md`
- Existing: `docs/superpowers/specs/…`, `docs/superpowers/plans/…` (already in the folder)

**Interfaces:**
- Produces: a git repo whose `main` contains upstream history plus `docs/`; remote `upstream` for later merges.

- [ ] **Step 1: Initialise and pull upstream into the existing folder**

```bash
cd "/Users/zachfarrell/Desktop/Coding_Projects/Audial-All Current Code/sound2vital-gui"
git init -b main
git add docs
git commit -m "docs: sound2vital-gui design and plan"
git remote add upstream https://github.com/mtytel/vital.git
git fetch upstream main
git merge --allow-unrelated-histories upstream/main -m "chore: import upstream vital"
```

Expected: `git log --oneline | tail -1` shows an upstream commit, `ls` shows `src third_party plugin standalone headless tests Makefile LICENSE docs`.

- [ ] **Step 2: Add build outputs to `.gitignore`**

Append to the upstream `.gitignore`:

```
plugin/builds/osx/build/
plugin/builds/linux_vst/build/
standalone/builds/osx/build/
standalone/builds/linux/build/
tests/builds/osx/build/
tests/builds/linux/build/
docker/out/
qualification/
*.xcuserdatad/
.DS_Store
```

- [ ] **Step 3: Write `docs/README.md`**

```markdown
# Audial Synth (sound2vital-gui)

GPLv3 fork of Vital 1.0.6 with the Resynth section. Design and plan:

- docs/superpowers/specs/2026-09-16-sound2vital-gui-design.md
- docs/superpowers/plans/2026-09-16-sound2vital-gui.md

Build notes accumulate in docs/build-notes.md; parity results in docs/parity.md.
```

- [ ] **Step 4: Verify and commit**

Run: `git status --short | head` and `git log --oneline | head -3`
Expected: only the new files are unstaged; history contains the upstream import.

```bash
git add .gitignore docs/README.md
git commit -m "chore: ignore build outputs, add docs index"
```

---

### Task 2: macOS baseline build without authentication

**Files:**
- Create: `docs/build-notes.md`, `scripts/build_macos.sh`
- Possibly modify: `plugin/builds/osx/Vial.xcodeproj/project.pbxproj` (only if the firebase frameworks fail to link on arm64; see Step 3)

**Interfaces:**
- Produces: `plugin/builds/osx/build/Release/Vial.app` (standalone) and `Vial.vst3`, built with `NO_AUTH=1`; the build command in `scripts/build_macos.sh` reused by every later task.

- [ ] **Step 1: Write `scripts/build_macos.sh`**

```bash
#!/usr/bin/env bash
# Build the VST3 and AU (plugin project) and the standalone app (standalone project)
# with authentication compiled out.
# Usage: scripts/build_macos.sh [Debug|Release] [target-prefix]   (defaults: Release, Vial)
set -euo pipefail
CONFIG="${1:-Release}"
PREFIX="${2:-Vial}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLAGS=(ARCHS=arm64 ONLY_ACTIVE_ARCH=YES GCC_TREAT_WARNINGS_AS_ERRORS=NO
       GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1')
for TARGET in "VST3" "AU"; do
  xcodebuild -project "$ROOT/plugin/builds/osx/$PREFIX.xcodeproj" \
    -target "$PREFIX - $TARGET" -configuration "$CONFIG" "${FLAGS[@]}" build
done
xcodebuild -project "$ROOT/standalone/builds/osx/$PREFIX.xcodeproj" \
  -target "$PREFIX - App" -configuration "$CONFIG" "${FLAGS[@]}" build
ls "$ROOT/plugin/builds/osx/build/$CONFIG/" "$ROOT/standalone/builds/osx/build/$CONFIG/"
```

`chmod +x scripts/build_macos.sh`. The `standalone/` project is Vital's real standalone application (computer keyboard, `--render` CLI); the plugin project's "Standalone Plugin" target is only JUCE's generic host shell and is not shipped.

- [ ] **Step 2: Build**

Run: `scripts/build_macos.sh Release Vial 2>&1 | tee /tmp/audial_build.log | tail -40`
Expected: `** BUILD SUCCEEDED **` three times and listings containing `Vial.vst3`, `Vial.component` (plugin build dir) and `Vial.app` (standalone build dir).

- [ ] **Step 3: If the link step fails on the firebase frameworks**

The symptom is `ld: ... firebase_auth.framework ... building for macOS-arm64 but attempting to link with file built for macOS-x86_64` or a missing-framework error. With `NO_AUTH=1` nothing references those symbols, so remove them from the link phase:

```bash
cp plugin/builds/osx/Vial.xcodeproj/project.pbxproj /tmp/project.pbxproj.bak
sed -i '' '/firebase.*\.framework in Frameworks/d' plugin/builds/osx/Vial.xcodeproj/project.pbxproj
sed -i '' '/firebase.*\.framework in Frameworks/d' standalone/builds/osx/Vial.xcodeproj/project.pbxproj
```

Then rebuild. Any other compiler error (JUCE 6.0.5 against the current Xcode) is fixed with the smallest change in `third_party/JUCE` and written to `docs/build-notes.md` with the error text and the diff, so the Linux and test builds can apply the same fix.

- [ ] **Step 4: Run the standalone and check behaviour**

Run: `open standalone/builds/osx/build/Release/Vial.app`
Expected: the synth opens with no login or "work offline" prompt, plays from the on-screen keyboard, and the About panel opens. Quit it.

- [ ] **Step 5: Record and commit**

`docs/build-notes.md`:

```markdown
# Build notes

## 2026-09-16 macOS baseline
- Xcode: <output of `xcodebuild -version`>
- Command: scripts/build_macos.sh Release Vial
- Result: standalone, VST3, AU built with NO_AUTH=1.
- Fixes applied: <none | list>
```

```bash
git add scripts/build_macos.sh docs/build-notes.md plugin/builds/osx/Vial.xcodeproj/project.pbxproj standalone/builds/osx/Vial.xcodeproj/project.pbxproj third_party
git commit -m "build: macOS baseline with authentication compiled out"
```

---

### Task 3: Rename to Audial Synth

**Files:**
- Modify: `plugin/JuceLibraryCode/JucePluginDefines.h`, `plugin/JuceLibraryCode/JuceHeader.h`, `standalone/JuceLibraryCode/JuceHeader.h`, `headless/JuceLibraryCode/JuceHeader.h`, `tests/JuceLibraryCode/JuceHeader.h`
- Modify: `plugin/builds/osx/Vial.xcodeproj/project.pbxproj`, `standalone/builds/osx/Vial.xcodeproj/project.pbxproj`, `plugin/builds/osx/Info-*.plist`, `standalone/builds/osx/Info-App.plist`
- Modify: `plugin/builds/linux_vst/Makefile`, `plugin/builds/linux_lv2/Makefile`, `standalone/builds/linux/Makefile`, `headless/builds/linux/Makefile`, `tests/builds/linux/Makefile`, root `Makefile`, `standalone/vital.desktop`
- Modify: `plugin/vital.jucer`, `standalone/vital.jucer`, `headless/vital.jucer`, `tests/vital.jucer`
- Modify: `src/common/load_save.cpp:1132,1168,1187,1752,1755`, `src/interface/editor_sections/update_check_section.cpp:38`, thread names in `download_section.h`, `update_check_section.h`, `authentication_section.h`, `full_interface.cpp`
- Modify: `src/interface/look_and_feel/paths.h:33-55` (logo paths)
- Rename: `plugin/builds/osx/Vial.xcodeproj` → `AudialSynth.xcodeproj`, same for standalone, headless, tests

**Interfaces:**
- Produces: product "Audial Synth", `JucePlugin_ManufacturerCode 0x41756469` ('Audi'), `JucePlugin_PluginCode 0x41755379` ('AuSy'), bundle id `ai.audialmusic.synth`, Linux artifacts `AudialSynth.vst3/Contents/x86_64-linux/AudialSynth.so`, Xcode targets `AudialSynth - …`, config file name and user folder `AudialSynth`.

- [ ] **Step 1: Plugin identity**

Edit `plugin/JuceLibraryCode/JucePluginDefines.h`: set

```c
 #define JucePlugin_Name                   "Audial Synth"
 #define JucePlugin_Desc                   "Audial Synth"
 #define JucePlugin_Manufacturer           "Audial"
 #define JucePlugin_ManufacturerCode       0x41756469
 #define JucePlugin_PluginCode             0x41755379
```

and every `JucePlugin_*` line whose value contains `Vial`, `vial` or `Vital` (`grep -n "Vial\|vial\|Vital" plugin/JuceLibraryCode/JucePluginDefines.h`) to the Audial equivalent: `JucePlugin_ManufacturerWebsite "https://audialmusic.ai"`, `JucePlugin_ManufacturerEmail "support@audialmusic.ai"`, `JucePlugin_CFBundleIdentifier "ai.audialmusic.synth"`, `JucePlugin_AUExportPrefix AudialSynthAU`, `JucePlugin_AUExportPrefixQuoted "AudialSynthAU"`, `JucePlugin_LV2URI "https://audialmusic.ai/synth"`.

In each `JuceLibraryCode/JuceHeader.h` (plugin, standalone, headless, tests):

```c
    const char* const  projectName    = "AudialSynth";
    const char* const  companyName    = "Audial";
```

- [ ] **Step 2: Xcode projects and plists**

```bash
for P in plugin standalone; do
  git mv "$P/builds/osx/Vial.xcodeproj" "$P/builds/osx/AudialSynth.xcodeproj"
done
git mv headless/builds/osx/Vital.xcodeproj headless/builds/osx/AudialSynth.xcodeproj
git mv tests/builds/osx/VitalTests.xcodeproj tests/builds/osx/AudialSynthTests.xcodeproj
sed -i '' 's/Vial - /AudialSynth - /g; s/Vital - /AudialSynth - /g; s/productName = Vial;/productName = AudialSynth;/g; s/productName = Vital;/productName = AudialSynth;/g; s/PRODUCT_NAME = Vial\b/PRODUCT_NAME = AudialSynth/g; s/PRODUCT_NAME = Vital\b/PRODUCT_NAME = AudialSynth/g; s/audio\.vial\.synth/ai.audialmusic.synth/g; s/audio\.vital\.synth/ai.audialmusic.synth/g; s/VitalTests/AudialSynthTests/g' \
  plugin/builds/osx/AudialSynth.xcodeproj/project.pbxproj \
  standalone/builds/osx/AudialSynth.xcodeproj/project.pbxproj \
  headless/builds/osx/AudialSynth.xcodeproj/project.pbxproj \
  tests/builds/osx/AudialSynthTests.xcodeproj/project.pbxproj
sed -i '' 's/Vial/Audial Synth/g; s/audio\.vial\.synth/ai.audialmusic.synth/g; s/Vial Audio/Audial/g' plugin/builds/osx/Info-*.plist standalone/builds/osx/Info-App.plist
grep -rn "Vial\|vital\.audio\|Vital Audio" plugin/builds/osx standalone/builds/osx tests/builds/osx | grep -v "\.nib" || echo "clean"
```

Expected: the final grep prints `clean`. Update `scripts/build_macos.sh` default prefix from `Vial` to `AudialSynth`.

- [ ] **Step 3: Linux Makefiles, root Makefile, desktop file, jucer files**

```bash
sed -i '' 's/Vial\.vst3/AudialSynth.vst3/g; s/Vial\.so/AudialSynth.so/g; s/JUCE_TARGET_VST := Vial/JUCE_TARGET_VST := AudialSynth/; s/JUCE_TARGET_APP := vital/JUCE_TARGET_APP := audialsynth/; s/"Vital"/"AudialSynth"/g' \
  plugin/builds/linux_vst/Makefile plugin/builds/linux_lv2/Makefile standalone/builds/linux/Makefile headless/builds/linux/Makefile tests/builds/linux/Makefile
sed -i '' 's/^PROGRAM = vital/PROGRAM = audialsynth/; s/^LIB_PROGRAM = Vital$/LIB_PROGRAM = AudialSynth/; s/^LIB_PROGRAM_FX = VitalFX/LIB_PROGRAM_FX = AudialSynthFX/; s#Vital\.lv2#AudialSynth.lv2#g; s#build/Vital\.so#build/AudialSynth.so#g; s#build/Vital\.vst3#build/AudialSynth.vst3#g' Makefile
git mv standalone/vital.desktop standalone/audialsynth.desktop
sed -i '' 's/Vital/Audial Synth/g; s/vital/audialsynth/g' standalone/audialsynth.desktop
sed -i '' 's/install -m644 standalone\/vital\.desktop \$(DESKTOP)\/vital\.desktop/install -m644 standalone\/audialsynth.desktop $(DESKTOP)\/audialsynth.desktop/' Makefile
sed -i '' 's/name="Vial"/name="AudialSynth"/g; s/pluginName="Vial"/pluginName="Audial Synth"/g; s/pluginManufacturer="Vial Audio"/pluginManufacturer="Audial"/g; s/pluginManufacturerCode="Open"/pluginManufacturerCode="Audi"/g; s/pluginCode="Vial"/pluginCode="AuSy"/g; s/bundleIdentifier="audio\.vial\.synth"/bundleIdentifier="ai.audialmusic.synth"/g; s/companyName="Vial Audio"/companyName="Audial"/g' plugin/vital.jucer standalone/vital.jucer headless/vital.jucer tests/vital.jucer
grep -n "Vial\|\"Vital\"" plugin/builds/linux_vst/Makefile standalone/builds/linux/Makefile Makefile *.jucer */vital.jucer || echo "clean"
```

Expected: the final grep prints `clean` (the `.jucer` file names themselves stay; they are not shipped).

- [ ] **Step 4: Source strings**

`src/common/load_save.cpp`: replace `config_options.applicationName = "Vial";` (three occurrences) with `"AudialSynth"`, and the two data-directory lines:

```cpp
  File directory = home_directory.getChildFile("Music").getChildFile("Audial Synth");
```

```cpp
  File directory = documents_dir.getChildFile("Audial Synth");
```

`src/interface/editor_sections/update_check_section.cpp:38`: text becomes `"There is a new version of Audial Synth!"`.

Thread names: `sed -i '' 's/"Vial Download Thread"/"Audial Download Thread"/; s/"Vial Install Thread"/"Audial Install Thread"/' src/interface/editor_sections/download_section.h src/interface/editor_sections/update_check_section.h src/interface/editor_sections/authentication_section.h`.

Then:

```bash
grep -rn "\"Vial\|Vial \|Vital!\|vital\.audio\|Vital Audio\|Tytel" src --include='*.cpp' --include='*.h' | grep -v "^src/.*: \* \|Copyright\|GNU\|clm " || echo "clean"
```

Expected: `clean`. (Licence headers and the wavetable `clm ` metadata string, which is a file-format marker, are excluded on purpose.)

- [ ] **Step 5: Replace the logo**

In `src/interface/look_and_feel/paths.h` replace the bodies of `vitalV()`, `vitalRing()`, `vitalWord()` and `vitalWordRing()`. Keep the function names (call sites stay) but return Audial shapes. Use the wave mark from `../txt2vox_gui/vst3/Resources/brand/wave.svg`: copy the `d="…"` attribute of its `<path>` into a string constant.

```cpp
    static Path vitalV() {
      static const char* kAudialWave = "<paste the d attribute of wave.svg here>";
      Path path = Drawable::parseSVGPath(kAudialWave);
      path.applyTransform(path.getTransformToScaleToFit(0.0f, 0.0f, 1.0f, 1.0f, true));
      return path;
    }

    static Path vitalRing() {
      Path path;
      path.addEllipse(0.0f, 0.0f, 1.0f, 1.0f);
      path.addEllipse(0.12f, 0.12f, 0.76f, 0.76f);
      path.setUsingNonZeroWinding(false);
      return path;
    }

    static Path vitalWord() { return vitalV(); }
    static Path vitalWordRing() { return vitalRing(); }
```

If `wave.svg` contains more than one path, concatenate their `d` attributes with a space.

- [ ] **Step 6: Build and verify**

Run: `scripts/build_macos.sh Release AudialSynth 2>&1 | tail -5 && open standalone/builds/osx/build/Release/AudialSynth.app`
Expected: build succeeds; the window title and header logo show the Audial mark; `plutil -p plugin/builds/osx/build/Release/AudialSynth.vst3/Contents/Info.plist | grep -i "bundleidentifier\|bundlename"` shows `ai.audialmusic.synth` and `Audial Synth`.

Run: `auval -v aumu AuSy Audi 2>&1 | tail -3` (after copying `AudialSynth.component` to `~/Library/Audio/Plug-Ins/Components/`)
Expected: `AU VALIDATION SUCCEEDED`.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "brand: rename to Audial Synth, replace logo, Audial plugin identity"
```

---

### Task 4: Linux VST3 build in Docker with headless load check

**Files:**
- Modify: `plugin/builds/linux_vst/Makefile`, `standalone/builds/linux/Makefile` (auth define and link flags)
- Create: `docker/Dockerfile.linux-vst3`, `docker/verify_vst3.py`, `scripts/build_linux_vst3.sh`

**Interfaces:**
- Produces: `make vst3 CONFIG=Release` on Ubuntu 22.04 yields `plugin/builds/linux_vst/build/AudialSynth.vst3`; the microservice's `builder/build_plugin.sh` relies on exactly this target and path.

- [ ] **Step 1: Switch the Linux Makefiles to `NO_AUTH` and drop the auth libraries**

```bash
sed -i '' 's/"-DREQUIRE_AUTH=1"/"-DNO_AUTH=1"/g; s/ -lfirebase_auth -lfirebase_app -lsecret-1 -lglib-2\.0//g; s# -L\.\./\.\./\.\./third_party/firebase_cpp_sdk/libs/linux/x86_64/##g; s# -I\.\./\.\./\.\./third_party/firebase_cpp_sdk/include##g' \
  plugin/builds/linux_vst/Makefile standalone/builds/linux/Makefile
grep -c "NO_AUTH=1" plugin/builds/linux_vst/Makefile standalone/builds/linux/Makefile
grep -c "firebase" plugin/builds/linux_vst/Makefile standalone/builds/linux/Makefile
```

Expected: `2` for each file in the first grep (Debug and Release), `0` in the second.

- [ ] **Step 2: Write `docker/verify_vst3.py`**

```python
"""Load the Linux VST3 headlessly through pedalboard and render one note."""
import sys

import numpy as np
from mido import Message
from pedalboard import load_plugin

bundle = sys.argv[1]
plugin = load_plugin(bundle)
assert plugin.is_instrument, "not an instrument"
audio = plugin([Message("note_on", note=60, velocity=100, time=0.0),
                Message("note_off", note=60, time=1.0)], duration=1.5, sample_rate=44100)
peak = float(np.abs(audio).max())
print(f"rendered shape={audio.shape} peak={peak:.4f}")
assert audio.shape[0] == 2 and audio.shape[1] >= 44100, "unexpected render shape"
assert peak > 1e-4, "silent render"
print("OK")
```

- [ ] **Step 3: Write `docker/Dockerfile.linux-vst3`**

```dockerfile
# syntax=docker/dockerfile:1
FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential git pkg-config ca-certificates \
      libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev libxinerama-dev \
      libxcursor-dev libxcomposite-dev libgl1-mesa-dev mesa-common-dev \
      libcurl4-openssl-dev libjack-jackd2-dev && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . /src
RUN make vst3 CONFIG=Release -j"$(nproc)" && \
    test -f plugin/builds/linux_vst/build/AudialSynth.vst3/Contents/x86_64-linux/AudialSynth.so

FROM python:3.12-slim AS verify
RUN apt-get update && apt-get install -y --no-install-recommends \
      libgl1 libfreetype6 libasound2 libcurl4 libx11-6 libxext6 libxinerama1 \
      libxrandr2 libxcursor1 libxcomposite1 libfontconfig1 xvfb && \
    rm -rf /var/lib/apt/lists/*
RUN pip install --no-cache-dir pedalboard==0.9.22 numpy==2.4.4 mido==1.3.3
COPY --from=build /src/plugin/builds/linux_vst/build/AudialSynth.vst3 /opt/audial/AudialSynth.vst3
COPY docker/verify_vst3.py /verify_vst3.py
RUN python /verify_vst3.py /opt/audial/AudialSynth.vst3 || xvfb-run -a python /verify_vst3.py /opt/audial/AudialSynth.vst3

FROM scratch AS artifact
COPY --from=build /src/plugin/builds/linux_vst/build/AudialSynth.vst3 /AudialSynth.vst3
```

- [ ] **Step 4: Write `scripts/build_linux_vst3.sh`**

```bash
#!/usr/bin/env bash
# Build and verify the Linux VST3 in Docker; export the bundle to docker/out/.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
docker build --platform linux/amd64 -f "$ROOT/docker/Dockerfile.linux-vst3" --target verify -t audialsynth-vst3-verify "$ROOT"
docker build --platform linux/amd64 -f "$ROOT/docker/Dockerfile.linux-vst3" --target artifact -o "$ROOT/docker/out" "$ROOT"
ls "$ROOT/docker/out/AudialSynth.vst3/Contents/x86_64-linux/"
```

`chmod +x scripts/build_linux_vst3.sh`.

- [ ] **Step 5: Run it**

Run: `scripts/build_linux_vst3.sh 2>&1 | tee /tmp/linux_vst3.log | tail -30`
Expected: the `verify` stage prints `rendered shape=(2, 66150) peak=…` and `OK`; `docker/out/AudialSynth.vst3/Contents/x86_64-linux/AudialSynth.so` exists. On this arm64 Mac the build is emulated and takes 30 to 60 minutes the first time.

If the `verify` stage fails inside `load_plugin` with an X11 or OpenGL message even under `xvfb-run`, record the exact text in `docs/build-notes.md` and stop: that is a go/no-go finding for the whole backend plan.

- [ ] **Step 6: Commit and record the backend commit**

```bash
git add plugin/builds/linux_vst/Makefile standalone/builds/linux/Makefile docker scripts/build_linux_vst3.sh
git commit -m "build: Linux VST3 without auth libraries, Docker build and headless load check"
git rev-parse HEAD
```

The printed sha is the first candidate `PLUGIN_COMMIT` for the microservice image (`genetic_vital` plan Task 8). Write it into `docs/build-notes.md`.

---

### Task 5: Accept newer-version presets with a warning

**Files:**
- Modify: `src/common/load_save.cpp:1028-1036` (`LoadSave::jsonToState`)
- Test: manual render through the standalone CLI

**Interfaces:**
- Produces: `jsonToState` returns `true` for presets whose `synth_version` is newer, after writing a line to the error log; unknown controls are ignored as before.

- [ ] **Step 1: Edit `jsonToState`**

Replace

```cpp
  int compare_feature_versions = compareFeatureVersionStrings(version, ProjectInfo::versionString);
  if (compare_feature_versions > 0)
    return false;
```

with

```cpp
  int compare_feature_versions = compareFeatureVersionStrings(version, ProjectInfo::versionString);
  if (compare_feature_versions > 0) {
    // Audial Synth: presets from the sound2vital engine may carry a newer version
    // string. loadControls() ignores controls this build lacks, so load anyway.
    writeErrorLog("Loading preset saved by newer synth version " + version +
                  " into " + ProjectInfo::versionString + "; unknown controls ignored");
  }
```

- [ ] **Step 2: Build and render a 1.6.4 preset through the standalone CLI**

```bash
scripts/build_macos.sh Release AudialSynth 2>&1 | tail -3
PRESET="$(ls /Users/zachfarrell/Desktop/Coding_Projects/genetic/queries/results/engine_v3/editable_corpus_parallel_v8/run/native/*/work/preset.vital | head -1)"
python3 -c "import json,sys; print('synth_version', json.load(open(sys.argv[1]))['synth_version'])" "$PRESET"
cd /tmp && "/Users/zachfarrell/Desktop/Coding_Projects/Audial-All Current Code/sound2vital-gui/standalone/builds/osx/build/Release/AudialSynth.app/Contents/MacOS/AudialSynth" --render -m 60 -l 2 "$PRESET"
ls -la /tmp/preset.wav && python3 -c "
import wave, struct, sys
w = wave.open('/tmp/preset.wav'); frames = w.readframes(w.getnframes()); w.close()
samples = struct.unpack('<%dh' % (len(frames)//2), frames)
print('peak', max(abs(s) for s in samples))"
```

Expected: `synth_version 1.6.4`; a `preset.wav` is written into `/tmp` and its peak is above 0. Before this change the CLI rendered silence for such a preset because the load was refused.

- [ ] **Step 3: Commit**

```bash
git add src/common/load_save.cpp
git commit -m "feat(presets): load presets from newer synth versions with a logged warning"
```

---

### Task 6: `AudialClient` pure helpers with unit tests

**Files:**
- Create: `src/common/audial_client.h`, `src/common/audial_client.cpp`, `tests/common/audial_client_test.cpp`
- Modify: `src/unity_build/common.cpp` (append `#include "audial_client.cpp"`), `tests/interface_tests.cpp` (append `#include "common/audial_client_test.cpp"`)
- Create: `scripts/run_tests_macos.sh`

**Interfaces:**
- Produces:

```cpp
struct AudialCredentials { juce::String base_url, user_id, api_key; bool complete() const; };
struct HttpResult { int status = 0; juce::String body; bool ok() const; juce::String describe() const; };
class AudialClient {
 public:
  struct ExecutionStatus { juce::String state, preset_url, error; };
  explicit AudialClient(AudialCredentials credentials);
  static juce::String sanitizeFilename(const juce::String& name);
  static juce::String buildRunBody(const juce::String& user_id, const juce::String& filename, const juce::String& file_url);
  static juce::String parseUrl(const juce::String& body);
  static ExecutionStatus parseExecution(const juce::String& body);
  HttpResult uploadReference(const juce::File& file, const juce::String& exe_id, const juce::String& filename);
  HttpResult runSound2Vital(const juce::String& filename, const juce::String& file_url);
  HttpResult getExecution(const juce::String& exe_id);
  bool downloadToFile(const juce::String& url, const juce::File& destination);
};
```

- [ ] **Step 1: Write the failing unit test**

`tests/common/audial_client_test.cpp`:

```cpp
#include "audial_client.h"

class AudialClientTest : public UnitTest {
  public:
    AudialClientTest() : UnitTest("Audial Client") { }

    void runTest() override {
      beginTest("sanitize filename keeps [A-Za-z0-9_.-]");
      expectEquals(AudialClient::sanitizeFilename("My Kick [2026].wav"), String("My_Kick__2026_.wav"));
      expectEquals(AudialClient::sanitizeFilename(".hidden"), String("audio.hidden"));
      expectEquals(AudialClient::sanitizeFilename(""), String("audio"));

      beginTest("run body");
      expectEquals(AudialClient::buildRunBody("u1", "kick.wav", "https://cdn/k"),
                   String("{\"original\":{\"filename\":\"kick.wav\",\"url\":\"https://cdn/k\"},\"userId\":\"u1\"}"));

      beginTest("parse upload url");
      expectEquals(AudialClient::parseUrl("{\"url\":\"https://cdn/x\"}"), String("https://cdn/x"));
      expectEquals(AudialClient::parseUrl("not json"), String());
      expectEquals(AudialClient::parseUrl("{\"other\":1}"), String());

      beginTest("parse execution");
      AudialClient::ExecutionStatus done = AudialClient::parseExecution(
          "{\"state\":\"completed\",\"preset\":{\"presetvital\":{\"filename\":\"preset.vital\",\"url\":\"https://cdn/p\"}}}");
      expectEquals(done.state, String("completed"));
      expectEquals(done.preset_url, String("https://cdn/p"));
      expect(done.error.isEmpty());

      AudialClient::ExecutionStatus failed = AudialClient::parseExecution("{\"state\":\"failed\",\"error\":\"Input is 25.0 s\"}");
      expectEquals(failed.state, String("failed"));
      expectEquals(failed.error, String("Input is 25.0 s"));

      AudialClient::ExecutionStatus processing = AudialClient::parseExecution("{\"state\":\"processing\"}");
      expectEquals(processing.state, String("processing"));
      expect(processing.preset_url.isEmpty());

      AudialClient::ExecutionStatus garbage = AudialClient::parseExecution("<html>");
      expectEquals(garbage.state, String());
      expect(garbage.error.isNotEmpty());

      beginTest("credentials completeness");
      AudialCredentials creds { "https://api.audialmusic.ai", "u1", "k" };
      expect(creds.complete());
      creds.api_key = "";
      expect(!creds.complete());
    }
};

static AudialClientTest audial_client_test;
```

Append to `tests/interface_tests.cpp`:

```cpp
#include "common/audial_client_test.cpp"
```

- [ ] **Step 2: Write `scripts/run_tests_macos.sh` and run it to see the failure**

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
xcodebuild -project "$ROOT/tests/builds/osx/AudialSynthTests.xcodeproj" -target "AudialSynthTests - ConsoleApp" \
  -configuration Release ARCHS=arm64 ONLY_ACTIVE_ARCH=YES GCC_TREAT_WARNINGS_AS_ERRORS=NO \
  GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1' build | tail -3
"$ROOT/tests/builds/osx/build/Release/AudialSynthTests" 2>&1 | tee /tmp/audial_tests.log | grep -i "audial client\|failed\|passed" | head -20
```

`chmod +x scripts/run_tests_macos.sh`. Run: `scripts/run_tests_macos.sh`
Expected: compile error, `audial_client.h` not found. (If the tests project itself does not build for reasons unrelated to the new file, apply the fixes recorded in `docs/build-notes.md` from Task 2 and note anything new.)

- [ ] **Step 3: Write `src/common/audial_client.h`**

```cpp
/* Audial Synth: HTTP client for the Audial API (sound2vital function).
 * GPLv3, same terms as the rest of this source. */
#pragma once

#include "JuceHeader.h"

struct AudialCredentials {
  String base_url;
  String user_id;
  String api_key;

  bool complete() const {
    return base_url.isNotEmpty() && user_id.isNotEmpty() && api_key.isNotEmpty();
  }
};

struct HttpResult {
  int status = 0;
  String body;

  bool ok() const { return status >= 200 && status < 300; }

  String describe() const {
    if (status == 0)
      return "no connection (check base URL / network)";
    return "HTTP " + String(status) + ": " + (body.isEmpty() ? String("<empty>") : body.substring(0, 160));
  }
};

class AudialClient {
  public:
    struct ExecutionStatus {
      String state;
      String preset_url;
      String error;
    };

    static constexpr int kTimeoutMs = 60000;

    explicit AudialClient(AudialCredentials credentials) : credentials_(std::move(credentials)) { }

    static String sanitizeFilename(const String& name);
    static String buildRunBody(const String& user_id, const String& filename, const String& file_url);
    static String parseUrl(const String& body);
    static ExecutionStatus parseExecution(const String& body);

    HttpResult uploadReference(const File& file, const String& exe_id, const String& filename);
    HttpResult runSound2Vital(const String& filename, const String& file_url);
    HttpResult getExecution(const String& exe_id);
    bool downloadToFile(const String& url, const File& destination);

  private:
    HttpResult request(URL url, bool post_like, const String& method, const String& extra_headers);
    String authHeaders() const;

    AudialCredentials credentials_;
};
```

- [ ] **Step 4: Write `src/common/audial_client.cpp`**

```cpp
#include "audial_client.h"

#include "json/json.h"

using json = nlohmann::json;

String AudialClient::sanitizeFilename(const String& name) {
  String result;
  for (int i = 0; i < name.length(); ++i) {
    juce_wchar c = name[i];
    bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                c == '_' || c == '.' || c == '-';
    result += keep ? String::charToString(c) : String("_");
  }
  if (result.isEmpty() || result.startsWithChar('.'))
    result = "audio" + result;
  return result;
}

String AudialClient::buildRunBody(const String& user_id, const String& filename, const String& file_url) {
  json body;
  body["userId"] = user_id.toStdString();
  body["original"]["filename"] = filename.toStdString();
  body["original"]["url"] = file_url.toStdString();
  return String(body.dump());
}

String AudialClient::parseUrl(const String& body) {
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object() || !parsed.count("url") || !parsed["url"].is_string())
    return "";
  return String(parsed["url"].get<std::string>());
}

AudialClient::ExecutionStatus AudialClient::parseExecution(const String& body) {
  ExecutionStatus status;
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) {
    status.error = "Unreadable execution response";
    return status;
  }
  if (parsed.count("state") && parsed["state"].is_string())
    status.state = String(parsed["state"].get<std::string>());
  if (parsed.count("error") && parsed["error"].is_string())
    status.error = String(parsed["error"].get<std::string>());
  if (parsed.count("preset") && parsed["preset"].is_object()) {
    for (auto& entry : parsed["preset"].items()) {
      if (entry.value().is_object() && entry.value().count("url") && entry.value()["url"].is_string()) {
        status.preset_url = String(entry.value()["url"].get<std::string>());
        break;
      }
    }
  }
  if (status.state == "failed" && status.error.isEmpty())
    status.error = "sound2vital failed";
  return status;
}

String AudialClient::authHeaders() const {
  return "x-api-key: " + credentials_.api_key + "\r\nx-user-id: " + credentials_.user_id + "\r\n";
}

HttpResult AudialClient::request(URL url, bool post_like, const String& method, const String& extra_headers) {
  HttpResult result;
  std::unique_ptr<InputStream> stream = url.createInputStream(post_like, nullptr, nullptr, extra_headers,
                                                              kTimeoutMs, nullptr, &result.status, 5, method);
  if (stream != nullptr)
    result.body = stream->readEntireStreamAsString();
  return result;
}

HttpResult AudialClient::uploadReference(const File& file, const String& exe_id, const String& filename) {
  String path = credentials_.base_url + "/api/files/" + URL::addEscapeChars(credentials_.user_id, false) +
                "/execution/" + URL::addEscapeChars(exe_id, false) + "/reference/" +
                URL::addEscapeChars(filename, false);
  URL url = URL(path).withFileToUpload("file", file, "application/octet-stream");
  return request(url, true, "PUT", authHeaders());
}

HttpResult AudialClient::runSound2Vital(const String& filename, const String& file_url) {
  URL url = URL(credentials_.base_url + "/api/functions/run/sound2vital")
                .withPOSTData(buildRunBody(credentials_.user_id, filename, file_url));
  return request(url, true, "POST", authHeaders() + "Content-Type: application/json\r\n");
}

HttpResult AudialClient::getExecution(const String& exe_id) {
  URL url(credentials_.base_url + "/api/db/" + URL::addEscapeChars(credentials_.user_id, false) +
          "/execution/" + URL::addEscapeChars(exe_id, false));
  return request(url, false, "GET", authHeaders());
}

bool AudialClient::downloadToFile(const String& url, const File& destination) {
  int status = 0;
  std::unique_ptr<InputStream> stream = URL(url).createInputStream(false, nullptr, nullptr, "", kTimeoutMs,
                                                                   nullptr, &status, 5, "GET");
  if (stream == nullptr || status < 200 || status >= 300)
    return false;
  destination.getParentDirectory().createDirectory();
  destination.deleteFile();
  FileOutputStream output(destination);
  if (!output.openedOk())
    return false;
  output.writeFromInputStream(*stream, -1);
  output.flush();
  return destination.existsAsFile() && destination.getSize() > 0;
}
```

Append `#include "audial_client.cpp"` to `src/unity_build/common.cpp`.

- [ ] **Step 5: Run the tests**

Run: `scripts/run_tests_macos.sh`
Expected: the log shows the "Audial Client" test with all expectations passed and no failures in the summary.

- [ ] **Step 6: Commit**

```bash
git add src/common/audial_client.h src/common/audial_client.cpp src/unity_build/common.cpp tests/common/audial_client_test.cpp tests/interface_tests.cpp scripts/run_tests_macos.sh
git commit -m "feat(client): AudialClient with unit-tested JSON helpers"
```

---

### Task 7: Credentials persisted through `LoadSave`

**Files:**
- Modify: `src/common/load_save.h` (declarations near `saveWorkOffline`), `src/common/load_save.cpp` (after `saveWorkOffline`)
- Create: `tests/common/load_save_credentials_test.cpp`; append its include to `tests/interface_tests.cpp`

**Interfaces:**
- Produces: `static void LoadSave::saveAudialCredentials(const std::string& base_url, const std::string& user_id, const std::string& api_key);` and `static AudialCredentials LoadSave::loadAudialCredentials();` (default base URL `https://api.audialmusic.ai` when unset).

- [ ] **Step 1: Write the failing test**

`tests/common/load_save_credentials_test.cpp`:

```cpp
#include "load_save.h"

class LoadSaveCredentialsTest : public UnitTest {
  public:
    LoadSaveCredentialsTest() : UnitTest("LoadSave Audial credentials") { }

    void runTest() override {
      AudialCredentials previous = LoadSave::loadAudialCredentials();

      beginTest("roundtrip");
      LoadSave::saveAudialCredentials("https://example.test", "user-1", "key-1");
      AudialCredentials loaded = LoadSave::loadAudialCredentials();
      expectEquals(loaded.base_url, String("https://example.test"));
      expectEquals(loaded.user_id, String("user-1"));
      expectEquals(loaded.api_key, String("key-1"));

      beginTest("default base url");
      LoadSave::saveAudialCredentials("", "user-1", "key-1");
      expectEquals(LoadSave::loadAudialCredentials().base_url, String("https://api.audialmusic.ai"));

      LoadSave::saveAudialCredentials(previous.base_url.toStdString(), previous.user_id.toStdString(),
                                      previous.api_key.toStdString());
    }
};

static LoadSaveCredentialsTest load_save_credentials_test;
```

Append `#include "common/load_save_credentials_test.cpp"` to `tests/interface_tests.cpp`.

- [ ] **Step 2: Run tests to see the failure**

Run: `scripts/run_tests_macos.sh`
Expected: compile error, `saveAudialCredentials` not a member.

- [ ] **Step 3: Implement**

`src/common/load_save.h`: add `#include "audial_client.h"` after `#include "json/json.h"`, and inside the public section after `static void saveWorkOffline(bool work_offline);`:

```cpp
    static void saveAudialCredentials(const std::string& base_url, const std::string& user_id,
                                      const std::string& api_key);
    static AudialCredentials loadAudialCredentials();
```

`src/common/load_save.cpp`: after `LoadSave::saveWorkOffline`:

```cpp
void LoadSave::saveAudialCredentials(const std::string& base_url, const std::string& user_id,
                                     const std::string& api_key) {
  json data = getConfigJson();
  data["audial_base_url"] = base_url;
  data["audial_user_id"] = user_id;
  data["audial_api_key"] = api_key;
  saveJsonToConfig(data);
}

AudialCredentials LoadSave::loadAudialCredentials() {
  json data = getConfigJson();
  AudialCredentials credentials;
  credentials.base_url = "https://api.audialmusic.ai";
  if (data.count("audial_base_url") && data["audial_base_url"].is_string()) {
    std::string base_url = data["audial_base_url"];
    if (!base_url.empty())
      credentials.base_url = base_url;
  }
  if (data.count("audial_user_id") && data["audial_user_id"].is_string())
    credentials.user_id = String(data["audial_user_id"].get<std::string>());
  if (data.count("audial_api_key") && data["audial_api_key"].is_string())
    credentials.api_key = String(data["audial_api_key"].get<std::string>());
  return credentials;
}
```

- [ ] **Step 4: Run tests**

Run: `scripts/run_tests_macos.sh`
Expected: "LoadSave Audial credentials" passes; the previous credentials (if any) are restored in `~/Library/Application Support/AudialSynth/AudialSynth.config`.

- [ ] **Step 5: Commit**

```bash
git add src/common/load_save.h src/common/load_save.cpp tests/common/load_save_credentials_test.cpp tests/interface_tests.cpp
git commit -m "feat(config): persist Audial API credentials in the app config"
```

---

### Task 8: `ResynthSection` overlay, job thread, header button, wiring

**Files:**
- Create: `src/interface/editor_sections/resynth_section.h`, `src/interface/editor_sections/resynth_section.cpp`
- Modify: `src/unity_build/interface_editor_sections.cpp` (append `#include "resynth_section.cpp"`)
- Modify: `src/interface/editor_sections/header_section.h` (Listener method, button member), `header_section.cpp` (constructor, `resized`, `buttonClicked`)
- Modify: `src/interface/editor_sections/full_interface.h` (include, member, override), `full_interface.cpp` (construction, `resized`, override body)

**Interfaces:**
- Consumes: `AudialClient`, `LoadSave::loadAudialCredentials/saveAudialCredentials`, `SynthBase::loadFromFile`, `SynthGuiInterface::externalPresetLoaded`, `LoadSave::getUserPresetDirectory()`.
- Produces: `class ResynthSection : public Overlay, public FileDragAndDropTarget` with `void setVisible(bool)`, `void startJob(const File& sample)`, `void cancelJob()`; `HeaderSection::Listener::showResynthSection()`; `FullInterface::showResynthSection()`.

- [ ] **Step 1: Write `resynth_section.h`**

```cpp
/* Audial Synth: drop a sample, run sound2vital, load the returned preset. GPLv3. */
#pragma once

#include "JuceHeader.h"
#include "overlay.h"
#include "audial_client.h"
#include "open_gl_image_component.h"
#include "synth_button.h"

class ResynthSection : public Overlay, public FileDragAndDropTarget {
  public:
    static constexpr int kPanelWidth = 560;
    static constexpr int kPanelHeight = 460;
    static constexpr int kPaddingX = 24;
    static constexpr int kPaddingY = 20;
    static constexpr int kButtonHeight = 32;
    static constexpr int kTextEditorHeight = 30;
    static constexpr int kDropZoneHeight = 110;
    static constexpr double kMaxSampleSeconds = 20.0;
    static constexpr int kPollIntervalMs = 2000;
    static constexpr int kJobTimeoutMs = 300000;

    enum class State { kIdle, kUploading, kSubmitting, kProcessing, kDownloading, kDone, kError };

    class Job : public Thread {
      public:
        Job(ResynthSection* section) : Thread("Audial Resynth Job"), section_(section) { }
        void run() override { section_->runJob(); }
      private:
        ResynthSection* section_;
    };

    ResynthSection(String name);
    virtual ~ResynthSection();

    void resized() override;
    void setVisible(bool should_be_visible) override;
    void buttonClicked(Button* clicked_button) override;
    void mouseUp(const MouseEvent& e) override;

    bool isInterestedInFileDrag(const StringArray& files) override;
    void filesDropped(const StringArray& files, int x, int y) override;

    void startJob(const File& sample);
    void cancelJob();
    void runJob();

  private:
    Rectangle<int> getPanelRect();
    Rectangle<int> getDropRect();
    void browseForSample();
    void setState(State state, const String& message);
    void postState(State state, const String& message);
    void loadPreset(const File& preset);
    void saveCredentialsFromFields();
    void setTextColors(OpenGlTextEditor* editor, const String& empty_text);
    bool sampleIsAcceptable(const File& sample, String& reason);

    OpenGlQuad body_;
    OpenGlQuad drop_zone_;
    std::unique_ptr<PlainTextComponent> title_text_;
    std::unique_ptr<PlainTextComponent> help_text_;
    std::unique_ptr<PlainTextComponent> drop_text_;
    std::unique_ptr<PlainTextComponent> status_text_;
    std::unique_ptr<PlainTextComponent> credentials_text_;
    std::unique_ptr<OpenGlTextEditor> base_url_;
    std::unique_ptr<OpenGlTextEditor> user_id_;
    std::unique_ptr<OpenGlTextEditor> api_key_;
    std::unique_ptr<OpenGlToggleButton> browse_button_;
    std::unique_ptr<OpenGlToggleButton> save_button_;
    std::unique_ptr<OpenGlToggleButton> cancel_button_;
    std::unique_ptr<OpenGlToggleButton> close_button_;

    AudioFormatManager format_manager_;
    Job job_;
    File sample_;
    std::atomic<State> state_ { State::kIdle };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResynthSection)
};
```

- [ ] **Step 2: Write `resynth_section.cpp`**

```cpp
#include "resynth_section.h"

#include "fonts.h"
#include "load_save.h"
#include "skin.h"
#include "synth_base.h"
#include "synth_gui_interface.h"

ResynthSection::ResynthSection(String name) : Overlay(name), body_(Shaders::kRoundedRectangleFragment),
                                              drop_zone_(Shaders::kRoundedRectangleFragment), job_(this) {
  format_manager_.registerBasicFormats();
  addOpenGlComponent(&body_);
  addOpenGlComponent(&drop_zone_);

  title_text_ = std::make_unique<PlainTextComponent>("title", "Resynth a sample");
  title_text_->setTextSize(20.0f);
  title_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(title_text_.get());

  help_text_ = std::make_unique<PlainTextComponent>("help",
      "Drop a one-shot (20 s max). Audial builds an editable patch from it.");
  help_text_->setTextSize(13.0f);
  help_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(help_text_.get());

  drop_text_ = std::make_unique<PlainTextComponent>("drop", "Drop a .wav / .aif / .flac / .mp3 here, or click Browse");
  drop_text_->setTextSize(14.0f);
  drop_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(drop_text_.get());

  status_text_ = std::make_unique<PlainTextComponent>("status", "");
  status_text_->setTextSize(13.0f);
  status_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(status_text_.get());

  credentials_text_ = std::make_unique<PlainTextComponent>("credentials", "Audial API credentials");
  credentials_text_->setTextSize(13.0f);
  credentials_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(credentials_text_.get());

  base_url_ = std::make_unique<OpenGlTextEditor>("Base URL");
  user_id_ = std::make_unique<OpenGlTextEditor>("User ID");
  api_key_ = std::make_unique<OpenGlTextEditor>("API Key", L'*');
  for (OpenGlTextEditor* editor : { base_url_.get(), user_id_.get(), api_key_.get() }) {
    editor->setMultiLine(false);
    addAndMakeVisible(editor);
    addOpenGlComponent(editor->getImageComponent());
  }

  browse_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Browse"));
  save_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Save credentials"));
  cancel_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Cancel"));
  close_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Close"));
  browse_button_->setUiButton(true);
  save_button_->setUiButton(false);
  cancel_button_->setUiButton(false);
  close_button_->setUiButton(false);
  for (OpenGlToggleButton* button : { browse_button_.get(), save_button_.get(), cancel_button_.get(), close_button_.get() }) {
    button->addListener(this);
    addAndMakeVisible(button);
    addOpenGlComponent(button->getGlComponent());
  }
  cancel_button_->setVisible(false);
}

ResynthSection::~ResynthSection() {
  job_.stopThread(1000);
}

Rectangle<int> ResynthSection::getPanelRect() {
  int x = (getWidth() - kPanelWidth) / 2;
  int y = (getHeight() - kPanelHeight) / 2;
  return Rectangle<int>(x, y, kPanelWidth, kPanelHeight);
}

Rectangle<int> ResynthSection::getDropRect() {
  Rectangle<int> panel = getPanelRect();
  return Rectangle<int>(panel.getX() + kPaddingX, panel.getY() + kPaddingY + 64,
                        panel.getWidth() - 2 * kPaddingX, kDropZoneHeight);
}

void ResynthSection::setTextColors(OpenGlTextEditor* editor, const String& empty_text) {
  editor->setColour(CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
  editor->setColour(TextEditor::textColourId, findColour(Skin::kPresetText, true));
  editor->setColour(TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
  editor->setColour(TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
  Colour empty_color = findColour(Skin::kBodyText, true);
  editor->setTextToShowWhenEmpty(empty_text, empty_color.withAlpha(0.5f * empty_color.getFloatAlpha()));
  editor->applyFontToAllText(Fonts::instance()->proportional_light().withPointHeight(14.0f), true);
  editor->redoImage();
}

void ResynthSection::resized() {
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));
  drop_zone_.setRounding(findValue(Skin::kBodyRounding));
  drop_zone_.setColor(findColour(Skin::kBody, true).overlaidWith(findColour(Skin::kLightenScreen, true)));

  Colour text_color = findColour(Skin::kBodyText, true);
  for (PlainTextComponent* text : { title_text_.get(), help_text_.get(), drop_text_.get(),
                                    status_text_.get(), credentials_text_.get() })
    text->setColor(text_color);

  Rectangle<int> panel = getPanelRect();
  body_.setBounds(panel);
  int text_width = panel.getWidth() - 2 * kPaddingX;
  int x = panel.getX() + kPaddingX;
  title_text_->setBounds(x, panel.getY() + kPaddingY, text_width, 28);
  help_text_->setBounds(x, panel.getY() + kPaddingY + 30, text_width, 22);

  Rectangle<int> drop = getDropRect();
  drop_zone_.setBounds(drop);
  drop_text_->setBounds(drop.getX(), drop.getY() + drop.getHeight() / 2 - 24, drop.getWidth(), 22);
  browse_button_->setBounds(drop.getX() + drop.getWidth() / 2 - 60, drop.getBottom() - kButtonHeight - 10, 120, kButtonHeight);

  int status_y = drop.getBottom() + 12;
  status_text_->setBounds(x, status_y, text_width, 22);

  int creds_y = status_y + 36;
  credentials_text_->setBounds(x, creds_y, text_width, 20);
  int field_y = creds_y + 26;
  base_url_->setBounds(x, field_y, text_width, kTextEditorHeight);
  user_id_->setBounds(x, field_y + kTextEditorHeight + 8, text_width / 2 - 6, kTextEditorHeight);
  api_key_->setBounds(x + text_width / 2 + 6, field_y + kTextEditorHeight + 8, text_width / 2 - 6, kTextEditorHeight);
  setTextColors(base_url_.get(), "https://api.audialmusic.ai");
  setTextColors(user_id_.get(), "Audial user id");
  setTextColors(api_key_.get(), "Audial API key");

  int buttons_y = panel.getBottom() - kPaddingY - kButtonHeight;
  int button_width = 140;
  save_button_->setBounds(x, buttons_y, button_width, kButtonHeight);
  cancel_button_->setBounds(panel.getRight() - kPaddingX - 2 * button_width - 12, buttons_y, button_width, kButtonHeight);
  close_button_->setBounds(panel.getRight() - kPaddingX - button_width, buttons_y, button_width, kButtonHeight);

  Overlay::resized();
}

void ResynthSection::setVisible(bool should_be_visible) {
  Overlay::setVisible(should_be_visible);
  if (should_be_visible) {
    AudialCredentials credentials = LoadSave::loadAudialCredentials();
    base_url_->setText(credentials.base_url);
    user_id_->setText(credentials.user_id);
    api_key_->setText(credentials.api_key);
    if (state_ == State::kDone || state_ == State::kError)
      setState(State::kIdle, "");
    Image image(Image::ARGB, 1, 1, false);
    Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }
}

void ResynthSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == browse_button_.get())
    browseForSample();
  else if (clicked_button == save_button_.get())
    saveCredentialsFromFields();
  else if (clicked_button == cancel_button_.get())
    cancelJob();
  else if (clicked_button == close_button_.get())
    setVisible(false);
}

void ResynthSection::mouseUp(const MouseEvent& e) {
  if (!getPanelRect().contains(e.getPosition()) && job_.isThreadRunning() == false)
    setVisible(false);
}

bool ResynthSection::isInterestedInFileDrag(const StringArray& files) {
  if (files.size() != 1)
    return false;
  StringArray wildcards;
  wildcards.addTokens(format_manager_.getWildcardForAllFormats(), ";", "\"");
  for (const String& wildcard : wildcards) {
    if (files[0].matchesWildcard(wildcard, true))
      return true;
  }
  return false;
}

void ResynthSection::filesDropped(const StringArray& files, int x, int y) {
  if (files.size() == 1)
    startJob(File(files[0]));
}

void ResynthSection::browseForSample() {
  FileChooser open_box("Choose a sample", File(), format_manager_.getWildcardForAllFormats());
  if (open_box.browseForFileToOpen())
    startJob(open_box.getResult());
}

void ResynthSection::saveCredentialsFromFields() {
  LoadSave::saveAudialCredentials(base_url_->getText().trim().toStdString(),
                                  user_id_->getText().trim().toStdString(),
                                  api_key_->getText().trim().toStdString());
  setState(state_, "Credentials saved");
}

bool ResynthSection::sampleIsAcceptable(const File& sample, String& reason) {
  std::unique_ptr<AudioFormatReader> reader(format_manager_.createReaderFor(sample));
  if (reader == nullptr) {
    reason = "Could not read " + sample.getFileName();
    return false;
  }
  double seconds = reader->lengthInSamples / reader->sampleRate;
  if (seconds > kMaxSampleSeconds) {
    reason = "Sample is " + String(seconds, 1) + " s; the limit is 20 s";
    return false;
  }
  if (reader->lengthInSamples < 256) {
    reason = "Sample is too short";
    return false;
  }
  return true;
}

void ResynthSection::startJob(const File& sample) {
  if (job_.isThreadRunning()) {
    setState(state_, "A job is already running");
    return;
  }
  String reason;
  if (!sampleIsAcceptable(sample, reason)) {
    setState(State::kError, reason);
    return;
  }
  saveCredentialsFromFields();
  if (!LoadSave::loadAudialCredentials().complete()) {
    setState(State::kError, "Enter your Audial user id and API key first");
    return;
  }
  sample_ = sample;
  setState(State::kUploading, "Uploading " + sample.getFileName() + "...");
  job_.startThread();
}

void ResynthSection::cancelJob() {
  job_.signalThreadShouldExit();
  job_.stopThread(3000);
  setState(State::kIdle, "Cancelled");
}

void ResynthSection::setState(State state, const String& message) {
  state_ = state;
  status_text_->setText(message);
  bool running = state == State::kUploading || state == State::kSubmitting ||
                 state == State::kProcessing || state == State::kDownloading;
  cancel_button_->setVisible(running);
  browse_button_->setVisible(!running);
  repaint();
}

void ResynthSection::postState(State state, const String& message) {
  Component::SafePointer<ResynthSection> safe(this);
  MessageManager::callAsync([safe, state, message] {
    if (safe != nullptr)
      safe->setState(state, message);
  });
}

void ResynthSection::runJob() {
  AudialClient client(LoadSave::loadAudialCredentials());
  String exe_id = Uuid().toDashedString();
  String filename = AudialClient::sanitizeFilename(sample_.getFileName());

  HttpResult upload = client.uploadReference(sample_, exe_id, filename);
  if (job_.threadShouldExit()) return;
  if (!upload.ok()) {
    postState(State::kError, "Upload failed: " + upload.describe());
    return;
  }
  String file_url = AudialClient::parseUrl(upload.body);
  if (file_url.isEmpty()) {
    postState(State::kError, "Upload returned no URL");
    return;
  }

  postState(State::kSubmitting, "Submitting to sound2vital...");
  HttpResult run = client.runSound2Vital(filename, file_url);
  if (job_.threadShouldExit()) return;
  if (!run.ok()) {
    postState(State::kError, "Run failed: " + run.describe());
    return;
  }
  json parsed = json::parse(run.body.toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.count("exeId") || !parsed["exeId"].is_string()) {
    postState(State::kError, "Run returned no execution id");
    return;
  }
  String job_exe_id = String(parsed["exeId"].get<std::string>());

  uint32 started = Time::getMillisecondCounter();
  AudialClient::ExecutionStatus status;
  while (true) {
    if (job_.threadShouldExit()) return;
    if (Time::getMillisecondCounter() - started > (uint32)kJobTimeoutMs) {
      postState(State::kError, "Timed out after 5 minutes; the job keeps running on the server");
      return;
    }
    job_.wait(kPollIntervalMs);
    if (job_.threadShouldExit()) return;
    HttpResult poll = client.getExecution(job_exe_id);
    if (!poll.ok()) {
      postState(State::kProcessing, "Waiting (" + poll.describe() + ")");
      continue;
    }
    status = AudialClient::parseExecution(poll.body);
    if (status.state == "completed" && status.preset_url.isNotEmpty())
      break;
    if (status.state == "failed") {
      postState(State::kError, status.error);
      return;
    }
    int seconds = (int)((Time::getMillisecondCounter() - started) / 1000);
    postState(State::kProcessing, "Processing... " + String(seconds) + " s");
  }

  postState(State::kDownloading, "Downloading preset...");
  File folder = LoadSave::getUserPresetDirectory().getChildFile("Resynth");
  String stamp = Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
  File preset = folder.getChildFile(sample_.getFileNameWithoutExtension().retainCharacters(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") + "_" + stamp + ".vital");
  if (!client.downloadToFile(status.preset_url, preset)) {
    postState(State::kError, "Preset download failed");
    return;
  }
  if (job_.threadShouldExit()) return;

  Component::SafePointer<ResynthSection> safe(this);
  MessageManager::callAsync([safe, preset] {
    if (safe != nullptr)
      safe->loadPreset(preset);
  });
}

void ResynthSection::loadPreset(const File& preset) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr) {
    setState(State::kError, "No synth to load into");
    return;
  }
  std::string error;
  if (!parent->getSynth()->loadFromFile(preset, error)) {
    setState(State::kError, "Preset load failed: " + String(error));
    return;
  }
  parent->externalPresetLoaded(preset);
  setState(State::kDone, "Loaded " + preset.getFileName());
  setVisible(false);
}
```

Append `#include "resynth_section.cpp"` to `src/unity_build/interface_editor_sections.cpp`.

- [ ] **Step 3: Header button**

`header_section.h`: inside `HeaderSection::Listener` add `virtual void showResynthSection() = 0;`; in the private members add `std::unique_ptr<OpenGlToggleButton> resynth_button_;`.

`header_section.cpp`, constructor, after the `view_spectrogram_` block:

```cpp
  resynth_button_ = std::make_unique<OpenGlToggleButton>("RESYNTH");
  resynth_button_->setUiButton(true);
  resynth_button_->setText("RESYNTH");
  resynth_button_->addListener(this);
  addAndMakeVisible(resynth_button_.get());
  addOpenGlComponent(resynth_button_->getGlComponent());
```

`HeaderSection::resized()`: replace

```cpp
  int tabs_width = preset_selector_x - component_padding - tab_offset_;
  tab_selector_->setBounds(tab_offset_, 0, tabs_width, height);
```

with

```cpp
  int resynth_width = height * 2.4f;
  int resynth_height = height * 0.6f;
  resynth_button_->setBounds(preset_selector_x - component_padding - resynth_width,
                             (height - resynth_height) / 2, resynth_width, resynth_height);
  int tabs_width = resynth_button_->getX() - component_padding - tab_offset_;
  tab_selector_->setBounds(tab_offset_, 0, tabs_width, height);
```

`HeaderSection::buttonClicked()`: add as the first branch

```cpp
  if (clicked_button == resynth_button_.get()) {
    resynth_button_->setToggleState(false, dontSendNotification);
    for (Listener* listener : listeners_)
      listener->showResynthSection();
    return;
  }
```

- [ ] **Step 4: FullInterface wiring**

`full_interface.h`: add `class ResynthSection;` to the forward declarations, `void showResynthSection() override;` next to `void showAboutSection() override;`, and `std::unique_ptr<ResynthSection> resynth_section_;` next to `update_check_section_`.

`full_interface.cpp`: add `#include "resynth_section.h"`. After the `update_check_section_` block in the constructor:

```cpp
  resynth_section_ = std::make_unique<ResynthSection>("resynth");
  addSubSection(resynth_section_.get(), false);
  addChildComponent(resynth_section_.get());
  resynth_section_->setAlwaysOnTop(true);
```

In `FullInterface::resized()` next to `update_check_section_->setBounds(bounds);` add `resynth_section_->setBounds(bounds);`. Add the method:

```cpp
void FullInterface::showResynthSection() {
  resynth_section_->setVisible(true);
}
```

- [ ] **Step 5: Build, then run the unit tests and the app**

Run: `scripts/build_macos.sh Release AudialSynth 2>&1 | tail -3 && scripts/run_tests_macos.sh`
Expected: builds; all unit tests still pass (the full-interface stress test now instantiates the new section).

Run: `open standalone/builds/osx/build/Release/AudialSynth.app`
Expected: the header shows RESYNTH; clicking it opens the panel; dropping a 21 s file shows "Sample is 21.0 s; the limit is 20 s"; with empty credentials, dropping a valid file shows "Enter your Audial user id and API key first"; Close hides the panel.

- [ ] **Step 6: Commit**

```bash
git add src/interface/editor_sections/resynth_section.h src/interface/editor_sections/resynth_section.cpp src/unity_build/interface_editor_sections.cpp src/interface/editor_sections/header_section.h src/interface/editor_sections/header_section.cpp src/interface/editor_sections/full_interface.h src/interface/editor_sections/full_interface.cpp
git commit -m "feat(resynth): drop-zone overlay, job thread, header button"
```

---

### Task 9: Mock Audial API and end-to-end GUI verification

**Files:**
- Create: `tools/mock_audial_api.py`, `docs/manual-checklist.md`

**Interfaces:**
- Produces: a local server implementing `PUT /api/files/{u}/execution/{e}/reference/{n}`, `POST /api/functions/run/sound2vital`, `GET /api/db/{u}/execution/{e}`, `GET /files/<name>`; completes each run after `--delay` seconds with `--preset` as the result.

- [ ] **Step 1: Write `tools/mock_audial_api.py`**

```python
"""Stand-in for the Audial API so the Resynth flow can be exercised offline.

  python tools/mock_audial_api.py --preset /path/to/preset.vital --port 8766 --delay 6
"""
import argparse
import json
import re
import threading
import time
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

UPLOAD = re.compile(r"^/api/files/([^/]+)/execution/([^/]+)/reference/([^/]+)$")
EXECUTION = re.compile(r"^/api/db/([^/]+)/execution/([^/]+)$")
executions = {}
lock = threading.Lock()


def make_handler(preset: Path, port: int, delay: float, fail: bool):
    class Handler(BaseHTTPRequestHandler):
        def _json(self, status, payload):
            data = json.dumps(payload).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def _authorised(self):
            return bool(self.headers.get("x-api-key")) and bool(self.headers.get("x-user-id"))

        def do_PUT(self):
            match = UPLOAD.match(self.path)
            if not match or not self._authorised():
                return self._json(403, {"error": "Unauthorized"})
            self.rfile.read(int(self.headers.get("Content-Length", "0")))
            user, exe, name = match.groups()
            self._json(200, {"url": f"http://localhost:{port}/files/{user}/{exe}/{name}"})

        def do_POST(self):
            if self.path != "/api/functions/run/sound2vital" or not self._authorised():
                return self._json(403, {"error": "Unauthorized"})
            body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", "0"))))
            exe = str(uuid.uuid4())
            with lock:
                executions[exe] = {"exeId": exe, "state": "created", "original": body["original"],
                                   "created": time.time(), "exeType": "preset"}
            self._json(200, executions[exe])

        def do_GET(self):
            if self.path == "/files/preset.vital":
                data = preset.read_bytes()
                self.send_response(200); self.send_header("Content-Length", str(len(data))); self.end_headers()
                return self.wfile.write(data)
            match = EXECUTION.match(self.path)
            if not match or not self._authorised():
                return self._json(403, {"error": "Unauthorized"})
            exe = match.group(2)
            with lock:
                record = executions.get(exe)
                if record is None:
                    return self._json(404, {"error": "not found"})
                age = time.time() - record["created"]
                if age >= delay:
                    if fail:
                        record.update(state="failed", error="Input is 25.0 s; the limit is 20 s")
                    else:
                        record.update(state="completed", preset={"presetvital": {
                            "filename": "preset.vital", "url": f"http://localhost:{port}/files/preset.vital"}})
                elif age >= 1:
                    record["state"] = "processing"
                self._json(200, record)

        def log_message(self, fmt, *args):
            print("mock-audial", self.command, self.path)
    return Handler


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preset", type=Path, required=True)
    parser.add_argument("--port", type=int, default=8766)
    parser.add_argument("--delay", type=float, default=6.0)
    parser.add_argument("--fail", action="store_true", help="complete every job as failed")
    args = parser.parse_args()
    ThreadingHTTPServer(("127.0.0.1", args.port), make_handler(args.preset.resolve(), args.port, args.delay, args.fail)).serve_forever()
```

- [ ] **Step 2: Write `docs/manual-checklist.md`**

```markdown
# Manual checklist (run after every GUI change)

Start the mock: `python3 tools/mock_audial_api.py --preset <any engine preset.vital> --delay 6`
Open `standalone/builds/osx/build/Release/AudialSynth.app`, click RESYNTH.

1. Credentials: base URL `http://localhost:8766`, user `u1`, key `k1`, Save. Quit and reopen: fields are restored.
2. Drop a 21 s wav: status "Sample is 21.0 s; the limit is 20 s"; nothing is sent (mock log empty).
3. Drop a 3 s wav: status goes Uploading → Submitting → Processing... N s → Downloading; the panel closes and the header preset name is the new `<sample>_<stamp>` file; sound plays from the keyboard; the preset browser lists it under User/Resynth.
4. Restart the mock with `--fail`, drop the 3 s wav: status shows "Input is 25.0 s; the limit is 20 s" and the panel stays open.
5. Start a job, click Cancel during Processing: status "Cancelled", Browse visible again.
6. Stop the mock, drop the 3 s wav: status "Upload failed: no connection (check base URL / network)".
7. Load the VST3 in a DAW (Ableton or Logic) and repeat step 3.
```

- [ ] **Step 3: Run the checklist**

Run every item. Expected: every line behaves as written. Record deviations in `docs/build-notes.md` and fix them before committing.

- [ ] **Step 4: Commit**

```bash
git add tools/mock_audial_api.py docs/manual-checklist.md
git commit -m "test: mock Audial API and manual GUI checklist"
```

---

### Task 10: Parity check against Vital 1.6.4

**Files:**
- Create: `tools/parity_check.py`, `docs/parity.md`
- Output (not committed): `qualification/parity/<date>/parity.json`, worst-10 wavs

**Interfaces:**
- Consumes: engine modules from `../genetic_vital/src` (`scripts.v2.vital_vst3.load_plugin/apply_preset_state/render_midi`, `sound2vital.diagnostics.compare_audio`), the installed `/Library/Audio/Plug-Ins/VST3/Vital.vst3`, the fork's `plugin/builds/osx/build/Release/AudialSynth.vst3`.

- [ ] **Step 1: Write `tools/parity_check.py`**

```python
"""Render presets through Vital 1.6.4 and Audial Synth and report per-preset residuals.

  ../genetic_vital/.venv/bin/python tools/parity_check.py --out qualification/parity/2026-09-30
"""
import argparse
import glob
import json
import sys
from pathlib import Path

import numpy as np
import soundfile as sf

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE.parent / "genetic_vital" / "src"))
from scripts.v2.vital_vst3 import load_plugin, apply_preset_state, render_midi  # noqa: E402
from sound2vital.diagnostics import compare_audio  # noqa: E402

DEFAULT_PRESETS = "/Users/zachfarrell/Desktop/Coding_Projects/genetic/queries/results/engine_v3/editable_corpus_parallel_v8/run/native/*/work/preset.vital"


def render(plugin, state, seconds):
    apply_preset_state(plugin, state)
    return render_midi(plugin, duration=seconds, note_duration=seconds, midi_note=60, sample_rate=44100)


def residual_db(reference, candidate):
    n = min(reference.shape[1], candidate.shape[1])
    a, b = reference[:, :n], candidate[:, :n]
    ref_rms = np.sqrt(np.mean(a ** 2)) + 1e-12
    return float(20 * np.log10(np.sqrt(np.mean((a - b) ** 2)) / ref_rms + 1e-12))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--presets", default=DEFAULT_PRESETS)
    parser.add_argument("--reference", type=Path, default=Path("/Library/Audio/Plug-Ins/VST3/Vital.vst3"))
    parser.add_argument("--fork", type=Path, default=HERE / "plugin/builds/osx/build/Release/AudialSynth.vst3")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--limit", type=int)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    reference = load_plugin(args.reference)
    fork = load_plugin(args.fork)
    rows = []
    for path in sorted(glob.glob(args.presets))[: args.limit]:
        preset = Path(path)
        state = json.loads(preset.read_text())
        report = preset.with_name("report.json")
        seconds = json.loads(report.read_text()).get("source_duration_s", 2.0) if report.is_file() else 2.0
        seconds = float(min(max(seconds, 0.5), 20.0)) + 0.25
        a = render(reference, state, seconds)
        b = render(fork, state, seconds)
        row = {"preset": str(preset), "seconds": seconds, "residual_db": residual_db(a, b),
               "compare_audio": compare_audio(a, b)}
        rows.append(row)
        print(json.dumps({"preset": preset.parent.parent.name, "residual_db": round(row["residual_db"], 2)}), flush=True)
        key = preset.parent.parent.name
        sf.write(args.out / f"{key}_vital164.wav", a.T, 44100)
        sf.write(args.out / f"{key}_audialsynth.wav", b.T, 44100)
    rows.sort(key=lambda r: r["residual_db"], reverse=True)
    values = np.array([r["residual_db"] for r in rows])
    summary = {"count": len(rows), "median_residual_db": float(np.median(values)),
               "worst_residual_db": float(values.max()), "below_minus_40_db": int((values < -40).sum()),
               "rows": rows}
    (args.out / "parity.json").write_text(json.dumps(summary, indent=2, default=float))
    print(json.dumps({k: summary[k] for k in ("count", "median_residual_db", "worst_residual_db", "below_minus_40_db")}))


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Dry run on three presets**

Run: `../genetic_vital/.venv/bin/python tools/parity_check.py --out qualification/parity/dryrun --limit 3`
Expected: three lines with residual values and a summary; wav pairs in the output directory. A residual near 0 dB means the two synths disagree as much as the signal itself; values below −40 dB mean near-identical output.

- [ ] **Step 3: Full run and review**

Run without `--limit`. Then listen to the ten worst pairs and view their spectrograms (`../genetic_vital/src/scripts/compare_visual.py` or any viewer). Write `docs/parity.md`:

```markdown
# Parity: Audial Synth (upstream 1.0.6 DSP) vs Vital 1.6.4

Run: qualification/parity/<date> (parity.json)
Presets: 207 engine results (editable_corpus_parallel_v8)
Median residual: <dB>   Worst: <dB>   Below −40 dB: <n>/207

Worst ten, with what differs audibly and which module is responsible:
1. <preset key>: <observation>
...

Decision: <proceed to pin PLUGIN_COMMIT | fix <module> first>, because <reason>.
```

- [ ] **Step 4: Commit**

```bash
git add tools/parity_check.py docs/parity.md
git commit -m "tools: parity check against Vital 1.6.4 with recorded decision"
```

---

### Task 11: Release build, packaging, and backend pin

**Files:**
- Create: `scripts/package_macos.sh`, `docs/distribution.md`
- Output (not committed): `dist/AudialSynth-macOS-<sha>.zip`

**Interfaces:**
- Produces: a zip with `AudialSynth.app`, `AudialSynth.vst3`, `AudialSynth.component`, `LICENSE`, and `SOURCE.txt` pointing at the exact commit; git tag `backend-v1` whose sha is the microservice's `PLUGIN_COMMIT`.

- [ ] **Step 1: Write `scripts/package_macos.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SHA="$(git -C "$ROOT" rev-parse --short HEAD)"
"$ROOT/scripts/build_macos.sh" Release AudialSynth >/dev/null
OUT="$ROOT/dist/AudialSynth-macOS-$SHA"
rm -rf "$OUT" && mkdir -p "$OUT"
cp -R "$ROOT/standalone/builds/osx/build/Release/AudialSynth.app" "$OUT/"
cp -R "$ROOT/plugin/builds/osx/build/Release/AudialSynth.vst3" "$OUT/"
cp -R "$ROOT/plugin/builds/osx/build/Release/AudialSynth.component" "$OUT/"
cp "$ROOT/LICENSE" "$OUT/LICENSE"
printf 'Audial Synth is GPLv3. Source for this build: https://github.com/zfarrell13/sound2vital-gui/tree/%s\n' "$(git -C "$ROOT" rev-parse HEAD)" > "$OUT/SOURCE.txt"
(cd "$ROOT/dist" && rm -f "AudialSynth-macOS-$SHA.zip" && zip -qr "AudialSynth-macOS-$SHA.zip" "AudialSynth-macOS-$SHA")
ls -la "$ROOT/dist/AudialSynth-macOS-$SHA.zip"
```

`chmod +x scripts/package_macos.sh`; add `dist/` to `.gitignore`.

- [ ] **Step 2: Write `docs/distribution.md`**

```markdown
# Distributing Audial Synth

- `scripts/package_macos.sh` builds Release and zips the app, VST3 and AU with
  the licence and a SOURCE.txt link to the exact commit (GPLv3 requires it).
- The build is unsigned in this iteration: users must right-click → Open the
  app once, and DAWs on macOS 15+ will refuse the unsigned VST3/AU until the
  bundle is signed and notarized. Signing is the next packaging task.
- Never include factory presets or wavetables from vital.audio.
- Tag the commit used for the hosted engine's render backend:
  `git tag backend-v1 && git push --tags`. The tag's sha is `PLUGIN_COMMIT`
  in sound2vital-runpod.
```

- [ ] **Step 3: Package, tag, push**

```bash
scripts/package_macos.sh
git add scripts/package_macos.sh docs/distribution.md .gitignore
git commit -m "release: macOS packaging script and distribution notes"
git tag backend-v1
git remote add origin https://github.com/zfarrell13/sound2vital-gui.git
git push -u origin main --tags
git rev-parse backend-v1
```

Expected: the zip exists; the printed sha goes into the microservice repository variable `PLUGIN_COMMIT` and `docs/build-notes.md`.

---

## Self-review notes

- Spec §3 (naming, logo, no vital.audio) → Task 3. §6 (build targets) → Tasks 2, 4. §7 (Resynth section and job flow) → Task 8. §8 (client) → Task 6. Credentials (§7 item 4) → Task 7. §9 (parity) → Task 10. §10 (testing) → Tasks 6, 7, 9. §4 version gate → Task 5. Distribution and backend pin → Task 11.
- Interface names match across tasks: `AudialCredentials`, `HttpResult`, `AudialClient::parseExecution`, `LoadSave::loadAudialCredentials`, `ResynthSection::startJob`, `HeaderSection::Listener::showResynthSection`, `FullInterface::showResynthSection`.
- Known judgement points, stated rather than hidden: JUCE 6.0.5 against the current Xcode may need small fixes (Task 2 Step 3 says where to record them); the firebase framework removal is conditional; parity acceptance is a reviewed decision, not a numeric gate.
