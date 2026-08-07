# AwesomeEpoch — Base-Client Retarget, Nameplate Distance, Configurable Exe

**Date:** 2026-08-07
**Status:** Approved design (pending written-spec review)
**Author:** thierbig (with Claude)

## Context

`AwesomeEpoch` is a fork of `someweirdhuman/awesome_wotlk`, retargeted to run against a
WoW 3.3.5a client via DLL injection (plus a patcher path and anti-detection code). The
original upstream `someweirdhuman/awesome_wotlk` is now **archived** and points to
`noname08662/awesome_wotlk` as the continuation. `FrostAtom/awesome_wotlk` is the original
canonical repo.

The game client this fork now targets is the **base World of Warcraft 3.3.5a (build 12340)
client**, whose executable is named `Wow.exe` — not `Ascension.exe` as the code and README
currently assume.

This design covers three user-requested changes plus one correction discovered during
exploration:

1. **Configurable game executable name/path** via a plain-text file (`gameExeLocation.txt`),
   defaulting to `Wow.exe`.
2. **Re-enable nameplate distance**, which was previously disabled.
3. **Bring the fork "up to par" with upstream** `noname08662/awesome_wotlk` via a selective
   feature port, explicitly including the MSDF vector-font system.
4. **(Correction)** Reconcile the README with reality — see below.

### Discovered discrepancy (must-fix)

The current README advertises an `AwesomeEpoch.exe` launcher and a `gameExeLocation.txt`
config file. **Neither exists in this repository.** The actual binaries
(`AwesomeWotlkInjector`, `AwesomeWotlkPatch`) read nothing from disk and use hardcoded
executable names. Therefore "make the exe name changeable in a .txt" is a **new
implementation**, not a fix to an existing feature. The README will be corrected to describe
what the code actually does. No new launcher binary will be built (out of scope / YAGNI).

## Goals

- The injector and patcher both locate the game executable using a name/path read from
  `gameExeLocation.txt`, falling back to sensible built-in defaults (with `Wow.exe` first).
- The `nameplateDistance` CVar works on the base 3.3.5a client.
- The fork gains upstream features that are safe to port to the base client, including a
  best-effort MSDF vector-font port.
- README and docs accurately reflect the shipped binaries and behavior.

## Non-Goals

- Building a new `AwesomeEpoch.exe` orchestration/launcher binary.
- Changing the anti-detection / evasion subsystems (`AntiDetection`, `AdvancedEvasion`,
  `EvasionLogger`) except where a ported feature strictly requires it.
- A full tree merge of upstream. Ports are selective and reviewed feature-by-feature.
- Server-side behavior. Nameplate distance is a client-side render setting only.

---

## Part 1 — Configurable game executable (`gameExeLocation.txt`)

### Config file format

A plain-text file named `gameExeLocation.txt` located **next to the running binary**. Each
binary reads the copy in its own directory; when the injector and patcher are placed in the
same folder, one file serves both. (Note: the current release workflow packages only the
patcher + DLL; the injector is built by a separate workflow. See Release packaging below.)

- Lines beginning with `#` are comments and ignored.
- Blank lines are ignored.
- The **first** remaining line is the value; leading/trailing whitespace and surrounding
  quotes are trimmed.
- If the file is missing, empty, or fully commented, built-in defaults are used.

The value may be any of:
- a **full path** to the executable (e.g. `E:\Games\wow\Wow.exe`),
- a **directory** containing the executable (e.g. `E:\Games\wow`),
- a **bare filename** (e.g. `Wow.exe`).

A commented template is shipped in the release zip, e.g.:

```
# Set the path, folder, or filename of your game client executable.
# Remove the leading '#' on the next line and edit it. Leave commented to use defaults.
# Wow.exe
```

### Shared reader

Add a small **header-only** helper `src/Common/GameExeConfig.h`, included by both the
patcher and the injector targets (via `target_include_directories` pointing at `src/Common`,
or a relative include). It provides:

- A function to read and parse `gameExeLocation.txt` from the directory of the current
  module, returning the trimmed value (empty if none). Narrow (`std::string`) and wide
  (`std::wstring`) variants, since the patcher is ANSI and the injector is Unicode.
- A helper to extract the **basename** from a value that may be a full path.

Keeping it header-only avoids new CMake link targets and matches the codebase's low-
abstraction style. If a `.cpp` proves cleaner, an `INTERFACE`/`OBJECT` library under
`src/Common` is an acceptable alternative — decided at implementation time.

### Built-in default order (both binaries)

1. `Wow.exe`
2. `Project-Epoch.exe` (patcher, file form) / `Project Epoch.exe` (injector, process form)
3. `Ascension.exe` (legacy, last)

### Patcher changes (`src/AwesomeWotlkPatch/Main.cpp`)

- `findGameClientExecutable()` builds a candidate list = **config-derived candidates first,
  then built-in defaults**, and returns the first that exists as a regular file:
  - config value is an existing file → use it directly;
  - config value is an existing directory → probe the default exe names inside it;
  - config value is a bare name → treat as a filename relative to the working directory.
- The `Ascension.exe`-specific patch set (`s_patches_ascension`) is currently selected by
  exact string match on `path`. Change this to match on the **basename** so a full path to
  `Ascension.exe` still triggers the Ascension patch set; base `Wow.exe` uses the default
  `s_patches`.

### Injector changes (`src/AwesomeWotlkInjector/Injector.cpp`)

- Before the existing default process-name search, read `gameExeLocation.txt`, extract the
  **basename** of the value, and, if non-empty, use it as the **first** process-name
  candidate. Then fall back to the existing (obfuscated) defaults.
- The existing command-line override (`argv[1]`) continues to take precedence over the
  config file.

### Trade-off (accepted)

The injector currently XOR-obfuscates its default process names as an anti-detection
measure. A user-editable `gameExeLocation.txt` necessarily stores the configured name in
**plaintext** on disk. This is inherent to the feature and accepted per the user's request;
the built-in defaults remain obfuscated in the binary.

### Release packaging

Update `.github/workflows/windows-release.yml` to include the commented
`gameExeLocation.txt` template in the release zip alongside the shipped binaries.

---

## Part 2 — Re-enable nameplate distance

### Current state

`CVarHandler_NameplateDistance` in `src/AwesomeWotlkLib/NamePlates.cpp` has its body fully
commented out and returns `1` (no-op). The CVar is registered with a default of `"43"`. The
README states nameplate-distance changes are "disabled as it's considered cheating."

### Change

The hardcoded address in the commented code (`*(float*)0x00ADAA7C = f*f`) is confirmed to be
the **correct** address for the base 3.3.5a (12340) client — upstream `noname08662` still
uses this exact address. Restore the handler and adopt upstream's refinements:

- Parse the incoming value as a float.
- Clamp to a sane base-client range (41.0–100.0). A non-positive/invalid value falls back
  to the default (41.0).
- Write `f * f` to `*(float*)0x00ADAA7C`.
- If the fork exposes an equivalent world-frame accessor, set the world-frame dirty flag so
  the change refreshes immediately (as upstream does). If no such accessor exists in the
  fork, the raw write is sufficient and the change applies on the next natural refresh; this
  is documented rather than forced.
- Change the registered CVar default from `"43"` to `"41"` (the base-client default).

### Docs

Update the README: remove the "changing nameplate distance is disabled as it's considered
cheating" note and the "Nameplate Distance Limitation" note; state that `nameplateDistance`
is enabled on the base 3.3.5a client. The existing ban warning at the top of the README
remains unchanged.

---

## Part 3 — Selective upstream port ("up to par")

### Approach

The fork has diverged from upstream substantially (it added the injector, anti-detection,
and evasion subsystems). A full merge is high-risk. Instead:

1. **Gap analysis (first task).** Produce a concrete written report comparing the fork's
   `src/AwesomeWotlkLib` against `noname08662/awesome_wotlk`: new CVars, Lua APIs, events,
   bugfixes, and modules the fork lacks. Upstream modules absent from the fork include at
   least: `Camera`, `CombatLog`, `D3D`, `VFX`, the `MSDF*` family, and support headers
   (`Enums.h`, `Lua.h`, `ReTools.h`, `Types.h`). The report classifies each item as
   **safe / conditional / defer** for the base client.
2. **Port safe wins.** Port self-contained features that (a) target base 3.3.5a, (b) do not
   touch the anti-detection/injector code, and (c) compile cleanly under the existing Win32
   MSVC/Clang toolchain. Each ported feature is a discrete, independently reviewable change.
3. **Preserve fork-only code.** Anti-detection, evasion, and the injector remain intact.

### MSDF vector fonts (gated sub-phase)

The user opted in to the MSDF font port. Scope reality: MSDF pulls in **two large vendored
dependencies** — `deps/freetype-2.14.1/` and `deps/msdfgen/` — a **Direct3D hook module**
(`D3D.cpp/h`), and ~9 `MSDF*` source files, plus CMake wiring under `deps/`. This is
effectively its own sub-project.

It is specified as a **gated phase** with an explicit fallback:

- Bring in `deps/freetype-2.14.1` and `deps/msdfgen` with their `CMakeLists.txt`, wire them
  into the `deps` build, and port the `MSDF*` + `D3D` modules and their `Entry.cpp` hooks.
- **Gate:** the Win32 Release build must still succeed and the DLL must still inject and run
  without regressing existing features (nameplates, injector, anti-detection).
- **Fallback:** if MSDF cannot be integrated without destabilizing the build or conflicting
  with the anti-detection hooks within this effort, it **graduates into its own follow-up
  spec** rather than blocking Parts 1, 2, and the safe ports. This keeps the three core
  deliverables shippable on their own.

### Decomposition note

Parts 1 and 2 and the "safe wins" of Part 3 form a coherent, shippable unit. The MSDF
sub-phase is large enough that it may become a separate spec/plan; that decision is made
during the gap analysis once the true integration cost is measured against the current
build.

---

## Testing & Verification

This is a Windows-only, injection-based C++ project; there is no existing automated test
suite, and behavior is validated against a live client. Verification is therefore
build- and manual-runtime-based:

- **Build:** the CMake Win32 Release build (matching `windows-release.yml`) compiles all
  targets — `AwesomeWotlkLib.dll`, `AwesomeWotlkPatch.exe`, `AwesomeWotlkInjector.exe` —
  after each part.
- **Config (Part 1):** unit-test the parser logic where feasible (comment/blank handling,
  path vs folder vs bare name, quote/whitespace trimming) as a small host-compilable check;
  manually verify the injector attaches to `Wow.exe` and honors a custom path in the txt.
- **Nameplate distance (Part 2):** manual in-client check that changing `nameplateDistance`
  visibly changes render distance and that invalid values fall back safely without crashing.
- **Port (Part 3):** each ported feature builds and does not regress existing features; MSDF
  passes the gate defined above or is deferred.

## Risks

- **Wrong offset / client build:** `0x00ADAA7C` is validated for base 3.3.5a 12340 only. If
  the actual client differs, the write must be re-verified; a bad write could crash the
  client. Mitigation: clamp + validated-address assumption documented; verify against the
  real client during implementation.
- **MSDF integration cost/regression:** largest risk; mitigated by the gate + follow-up-spec
  fallback above.
- **Anti-detection interaction:** ported hooks (especially D3D) may interact with existing
  evasion hooks. Mitigation: keep ports additive and behind the build/runtime gate.
- **Plaintext exe name** slightly weakens injector obfuscation (accepted, see Part 1).

## Out of Scope

- New launcher binary, anti-detection changes, full upstream merge, server-side behavior,
  and any addon rewrite beyond what a re-enabled CVar requires.
