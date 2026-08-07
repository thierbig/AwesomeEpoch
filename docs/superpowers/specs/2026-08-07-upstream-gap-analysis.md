# Upstream gap analysis: AwesomeEpoch vs `noname08662/awesome_wotlk`

**Date:** 2026-08-07
**Scope:** Compare this fork's `src/AwesomeWotlkLib` against the active upstream continuation
`noname08662/awesome_wotlk` (`main` branch, last push 2026-06-27). Seeds two follow-up plans:
a safe-wins port plan and a gated MSDF-fonts port plan.

## 0. Verified repo lineage

Checked live via `gh api`, not assumed:

| Repo | `archived` | `fork` | `parent` |
|---|---|---|---|
| `FrostAtom/awesome_wotlk` | false | false | — (original) |
| `someweirdhuman/awesome_wotlk` | **true** | false | — |
| `noname08662/awesome_wotlk` | false | **true** | `someweirdhuman/awesome_wotlk` |

Confirms the context given in the task: `someweirdhuman` is archived, `noname08662` is the
active fork-of-the-fork and the right upstream to diff against. `thierbig/AwesomeEpoch` (this
repo) reports `fork:false` on the GitHub API (detached fork relationship), but shared hardcoded
client addresses recovered below (`0x0098E9F9`, `0x0098EA27`, `0x00ADAA7C` in `NamePlates.cpp`)
confirm a genuinely common code ancestor and, more usefully, that the fork's target client
binary is address-compatible with upstream's at those offsets.

## 1. Method and what was actually fetched

`gh` was authenticated and reachable, so this is a **verified** diff, not an inference from the
task brief's hints. Fetched via `gh api`:

- Full upstream tree: `repos/noname08662/awesome_wotlk/git/trees/main?recursive=1` (4037 paths,
  with blob sizes).
- Full text of `docs/readme.md` (174 lines) and `docs/api_reference.md` (593 lines) — upstream's
  feature/CVar/API/event lists, used verbatim below rather than re-derived.
- Full text of: `Entry.cpp`, `Hooks.h`, `CombatLog.h`/`.cpp`, `Camera.h`/`.cpp`, `VFX.h`/`.cpp`,
  `ReTools.h`, `BugFixes.cpp`, `CommandLine.cpp`, `D3D.h` (declarations only), `Misc.cpp`,
  `NamePlates.cpp`/`.h`, `Types.h`/`Lua.h`/`Enums.h` (headers only), `deps/CMakeLists.txt`,
  `src/AwesomeWotlkLib/CMakeLists.txt`, `deps/skia` directory listing.
- Blob sizes for every file in `src/AwesomeWotlkLib` (upstream) and byte counts for every file
  in the fork's `src/AwesomeWotlkLib`, used to gauge port size.

Not fetched (inferred from the brief / not needed for classification): `MSDF*.cpp/.h` bodies,
`D3D.cpp` body, `deps/freetype-2.14.1` and `deps/msdfgen` internals, `deps/skia` binary
contents. These sit entirely in the **defer** bucket; their scope is characterized by file size,
CMake wiring, and the vendored deps involved, not by reading their full implementation, since
the fork classification does not change based on their internals.

Local commands run against this repo: `ls src/AwesomeWotlkLib`,
`grep -rn "registerCVar\|registerFunction\|registerEvent" src/AwesomeWotlkLib` (41 hits, no
`registerFunction` — Lua functions are exposed via `luaL_Reg` tables and `lua_pushcfunction`,
not a `registerFunction` call, see §2), `wc -c` on every fork lib file for size comparison.

## 2. Fork inventory (verified against this checkout)

`src/AwesomeWotlkLib/` (28 files): `AdvancedEvasion.{cpp,h}`, `AntiDetection.{cpp,h}`,
`BugFixes.{cpp,h}`, `CMakeLists.txt`, `CommandLine.{cpp,h}`, `Entry.cpp`, `EvasionLogger.{cpp,h}`,
`GameClient.h`, `Hooks.{cpp,h}`, `Inventory.{cpp,h}`, `Item.{cpp,h}`, `Misc.{cpp,h}`,
`NamePlates.{cpp,h}`, `Spell.{cpp,h}`, `UnitAPI.{cpp,h}`, `Utils.{cpp,h}`, `VoiceChat.{cpp,h}`.

`AntiDetection`, `AdvancedEvasion`, `EvasionLogger` are fork-only (no upstream equivalent) and
are load-bearing for the injection-based ban-avoidance model described in the fork's README —
any port work must not weaken these.

Registered CVars (`Hooks::FrameXML::registerCVar`, 26 total): nameplateDistance,
nameplateStacking, nameplateXSpace, nameplateYSpace, nameplateUpperBorder, nameplateOriginPos,
nameplateSpeedRaise, nameplateSpeedReset, nameplateSpeedLower, nameplateHitboxHeight/Width,
nameplateFriendlyHitboxHeight/Width, nameplateStackFriendly(+Mode), nameplateMaxRaiseDistance,
nameplateExtendWorldFrameHeight, nameplateUpperBorderOnlyBoss (all in `NamePlates.cpp`);
enableStancePatch (`Spell.cpp`); showPlayer, cameraFov, interactionAngle, interactionMode
(`Misc.cpp`); ttsVoice, ttsSpeed, ttsVolume (`VoiceChat.cpp`). Two more —
`cameraIndirectAlpha`/`cameraIndirectVisibility` — exist as **commented-out** registrations in
`Hooks.cpp` (dead code, not active).

Registered events (`Hooks::FrameXML::registerEvent`, 9 total): NAME_PLATE_CREATED,
NAME_PLATE_UNIT_ADDED, NAME_PLATE_UNIT_REMOVED, NAME_PLATE_OWNER_CHANGED,
VOICE_CHAT_TTS_PLAYBACK_{FAILED,FINISHED,STARTED}, VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE,
VOICE_CHAT_TTS_VOICES_UPDATE.

Lua API surface (via `luaL_Reg` tables, confirmed present and functionally matching upstream's
`docs/api_reference.md`): `UnitAPI.cpp` (UnitIsControlled/Disarmed/Silenced, UnitOccupations,
UnitOwner, UnitTokenFromGUID), `Inventory.cpp` (GetInventoryItemTransmog), `Item.cpp`
(GetItemInfoInstant), `Spell.cpp` (GetSpellBaseCooldown), `VoiceChat.cpp` (C_VoiceChat,
C_TTSSettings), `NamePlates.cpp` (C_NamePlate + Get/SetStackingEnabled,
Get/SetOcclusionEnabled), `Misc.cpp` (FlashWindow, IsWindowFocused, FocusWindow,
CopyToClipboard, QueueInteract).

**Conclusion: the fork already tracks upstream's "New API Functions" list from the README at
essentially 1:1 parity.** The real gap is in newer subsystems added after that sync point:
Camera, CombatLog, D3D+MSDF rendering, VFX, and an expanded NamePlates stacking/occlusion engine
— plus a handful of new CVars grafted onto `Misc.cpp`.

## 3. Upstream inventory (`src/AwesomeWotlkLib`, sizes from git blob metadata)

Files the fork **lacks entirely** (module name — files — byte size):

| Module | Files | Size |
|---|---|---|
| Camera | `Camera.cpp` (7885B), `Camera.h` (53B) | ~7.9 KB |
| CombatLog | `CombatLog.cpp` (3381B), `CombatLog.h` (56B) | ~3.4 KB |
| D3D | `D3D.cpp` (21820B), `D3D.h` (10828B) | ~32.6 KB |
| VFX | `VFX.cpp` (53B), `VFX.h` (50B) | ~0.1 KB (empty stub, "WIP") |
| MSDF | `MSDF.{cpp,h}` (20915+2218B) | ~23.1 KB |
| MSDFCache | `MSDFCache.{cpp,h}` (17767+5018B) | ~22.8 KB |
| MSDFFont | `MSDFFont.{cpp,h}` (10079+1799B) | ~11.9 KB |
| MSDFManager | `MSDFManager.{cpp,h}` (9205+3072B) | ~12.3 KB |
| MSDFPregen | `MSDFPregen.{cpp,h}` (11222+2522B) | ~13.7 KB |
| MSDFShaders | `MSDFShaders.h` (2645B) | ~2.6 KB |
| MSDFUtils | `MSDFUtils.h` (5663B) | ~5.7 KB |
| MSDFValidator | `MSDFValidator.h` (9483B) | ~9.3 KB |
| Support headers | `Enums.h` (50830B), `Types.h` (11534B), `Lua.h` (12809B), `ReTools.h` (430B) | ~74.6 KB |

`GameClient.h` upstream is 72836B vs the fork's 72498B — a ~340-byte difference, i.e.
**essentially the same file**. This confirms `Enums.h`/`Types.h`/`Lua.h` are net-new
infrastructure added *alongside* `GameClient.h`, not a refactor extracted out of it — lowering
the risk that vendoring them breaks existing fork code.

Vendored third-party deps upstream that the fork does not have:

| Dep | Upstream mechanism | Notes |
|---|---|---|
| `deps/freetype-2.14.1` | Built from source, `add_subdirectory`, several `FT_DISABLE_*` flags off (zlib/png/bzip2/brotli/harfbuzz) | Standard CMake C library, no odd toolchain needs |
| `deps/msdfgen` | Built from source, `add_subdirectory`, `MSDFGEN_USE_SKIA=ON` | Links against Skia for path rendering |
| `deps/skia` | **Not built from source** — vendors `include/` headers plus a prebuilt `skia.lib` + `skia.dll` binary (~475 files, mostly headers) | Opaque prebuilt binary; smaller build burden than a from-source Skia but a supply-chain/trust concern and a `skia.dll` that must ship alongside the mod |
| `deps/unordered_dense` | Header-only, `INTERFACE` CMake target aliased `ankerl::unordered_dense` | Trivial — MIT-style header-only hash map/set, used by `Camera.cpp` and the expanded `NamePlates.cpp` |

Existing shared dep `Detours` — already vendored in the fork, no action needed.

Modules present in **both** with upstream extending them (not full modules, deltas only):

- **`NamePlates.cpp`**: upstream 53729B vs fork 29959B (~24KB larger). Adds 24 new nameplate
  CVars beyond the fork's existing 16 (mouse-mode click-through, inertia/hysteresis-based
  stacking physics, band overlap tolerances, per-side hitbox anchor/scale, raise/lower/pull
  speed and distance, screen-edge clamping, line-of-sight occlusion alpha with two modes,
  non-target alpha, alpha transition speed, placement offset). Full list and semantics are in
  upstream's `docs/api_reference.md` §C_NamePlates (fetched, see §5 sources). Implemented via 4
  `__declspec(naked)` asm hooks touching 16 hardcoded addresses; **3 of those addresses
  (`0x0098E9F9`, `0x0098EA27`, `0x00ADAA7C`) are already present, byte-identical, in the fork's
  own `NamePlates.cpp`** — strong (not certain) evidence the fork's target binary is
  address-compatible with upstream's remaining 13 new addresses.
- **`Misc.cpp`**: upstream 15583B vs fork 10865B. Adds 4 new CVars (`objectHighlightMode`,
  `portraitResolution`, `chatLogSessionKey`, `combatLogSessionKey`) and macro-conditional
  keyword support (`cursor`, `playerlocation`) via a hook on the client's
  `CGGameUI::SecureCmdOptionParseFn`. Uses 11 new hardcoded addresses, all disjoint from the
  fork's single existing address in this file (`0x00607C20`). Upstream's `Misc.cpp` has an
  `#include "D3D.h"` but **does not actually reference any D3D9 symbol in the file body**
  (verified by grep for `IDirect3D`/`Callback`/`GetDevice` — zero hits) — the include appears
  vestigial, so these 4 CVars are *not* actually gated on the D3D module despite the include.
- **`BugFixes.cpp`**: **not a gap** — the fork already has the clipboard-fix hook
  (`Clipboard_GetString`/`SetString` around `0x008726F0`/`0x008727E0`), matching upstream's
  intent, but its `BugFixes::initialize()` body is fully commented out (disabled, "not required
  for NamePlate API") and the call is never invoked from `Entry.cpp` either. Also: the fork's
  `alloc()` call for the clipboard buffer omits the `+1` for the null terminator that upstream's
  version has (`alloc(str.size())` vs upstream's `alloc(str.size() + 1)` with an explicit
  `memcpy(..., str.size()+1)`); worth fixing if/when re-enabled.
- **`CommandLine.cpp`**: upstream is a near-drop-in stylistic modernization (structured
  bindings, a `setCVarFromParam` helper) with the **same** feature set (`-login`/`-password`,
  `-realmlist`/`-realmname`, `-character`) — not a feature gap. **Bonus finding**: diffing it
  surfaced a live bug in the fork's version. In `gluexml_postload()`, the `realmlist` branch
  correctly guards `cvar && realmList` before calling `Console::SetCVarValue`, but the
  `realmname` branch only guards `cvar` (missing `&& realmname`) — so on any launch *without*
  `-realmname` passed, `SetCVarValue` is called with a NULL value pointer. Upstream's refactor
  (which routes both through the same guarded lambda) doesn't have this asymmetry. Trivial,
  low-risk fix (`if (Console::CVar* cvar = Console::FindCVar("realmName"); cvar && realmname)`),
  independent of the port work but worth folding into the safe-wins plan since it was found
  during this diff.

New CVars from upstream's README/API-reference not covered above: `MSDFMode` (gates the MSDF
pipeline — defer bucket only), `ttsVoice`/`ttsSpeed`/`ttsVolume` (already in fork),
`cameraFov`/`showPlayer`/`interactionMode`/`interactionAngle` (already in fork, see caveat in
§4 Camera row about a CVar-name collision).

## 4. Infrastructure divergences that affect every port item

- **`Hooks::FrameXML::registerCVar` signature has drifted.** Fork:
  `registerCVar(Console::CVar** dst, const char* str, const char* desc, Console::CVarFlags flags, const char* initialValue, Console::CVar::Handler_t func)`
  — 6 required args, `flags` positioned 4th. Upstream:
  `registerCVar(CVar** dst, const char* str, const char* desc, const char* initialValue, CVar::Handler_t func, CVar::CVarFlags flags = (CVarFlags)1, const std::function<void(CVar*)>& initCallback = nullptr)`
  — `flags` now optional and moved after `func`, plus a new optional `initCallback`. Every
  upstream module that registers a CVar (Camera, expanded NamePlates, expanded Misc) is written
  against the new signature. Porting requires either updating the fork's `Hooks.h`/`Hooks.cpp`
  to the newer signature (touches a shared header every existing module includes) or
  hand-adapting every ported call site to the fork's older positional form. This is the single
  highest-leverage piece of shared-hook risk in the whole gap.
- **Lua calling convention differs.** Upstream wraps the raw Lua C API behind a `Lua::` namespace
  (`Lua::lua_pushnumber`, `Lua::lua_setglobal`, etc., declared in the new `Lua.h`); the fork
  calls the raw (non-namespaced) functions directly (confirmed in fork's `Entry.cpp`: bare
  `lua_pushnumber`/`lua_setglobal`, no `Lua::` prefix, and no `Lua.h` in the tree). Any upstream
  module body copied verbatim (Camera's `Lua::lua_pushnumber`, Misc's `Lua::lua_gettop`, etc. in
  the macro-conditional hook) needs either `Lua.h` vendored or its `Lua::` prefixes stripped.
- **`Hooks::registerLuaLib` takes `const Lua::lua_CFunction&`** upstream vs the fork's raw
  `lua_CFunction` — same class of issue, smaller blast radius.
- **`Entry.cpp` attach sequence is far more elaborate in the fork than upstream.** Upstream's
  `OnAttach()` is a single `DetourTransactionBegin/…/Commit` block calling every module's
  `initialize()` in a flat list, then registers the Lua lib. The fork's `Entry.cpp` instead
  splits initialization across `AttachThread`/`DelayedInitThread`, environment-safety checks
  (`IsEnvironmentSafe`, VM/sandbox/anti-cheat detection), extensive `EVASION_LOG_*`
  instrumentation, and `__try/__except` guards — with several existing modules'
  `initialize()` calls already commented out ("disabled: not required for NamePlate API").
  Any new module's `initialize()` call must be placed deliberately in this sequence (which
  `OnAttach` vs `OnRealAttach` phase, before or after `Hooks::ensureCustomCVarsRegistered()`)
  rather than dropped in blindly — this is the concrete "Entry.cpp wiring" risk the task brief
  calls out, and it applies to literally every new module, safe or not.

## 5. Classified gap table

Legend: **safe** = self-contained, base-3.3.5a-compatible, no anti-detection/injector coupling,
one `initialize()` call to wire in. **conditional** = portable but needs care (CVar/name
collisions, shared-hook signature work, hardcoded-address verification, or thoughtful
`Entry.cpp` placement). **defer** = large/invasive MSDF font stack.

| # | Item | Upstream source | Bucket | Est. cost | Risk notes |
|---|---|---|---|---|---|
| 1 | CombatLog module — per-session combat-log filename via `CreateFileA/W` hook | `CombatLog.{cpp,h}` (~3.4KB) | **safe** | ~0.5 day | Pure Detours hook, no CVar, no address dependency beyond the two `CreateFileA`/`CreateFileW` symbols themselves (resolved dynamically, not hardcoded offsets). One `initialize()` call. |
| 2 | VFX module | `VFX.{cpp,h}` | **safe** | trivial (~5 min) | Upstream body is a no-op `// WIP` stub. Port for parity/forward-compat; zero functional risk, zero present value. |
| 3 | `ReTools.h` support header | `ReTools.h` (430B) | **safe** | trivial | Inert no-op `ToolkitManager` class, referenced only in `#ifdef _DEBUG`-adjacent dead paths upstream. No behavior change. |
| 4 | `unordered_dense` dependency (header-only) | `deps/unordered_dense`, `deps/CMakeLists.txt` `ankerl::unordered_dense` target | **safe** | ~1 hr | Header-only MIT-style lib, `INTERFACE` CMake target, no build step. Prerequisite for items 6 and 7 below. |
| 5 | `Enums.h` support header | `Enums.h` (50830B) | **safe** | ~1 hr to vendor + verify no macro/enum name collisions against fork's `GameClient.h` | Purely additive; `GameClient.h` is ~99.5% size-identical fork vs upstream, so this is new content, not an extraction. |
| 6 | Bugfix (not a port item): `CommandLine.cpp` `realmname` null-guard | Found via diff against upstream's `CommandLine.cpp` | **safe** | trivial (1-line fix) | Missing `&& realmname` guard before `Console::SetCVarValue` call; NULL value passed on any launch without `-realmname`. Independent of upstream port, bundle into the safe-wins plan. |
| 7 | `Lua.h` + `Types.h` support headers (calling-convention decision) | `Lua.h` (12809B), `Types.h` (11534B) | **conditional** | ~1 day (decision + spot-port) | Not risky in isolation, but adopting them is a project-wide convention change (`Lua::`-prefixed wrappers) that every subsequent module port (Camera, NamePlates delta, Misc delta) is written against. Decide once: vendor `Lua.h`/`Types.h` and use the wrapper convention going forward, or strip `Lua::` prefixes at each port site. Recommend vendoring — cheaper than repeated manual stripping. |
| 8 | `Hooks::FrameXML::registerCVar` / `registerLuaLib` signature reconciliation | `Hooks.h`/`Hooks.cpp` (fork's, needs editing) | **conditional** | ~0.5–1 day + regression pass on all 26 existing fork CVars | Shared header every module includes. Safest path: add an overload matching upstream's new signature (optional `flags`/`initCallback`, reordered params) alongside the fork's existing one, rather than changing the existing call sites' behavior. Must re-verify all 26 existing CVar registrations still compile/behave after the header edit. |
| 9 | Camera module — camera FOV/indirect-visibility fade for occluding models | `Camera.{cpp,h}` (~7.9KB) | **conditional** | ~1–2 days | Two blocking issues: (a) **CVar name collision** — upstream's `Camera::initialize()` registers `showPlayer` and `cameraFov`, both of which the fork **already registers** in `Misc.cpp`; naively adding both would double-register the same CVar names. Must merge into the fork's existing `Misc.cpp` handlers instead of copying the file wholesale, or drop the duplicate registrations and keep only the new `cameraIndirectAlpha`/`cameraIndirectVisibility` pair (which the fork already has as **dead, commented-out** registrations in `Hooks.cpp` — reviving those is the actual net-new work here). (b) 3 `__declspec(naked)` asm hooks at hardcoded addresses (`0x004F90E2` etc., none overlapping the fork's confirmed-compatible `NamePlates.cpp` addresses) — needs verification against the fork's actual target binary before shipping. Depends on item 4 (`unordered_dense`). |
| 10 | NamePlates advanced stacking/occlusion/clamp engine (24 new CVars) | `NamePlates.cpp` delta (~24KB of the 53.7KB file) | **conditional** | ~3–5 days | Highest-value port item (this *is* upstream's headline "Improved Nameplate Sorting" feature and the fork's core differentiator). Must be merged into the fork's own already-diverged `NamePlates.cpp` (fork is 30KB vs upstream 53.7KB — not a clean overwrite target; fork has its own CVar set already, e.g. `nameplateXSpace`/`nameplateYSpace`/`nameplateStackFriendly*` with no upstream equivalent, that must be preserved). 4 naked-asm hooks across 16 hardcoded addresses; 3 addresses independently verified identical to the fork's current build (see §3), giving reasonable but not certain confidence the other 13 are address-compatible too — recommend a debug build with address logging/breakpoints before trusting them blind. Depends on items 4 and 8. |
| 11 | `Misc.cpp` new CVars: `objectHighlightMode`, `portraitResolution` + macro conditionals `cursor`/`playerlocation` | `Misc.cpp` delta (~4.7KB of the 15.6KB file) | **conditional** | ~1–2 days | 11 new hardcoded addresses, all disjoint from the fork's one existing address in this file. The macro-conditional feature hooks `CGGameUI::SecureCmdOptionParseFn` — a client-shared macro-parsing entry point, so verify it doesn't collide with any other macro-conditional patch the fork may add later. Confirmed *not* actually dependent on the D3D module despite a vestigial `#include "D3D.h"` in upstream's file (no D3D9 symbols referenced in the body). Depends on item 8. |
| 12 | `chatLogSessionKey`/`combatLogSessionKey` CVars (per-launch chat/combat log filenames via CVar-gated pointer patch) | `Misc.cpp` delta | **conditional** | ~0.5 day, folded into item 11's estimate | Distinct mechanism from item 1 (`CombatLog.cpp`'s `CreateFile` hook) — this patches a literal data pointer in the binary (`0x00AC7A40`/`0x00AC7A44`) rather than hooking the file-open call. Both upstream features target the same "isolate logs per session" goal via different mechanisms (chat log path + combat log path pointer patch here, vs. redirecting *any* `WoWCombatLog.txt` open in item 1). Recommend porting item 1 (`CombatLog.cpp`) alone first as the safer of the two combat-log mechanisms, and treating this pointer-patch CVar pair as optional/lower-priority. |
| 13 | D3D module — Direct3D9 vtable hook (Present/BeginScene/EndScene/DrawPrimitive(Indexed)/SetTexture/SetRenderState/CreateTexture/SetRenderTarget/Clear/Reset) | `D3D.{cpp,h}` (~32.6KB) | **defer** | part of MSDF-fonts plan | Foundational render hook the entire MSDF pipeline sits on. No standalone value without MSDF; scope it together. |
| 14 | MSDF font-rendering pipeline (8 modules: MSDF, MSDFCache, MSDFFont, MSDFManager, MSDFPregen, MSDFShaders, MSDFUtils, MSDFValidator) | `MSDF*.{cpp,h}` (~101.4KB combined) | **defer** | part of MSDF-fonts plan | Vector glyph-atlas rendering, font blacklisting via `Fonts_AwesomeWotLK` folder convention, `MSDFMode` CVar (0/1/2). Largest single chunk of upstream code not in the fork. |
| 15 | `deps/freetype-2.14.1` (vendor + build from source) | `deps/freetype-2.14.1/` | **defer** | part of MSDF-fonts plan | Standard CMake C lib; low technical risk in isolation but adds real build time and a large vendored tree. |
| 16 | `deps/msdfgen` (vendor + build from source, `MSDFGEN_USE_SKIA=ON`) | `deps/msdfgen/` | **defer** | part of MSDF-fonts plan | Built from source; hard-links to Skia (item 17). |
| 17 | `deps/skia` (vendor headers + prebuilt `skia.lib`/`skia.dll`, not built from source) | `deps/skia/` (~475 files, mostly headers under `include/`) | **defer** | part of MSDF-fonts plan | Not a from-source build — upstream ships a prebuilt static lib + a runtime DLL. Lowers build-time cost vs. building Skia from source, but is an opaque prebuilt binary dependency (supply-chain trust, and `skia.dll` must be redistributed alongside `AwesomeWotlkLib.dll`). |
| 18 | CMake wiring for items 13–17 | `deps/CMakeLists.txt`, `src/AwesomeWotlkLib/CMakeLists.txt` | **defer** | part of MSDF-fonts plan | New `target_include_directories` (`deps/skia/include`, `deps/msdfgen`, build-dir `deps/msdfgen`) and `target_link_libraries` (`msdfgen::msdfgen-core`, `msdfgen::msdfgen-ext`, `freetype`, the literal `deps/skia/skia.lib` path, `ankerl::unordered_dense`). Non-trivial CMake surgery against the fork's current, much simpler lib `CMakeLists.txt`. |

**Counts:** 18 classified entries — **6 safe**, **7 conditional**, **5 defer** (items 13–18 above
collapse to 5 file-system-level defer entries: D3D, MSDF-family, freetype, msdfgen, skia+CMake
wiring, counting the CMake wiring together with skia since they're inseparable).

## 6. Recommendations for the follow-up plans

- **Safe-wins port plan**: order items 1–6 first (all safe, ~1–2 days combined including the
  `CommandLine.cpp` bugfix), then items 7–12 (conditional) once item 8's `Hooks.h` signature
  work lands, since items 9–12 all depend on it. Item 10 (NamePlates advanced stacking) is the
  highest-value, highest-effort conditional item and should get its own task with a dedicated
  manual-verification pass (in-game `/console nameplateStacking <mode>` sweep across the new
  modes) given it's the fork's core differentiator.
- **MSDF-fonts port plan**: items 13–18 as already scoped by the task brief. Given skia ships as
  a prebuilt binary rather than building from source, the CMake/build-time burden is smaller
  than "vendor and build three large C++ libraries" might suggest — the real cost is wiring
  `D3D.cpp`'s Direct3D9 hook safely into the fork's existing render/attach path and validating
  the MSDF pipeline doesn't regress FPS or trip anti-cheat on the fork's actual target client.
  Recommend the gate stated in the brief (Win32 Release still builds, DLL still injects cleanly)
  plus a render-hook-specific check: confirm `D3D::initialize()`'s vtable hook coexists with any
  present/existing hook the fork's Hooks/AntiDetection layer already installs on the D3D9
  device, since that's the one shared-hook surface item 13 introduces that nothing else in this
  report touches.
