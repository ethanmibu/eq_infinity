# EQInfinity

8-band JUCE EQ plugin project (EQ Eight–style workflow, original UI/branding) targeting:
- macOS: AU, VST3, Standalone
- Windows: VST3, Standalone

Current implementation includes Milestone 2 (APVTS parameter system) and Milestone 3 DSP foundations.

## Current Feature Set

- 8-band EQ with draggable nodes and bottom-strip per-band controls
- Node click auto-enables the band
- Node double-click resets the band to defaults
- Drag modifiers:
  - `Shift`: fine frequency/gain adjustment
  - `Option/Alt`: Q adjustment via vertical drag
  - `Command` (macOS) / `Ctrl` (Windows): hold for temporary band solo audition
- Global modes:
  - `Stereo` (default), `Mid/Side`, or `Left/Right`
  - Quality `Eco` or `HQ (2x)` oversampling
- Edit targeting:
  - `Link` (write both A/B banks)
  - side-specific `A` / `B` (labels adapt to `L/R` or `M/S` by mode)

## Prerequisites

- CMake 3.21+
- C++17 toolchain
- JUCE in `third_party/JUCE` (submodule expected)

### macOS
- Xcode + command line tools
- Ninja (recommended)
- clang-format (for formatting checks)

### Windows
- Visual Studio 2022 (Desktop development with C++)

## Clone + JUCE Setup

```bash
git clone <repo-url>
cd eq_infinity
git submodule update --init --recursive
```

## Build

### macOS

```bash
./scripts/setup_macos.sh
./scripts/configure.sh -G Ninja
./scripts/build.sh
```

### Windows (PowerShell)

```powershell
.\scripts\setup_windows.ps1
bash ./scripts/configure.sh -G "Visual Studio 17 2022" -A x64
bash ./scripts/build.sh --config Release
```

Artifacts are generated under `build/EQInfinity_artefacts/`.

## Test

```bash
./scripts/configure.sh -DBUILD_TESTING=ON
./scripts/build.sh --target eq_infinity_tests
ctest --test-dir build --output-on-failure
```

## Formatting

```bash
# apply formatting
./scripts/format.sh apply

# check formatting (non-mutating, CI-safe)
./scripts/format.sh check
```

## Key CMake Options

- `-DEQINF_COPY_PLUGIN_AFTER_BUILD=ON|OFF`
  - Controls whether built plugins are copied into system plugin directories.
- `-DEQINF_PLUGIN_FORMATS="VST3;Standalone"` (Windows default)
- `-DEQINF_PLUGIN_FORMATS="AU;VST3;Standalone"` (macOS default)
- `-DZL_JUCE_COPY_PLUGIN=TRUE|FALSE` (template-compatible alias)
- `-DZL_JUCE_FORMATS="..."` (template-compatible alias)

## Plugin Scan Paths

### macOS
- AU: `/Library/Audio/Plug-Ins/Components` and `~/Library/Audio/Plug-Ins/Components`
- VST3: `/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/VST3`

### Windows
- VST3: `C:\Program Files\Common Files\VST3`

## Troubleshooting

- Plugin not showing in host:
  - verify format was built via `EQINF_PLUGIN_FORMATS`
  - disable auto-copy (`EQINF_COPY_PLUGIN_AFTER_BUILD=OFF`) and install manually if needed
  - re-scan plugins in the host
- Build cannot find JUCE:
  - run `git submodule update --init --recursive`

## License

- This project is licensed under the [GNU Affero General Public License v3.0](https://www.gnu.org/licenses/agpl-3.0.html).
- See `LICENSE` for the full text.
- If you distribute or provide network access to modified versions, ensure AGPL-3.0 source-availability requirements are met.
- JUCE is a separate dependency with its own licensing terms.
