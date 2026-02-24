# AGENTS.md instructions
Project: 8-band EQ plugin (EQ Eight–style workflow) using JUCE (C++)  
Targets: VST3 (Win/macOS) + AU (macOS), plus Standalone for dev  
Non-goals: Copying Ableton branding/trade dress; AAX is optional and out of initial scope.

## How agents should work
**Antigravity workflow** (required):
1) Make a short plan with milestones and deliverables.
2) Scaffold repo structure and build tooling.
3) Implement features in small, testable increments.
4) Keep the project runnable at each step (Standalone + plugin builds).
5) Add documentation and scripts so a new dev can build within 10 minutes.

**Rules:**
- Prioritize “builds + runs” over feature completeness.
- Never block on UI polish early; get audio processing working first.
- Avoid heavy dependencies unless justified.
- Do not perform allocations, locks, or file I/O on the audio thread.
- Do not copy EQ Eight visuals; replicate interaction patterns with original styling.

## Quick outcome definition
This repo should result in:
- A JUCE-based plugin that compiles on macOS and Windows
- Builds produce: Standalone app + VST3 (and AU on macOS)
- 8 bands with per-band: Type, Freq, Gain (if applicable), Q (if applicable), Slope (cuts), Enabled
- Global output gain
- A simple EQ graph UI with draggable nodes
- (Phase 2) FFT spectrum analyzer overlay

## Repo structure (target)
.
├── README.md
├── agent.md
├── LICENSE
├── .gitignore
├── .editorconfig
├── .clang-format
├── .github/
│   └── workflows/
│       ├── build-macos.yml
│       └── build-windows.yml
├── scripts/
│   ├── setup_macos.sh
│   ├── setup_windows.ps1
│   ├── configure.sh
│   ├── build.sh
│   └── format.sh
├── third_party/
│   └── JUCE/           (git submodule OR pinned download)
├── CMakeLists.txt
└── src/
    ├── PluginProcessor.h/.cpp
    ├── PluginEditor.h/.cpp
    ├── dsp/
    │   ├── EqBand.h/.cpp
    │   ├── EqEngine.h/.cpp
    │   ├── SmoothedValueHelpers.h/.cpp
    │   └── ResponseCurve.h/.cpp
    ├── ui/
    │   ├── EqPlotComponent.h/.cpp
    │   ├── SpectrumAnalyzer.h/.cpp (phase 2)
    │   ├── LookAndFeel.h/.cpp
    │   └── Widgets.h/.cpp
    └── util/
        ├── Params.h/.cpp
        └── Debug.h/.cpp

## Build system requirements
Use **CMake** + JUCE CMake integration.

### macOS
- Xcode + command line tools
- Builds: AU, VST3, Standalone
- Note: AU requires proper bundle signing to load in some hosts; for dev, local builds are fine.

### Windows
- Visual Studio 2022 (or latest installed)
- Builds: VST3, Standalone

## Task breakdown (agents should follow this order)
### Milestone 0 — Repo bootstrap
- Initialize git repo with the target folder structure.
- Add `.gitignore` suitable for:
  - macOS `.DS_Store`, Xcode, DerivedData
  - Windows build artifacts
  - CMake build dirs (`build/`, `out/`)
  - JUCE generated intermediates if any
- Add `README.md` with:
  - what it is
  - how to build on macOS / Windows
  - plugin install locations and how to rescan in DAWs
- Add formatting tools:
  - `.clang-format`
  - scripts/format.sh (clang-format over src)

Deliverable: clean scaffold with CI placeholders and docs.

### Milestone 1 — JUCE + CMake “hello plugin”
- Add JUCE as:
  - Preferred: git submodule at `third_party/JUCE`
  - Alternative: pinned release tarball with checksum notes in README
- Create a JUCE plugin project via CMake:
  - plugin name: `GravityEQ` (placeholder; can change later)
  - formats: VST3, AU (mac), Standalone
- Minimal audio processor that passes audio through (unity gain).
- Minimal editor UI (just a label and an output gain slider).

Deliverable: builds run; plugin loads in at least one host.

### Milestone 2 — Parameter system (APVTS)
- Implement `Params` module defining:
  - 8 bands * (enabled, type, freq, gain, Q, slope)
  - global output gain
- Use `AudioProcessorValueTreeState` (APVTS)
- Ensure parameter IDs are stable and versionable:
  - e.g. `b1_enabled`, `b1_type`, `b1_freq`, `b1_gain`, `b1_q`, `b1_slope` … `b8_*`, `out_gain`

Deliverable: parameters exist, automate correctly, state saves/loads.

### Milestone 3 — DSP engine (8-band EQ)
- Implement `EqBand` that can represent:
  - Peak (bell)
  - LowShelf / HighShelf
  - HPF / LPF with selectable slope (12/24/36/48 dB/oct by cascading biquads)
- Implement smoothing for freq/gain/Q changes:
  - per-block coefficient updates
  - or smoothed parameter values with a safe update cadence
- Build `EqEngine` combining enabled bands in series.
- Validate with basic test audio (pink noise / sine sweeps) using host analyzer.

Deliverable: audible EQ that is stable, click-free, and CPU-safe.

### Milestone 4 — EQ response + graph UI
- Implement `ResponseCurve` computation (not on audio thread):
  - calculate combined magnitude response for drawing
  - update at ~30–60 Hz on a timer
- Implement `EqPlotComponent`:
  - log-frequency x axis (20 Hz–20 kHz)
  - dB y axis (e.g. +24 to -24)
  - draggable nodes:
    - x = frequency
    - y = gain (where applicable)
    - modifier or wheel adjusts Q
  - selection highlight for active band
- Add a side panel (or bottom bar) to edit the selected band.

Deliverable: EQ Eight–style interaction with original look.

### Milestone 5 — Spectrum analyzer (Phase 2)
- Implement `SpectrumAnalyzer`:
  - FFT using JUCE dsp FFT
  - ring buffer to move samples from audio thread to GUI safely (lock-free)
  - draw magnitude trace in the plot
- Optional: pre/post toggle.

Deliverable: stable analyzer without audio thread issues.

### Milestone 6 — Packaging & polish
- Add presets (optional; host state is enough initially)
- Ensure resizing works
- Add CI builds for macOS + Windows (best-effort; codesign can be excluded for CI)
- Finalize README troubleshooting section:
  - plugin scan paths
  - common “plugin not showing” fixes

## Coding standards
- C++17 minimum.
- No dynamic allocations in `processBlock`.
- No mutex locks in `processBlock`.
- Use atomics or lock-free FIFOs for analyzer data transfer.
- Keep DSP separate from UI (`src/dsp` vs `src/ui`).
- Keep parameter IDs centralized in `Params`.

## Testing checklist (agent should run)
- Sample rates: 44.1k, 48k, 96k
- Buffer sizes: 64, 128, 512, 1024
- Automation: record automation on freq/gain/Q and verify no pops
- Save project → reopen: settings persist
- Mono and stereo correctness
- CPU sanity: analyzer does not spike CPU; silence input does not cause denormals

## Deliverables for each PR/change
- Clear commit messages
- Updated README if build steps change
- Screenshots/gifs optional but welcome
- If adding a dependency, document it and justify

## Antigravity “definition of done”
A milestone is done only when:
- The project builds successfully on at least one platform
- Standalone runs
- Plugin loads in a host (at least one DAW or plugin host)
- No obvious audio thread violations introduced

## Notes on legal / design
- UI should be “inspired by” the workflow (bands + graph + nodes) but use original layout, colors, typography, and iconography.
- Do not ship Ableton-like assets or names.