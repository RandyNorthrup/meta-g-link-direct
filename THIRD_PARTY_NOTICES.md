# Third-party source and development inputs

The project is licensed under GNU AGPL version 3. See `LICENSE`.

## Visor

- Upstream: https://github.com/macedonga/visor
- Revision: `e7077b4051b1e3e605c667aa2a010fbe41f346a0`
- License: AGPL-3.0; preserve upstream license and contributor attribution.
- Authors: upstream Visor contributors; see the pinned repository history.
- Ported implementation: `core/transport/src/main/kotlin/com/macedonga/visor/core/transport/Sdu.kt` to `src/protocol/sdu.cpp` and its declared interface.
- Changes: C++20 span-based prefix inspection, explicit incomplete/invalid/ready result, no allocation or exception on invalid input, independent C++ test vectors and boundary cases.
- Other Visor identity, streaming and Wi-Fi sources are currently research references only. No working Windows connection port is claimed.

## OBS Studio

- Upstream: https://github.com/obsproject/obs-studio
- Version/revision: `32.2.2` / `ba2f32bdf791005443988a4955e963663e16b1ed`
- OBS public module headers and import library are development/runtime inputs; preserve their original license notices. OBS code identifies GPL version 2 or later in its source notices.
- The plugin follows the documented module API. It does not copy a private ABI declaration or replace the installed OBS runtime.
- Source/dependency archives are downloaded to ignored `.deps/` and verified against pinned hashes. Their individual licenses remain with the original packages. The module ZIP does not redistribute that development dependency tree.

No Meta, Ray-Ban, Apple, or OBS logos are included. Product names identify compatibility targets and imply no endorsement. Public distribution licensing review remains outside this personal-prototype milestone.
