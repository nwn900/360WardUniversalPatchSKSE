# Changelog

## [1.3.0] - 2026-09-14

### Changed

- Preserve explicitly assigned custom casting and hit ArtObjects on `WardPower` effects.
- Continue forwarding empty ward visual slots and known vanilla/360 Ward ArtObjects.
- Restrict the native 360 Ward pulse to active ward effects that use known or forwarded ward art, leaving fully custom ward visuals to their owning mod.

### Verification

- Focused policy tests and the MSVC x64 Release build passed. In-game Skyrim runtime and gameplay behavior remain unverified.

## [1.2.0] - 2026-08-28

- Verified the Skyrim AE 1.7.104 Address Library mapping and build/static compatibility. An in-game 1.7.104 DLL-load or gameplay test has not been performed, so runtime behavior on that version remains unvalidated.
- Requires the matching external `versionlib-1-7-104-0.bin` Address Library file (format 5); it is not bundled with this plugin.
- Updated the CommonLibSSE-NG dependency baseline to v6.7.1. No plugin hook-behavior change; this release contains compatibility and documentation updates only.
