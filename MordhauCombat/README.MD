# MordhauCombat

Copyright (C) 2025-2026 Mithras666.

The original source code for this mod is licensed under the GNU General Public
License, version 3 or (at your option) any later version. It is distributed
without any warranty, including the implied warranties of merchantability or
fitness for a particular purpose. See [LICENSE.txt](LICENSE.txt) for the full,
unmodified license text.

Source code: [https://github.com/mithras777/Skyrim-Mods/tree/main/MordhauCombat](https://github.com/mithras777/Skyrim-Mods/tree/main/MordhauCombat)

This project uses CommonLibSSE-NG revision
a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9. That dependency is licensed under
GPL-3.0-or-later with its Modding Exception and GPL-3.0 Linking Exception; the
pinned dependency's COPYING.txt and EXCEPTIONS.md are included in each
release's licenses/MordhauCombat/commonlib_ng/ directory. The lock file records exact
dependency revisions and source archive hashes.

Portions derived from the ArranzCNL SkyrimSE Plugin Template retain its MIT
license; see licenses/third-party/ArranzCNL-MIT-LICENSE.txt. Other third-party
components and bundled assets retain their applicable notices and terms under
licenses/ and the mod's Nexus page.

Build with CMake 3.24+, Visual Studio 2022, and the ng preset:

    cmake --preset ng
    cmake --build --preset ng-release

See MIGRATION.md for migration notes and runtime limitations.