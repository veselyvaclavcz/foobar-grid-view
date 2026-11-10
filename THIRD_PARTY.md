# Third-party components

## foobar2000 SDK (`SDK-2025-03-07/`)

The `SDK-2025-03-07/` folder contains the foobar2000 2.x SDK (release
2025-03-07) by Peter Pawlowski. It is **not** covered by this repository's
[MIT License](LICENSE); it is redistributed here unchanged under its own
license terms so that the component can be built without extra downloads.

| Part | License file | Terms |
| --- | --- | --- |
| foobar2000 SDK | [`SDK-2025-03-07/sdk-license.txt`](SDK-2025-03-07/sdk-license.txt) | BSD-style license, Copyright (c) 2002-2025 Peter Pawlowski |
| pfc | [`SDK-2025-03-07/pfc/pfc-license.txt`](SDK-2025-03-07/pfc/pfc-license.txt) | zlib-style license, Copyright (C) 2002-2024 Peter Pawlowski |
| libPPUI | [`SDK-2025-03-07/libPPUI/libPPUI-license.txt`](SDK-2025-03-07/libPPUI/libPPUI-license.txt) | zlib-style license, Copyright (C) 2002-2024 Peter Pawlowski |

The SDK license permits redistribution in source form as long as the copyright
notice, the list of conditions and the disclaimer are retained; the license
files above are kept in place for that reason. The pfc and libPPUI licenses
likewise require that their notices are not removed or altered.

Notes on this copy:

- The SDK source files have not been modified in this repository.
- `SDK-2025-03-07/BUILD_SDK_LIBS.sln` is **not** part of the original SDK. It
  was added by this project to build the SDK libraries the component links
  against (`pfc`, `foobar2000_SDK`, `shared`, `foobar2000_component_client`).
- Build output (object files, libraries, precompiled headers, logs) is not
  tracked; it is produced locally or in CI.
- The project name "foobar2000" and the SDK author's name are used only to
  identify the SDK; no endorsement by the author is implied.

Upstream: <https://www.foobar2000.org/SDK>
