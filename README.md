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

Source code: https://github.com/nwn900/WardVisualForwarder

The GitHub release is a plain SKSE archive containing `SKSE/Plugins/360WardUniversalPatchSKSE.dll`. The local FOMOD staging files are not part of the release.

## Build

1. Clone the repository and initialize the CommonLibSSE-NG submodule: `git submodule update --init --recursive`.
2. Bootstrap `tools/vcpkg`.
3. Configure with the Visual Studio x64 generator and `tools/vcpkg/scripts/buildsystems/vcpkg.cmake`.
4. Build `360WardUniversalPatchSKSE` in Release.
5. Run the `360WardUniversalPatchSKSEPolicyTests` CTest test.

The release archive is created outside the game directory from the resulting DLL.
