# EQInfinity

Cross-platform JUCE-based audio effect plugin scaffold (VST3/AU/Standalone) built with CMake (C++17).

## Getting JUCE (as submodule)

This repo expects JUCE at `third_party/JUCE`.

```bash
git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE
git submodule update --init --recursive
```

> **Note:** JUCE is licensed separately. See JUCE licensing terms on the JUCE repository. This scaffold does not include JUCE.

## Build (macOS)

**Prereqs**
- Xcode + command line tools
- CMake 3.21+
- Ninja (recommended)

```bash
./scripts/setup_macos.sh
./scripts/configure.sh -G Ninja
./scripts/build.sh
```

Binaries will be in `build/EQInfinity_artefacts/`.

## Build (Windows)

**Prereqs**
- Visual Studio 2022 (Desktop development with C++)
- CMake 3.21+
- (Optional) Ninja

PowerShell:

```powershell
.\scripts\setup_windows.ps1
.\scripts\configure.sh -G "Visual Studio 17 2022" -A x64
.\scripts\build.sh --config Release
```

Binaries will be in `build\EQInfinity_artefacts\`.

## Plugin scan paths

### macOS
- AU: `/Library/Audio/Plug-Ins/Components` and `~/Library/Audio/Plug-Ins/Components`
- VST3: `/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/VST3`

### Windows
- VST3: `C:\Program Files\Common Files\VST3`

## Development notes

- Initial plugin is a pass-through with an **Output Gain** parameter (AudioProcessorValueTreeState).
- State is saved/restored via APVTS XML.
- Supports mono and stereo buses.
- **Audio thread rules:** no allocations and no locks inside `processBlock`.

## Licensing

- `LICENSE` in this repo is a placeholder for your project’s license.
- JUCE is a third-party dependency and is licensed separately; ensure compliance with JUCE licensing for your distribution.
