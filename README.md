# 360WardUniversalPatchSKSE

This CommonLibSSE-NG SKSE plugin scans the final loaded `EffectSetting` records at `kDataLoaded` and forwards ward-related art fields to the vanilla ward ArtObjects used by 360 Ward:

- `Casting Art` -> `Skyrim.esm|0253F1` (`WardInHandFX`)
- `Hit Effect Art` -> `Skyrim.esm|018124` (`WardHitEffect`)
- ward-related `Enchant Effect Art` -> `Skyrim.esm|018124`

The scan is load-order independent. It identifies ward MGEFs by `WardPower` and also catches records that still reference the known vanilla/360 Ward ArtObjects. It does not edit ESP/ESM files, the Skyrim installation, a save, or an MO2 profile.

## Native pulse reproduction

The DLL also reproduces the relevant `SphereWard` behavior in native code. It hooks CommonLibSSE-NG's concrete Papyrus VM `SendEvent` vtable entry using the documented SE/AE and VR indices, observes actor-level `OnWardHit` and `OnHit` dispatch, extracts the first `ObjectReference` argument, and calls the 360 Ward pulse ArtObject with the equivalent facing-target semantics.

The pulse is queued through SKSE's main-thread task interface before applying the ArtObject. If an active matching ward already has the exact `SphereWard` script attached, the DLL defers to that script so it does not double-play the pulse. Other matching wards receive the native pulse without VMAD injection. This reproduces the script's visual behavior; it does not add Papyrus properties or scripts to records.

The compiled binary is built with the live CommonLibSSE-NG source tree with Skyrim SE, AE, and VR targets enabled in one DLL. Address Library for the target runtime and SKSE/VR SKSE remain end-user requirements.

### Runtime support

The Skyrim AE 1.7.104 Address Library mapping and this source's build/static compatibility have been verified. An in-game Skyrim AE 1.7.104 DLL-load or gameplay test has not been performed, so runtime behavior on that version remains unvalidated. The build continues to target SE 1.5.x, AE, and VR.

Skyrim AE 1.7.104 requires the matching external Address Library file `versionlib-1-7-104-0.bin` (format 5). It is not bundled with this plugin; CommonLibSSE-NG 6.7.1+ resolves it at load time, so no plugin rebuild is needed when the required Address Library file is installed.

The GitHub release ZIP root contains `SKSE/Plugins/360WardUniversalPatchSKSE.dll`. Address Library remains an external requirement and is not included in the archive.

## Build

Use a VS 2022 Developer PowerShell with the MSVC environment loaded. Keep vcpkg outside this repository, bootstrap it there, and expose that checkout through `VCPKG_ROOT`.

From the external vcpkg checkout, run:

```powershell
.\bootstrap-vcpkg.bat
$env:VCPKG_ROOT = (Get-Location).Path
```

Then, from the repository root after cloning and initializing the CommonLibSSE-NG submodule (`git submodule update --init --recursive`), run:

```powershell
cmake --preset release -DBUILD_TESTING=ON
cmake --build build\release --target 360WardUniversalPatchSKSE
cmake --build build\release --target 360WardUniversalPatchSKSEPolicyTests
ctest --test-dir build\release --output-on-failure
```

The `release` configure preset uses Ninja and the external `$env{VCPKG_ROOT}` toolchain. There is no separate build preset; the build commands use the generated `build\release` tree. `BUILD_TESTING=ON` enables this project's policy test. CommonLibSSE-NG's own `BUILD_TESTS` remains `OFF`; do not confuse it with `BUILD_TESTING`.

The release archive is created outside the game directory from the resulting DLL.
