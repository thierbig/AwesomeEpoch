# AwesomeEpoch Base-Client Retarget Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Retarget AwesomeEpoch to the base WoW 3.3.5a (12340) client — make the game
executable name/path configurable via `gameExeLocation.txt` (default `Wow.exe`), re-enable
the `nameplateDistance` CVar, correct the README, and produce the upstream gap-analysis that
seeds the feature-port follow-up plans.

**Architecture:** A single header-only helper (`src/Common/GameExeConfig.h`) parses the
config file; the patcher and injector both include it to build their executable-candidate
lists ahead of built-in defaults. The nameplate change restores an existing (correct-for-
base-client) memory write. The upstream port is deliberately deferred to follow-up plans
written from a concrete gap report, because its tasks cannot be specified without knowing
which features the report surfaces.

**Tech Stack:** C++17/20, MSVC (Visual Studio 2022, Win32/x86), CMake, Microsoft Detours,
GitHub Actions. Dev machine is WSL/Linux; Windows targets build on CI or a Windows box.

## Global Constraints

- **Platform:** Windows, 32-bit (Win32/x86). Do not introduce 64-bit-only code.
- **C++ standard:** The shared header MUST compile under **C++17** (the injector target is
  C++17) and C++20 (patcher/lib). Use no feature newer than C++17 in `GameExeConfig.h`.
- **Global defines:** The build defines `UNICODE`, `_UNICODE`, `WIN32_LEAN_AND_MEAN`,
  `_CRT_SECURE_NO_WARNINGS` project-wide. Use explicit `...A`/`...W` Win32 calls; do not rely
  on `TCHAR` macros.
- **Default exe order (both binaries):** `Wow.exe`, then `Project-Epoch.exe` /
  `Project Epoch.exe`, then `Ascension.exe` (legacy last).
- **Config filename:** `gameExeLocation.txt`, read from the directory of the running binary.
- **Nameplate address:** `0x00ADAA7C` (validated for base 3.3.5a build 12340 only).
- **Preserve:** anti-detection / evasion / injector logic. Ports are additive.
- **Testing reality:** There is no runtime test framework; the client is validated manually
  on Windows. Only the pure config-parser logic (Task 1) is unit-tested, and it is written to
  compile and run locally with `g++ -std=c++17` on WSL. Windows targets are build-verified on
  CI (push the branch → `Build Injector (x86)` workflow builds `AwesomeWotlkLib` +
  `AwesomeWotlkInjector`; a tag → `windows-release.yml` builds the patcher) or on a Windows
  machine with VS 2022. Runtime behavior is verified manually against a real client.
- **Commits:** Commit after every task. Work stays on branch
  `change-exe-and-sync-from-main-branch-nameplate-distance`.

---

### Task 1: Shared config parser + unit tests

Pure, host-testable parsing logic. This is the only task with a real red/green unit cycle.

**Files:**
- Create: `src/Common/GameExeConfig.h`
- Test: `src/Common/tests/test_gameexe_config.cpp`

**Interfaces:**
- Consumes: nothing.
- Produces (namespace `GameExeConfig`, all `inline`):
  - `std::string parseValue(std::istream& in)` — first non-blank, non-`#` line, trimmed and
    unquoted; `""` if none.
  - `std::string parseValueFromText(const std::string& text)` — convenience wrapper.
  - `std::string basename(const std::string& value)` — component after the last `/` or `\`
    (splits on **both** separators on every OS); returns input unchanged if no separator.
  - `const std::array<const char*, 3>& defaultNames()` — `{ "Wow.exe",
    "Project-Epoch.exe", "Ascension.exe" }`.
  - `std::string readConfiguredValue()` *(Windows-only, `#ifdef _WIN32`)* — reads
    `gameExeLocation.txt` next to the current module; `""` if missing/empty.
  - `std::wstring widen(const std::string& s)` *(Windows-only)* — ACP → UTF-16.

- [ ] **Step 1: Write the failing test**

Create `src/Common/tests/test_gameexe_config.cpp`:

```cpp
#include "../GameExeConfig.h"
#include <cassert>
#include <iostream>

using namespace GameExeConfig;

int main() {
    // Empty / comment-only / blank input yields no value.
    assert(parseValueFromText("") == "");
    assert(parseValueFromText("# only a comment\n") == "");
    assert(parseValueFromText("\n\n   \n# c\n") == "");

    // First non-comment, non-blank line wins.
    assert(parseValueFromText("# c\nWow.exe\n") == "Wow.exe");
    assert(parseValueFromText("A.exe\nB.exe\n") == "A.exe");

    // Whitespace and CRLF are trimmed.
    assert(parseValueFromText("  Wow.exe  \r\n") == "Wow.exe");

    // Surrounding single or double quotes are stripped.
    assert(parseValueFromText("\"C:\\Games\\Wow.exe\"\n") == "C:\\Games\\Wow.exe");
    assert(parseValueFromText("'E:/wow'\n") == "E:/wow");

    // basename() splits on both separators regardless of host OS.
    assert(basename("C:\\Games\\Wow.exe") == "Wow.exe");
    assert(basename("E:/games/wow/Wow.exe") == "Wow.exe");
    assert(basename("Wow.exe") == "Wow.exe");
    assert(basename("") == "");

    // Defaults are ordered base-client first.
    assert(std::string(defaultNames()[0]) == "Wow.exe");
    assert(std::string(defaultNames()[2]) == "Ascension.exe");

    std::cout << "All GameExeConfig tests passed\n";
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:
```bash
g++ -std=c++17 -o /tmp/test_gameexe_config src/Common/tests/test_gameexe_config.cpp && /tmp/test_gameexe_config
```
Expected: FAIL — compile error, `GameExeConfig.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `src/Common/GameExeConfig.h`:

```cpp
#pragma once
#include <string>
#include <istream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <array>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace GameExeConfig {

// Built-in fallback names, highest priority first (base client leads).
inline const std::array<const char*, 3>& defaultNames() {
    static const std::array<const char*, 3> names{
        "Wow.exe", "Project-Epoch.exe", "Ascension.exe" };
    return names;
}

inline std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const std::size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    const std::size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

inline std::string unquote(const std::string& s) {
    if (s.size() >= 2) {
        const char c = s.front();
        if ((c == '"' || c == '\'') && s.back() == c)
            return s.substr(1, s.size() - 2);
    }
    return s;
}

// First non-blank, non-comment line, trimmed and unquoted. "" if none.
inline std::string parseValue(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        const std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        return unquote(t);
    }
    return "";
}

inline std::string parseValueFromText(const std::string& text) {
    std::istringstream ss(text);
    return parseValue(ss);
}

// Filename after the last '/' or '\' (both separators, every OS).
inline std::string basename(const std::string& value) {
    const std::size_t pos = value.find_last_of("/\\");
    return pos == std::string::npos ? value : value.substr(pos + 1);
}

#ifdef _WIN32
inline std::filesystem::path moduleDir() {
    char buf[MAX_PATH] = { 0 };
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return std::filesystem::current_path();
    return std::filesystem::path(buf).parent_path();
}

// Read gameExeLocation.txt next to the executable; "" if missing/empty.
inline std::string readConfiguredValue() {
    const std::filesystem::path cfg = moduleDir() / "gameExeLocation.txt";
    std::ifstream f(cfg);
    if (!f) return "";
    return parseValue(f);
}

inline std::wstring widen(const std::string& s) {
    if (s.empty()) return L"";
    const int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), &w[0], len);
    return w;
}
#endif

} // namespace GameExeConfig
```

- [ ] **Step 4: Run test to verify it passes**

Run:
```bash
g++ -std=c++17 -o /tmp/test_gameexe_config src/Common/tests/test_gameexe_config.cpp && /tmp/test_gameexe_config
```
Expected: PASS — prints `All GameExeConfig tests passed`.

- [ ] **Step 5: Commit**

```bash
git add src/Common/GameExeConfig.h src/Common/tests/test_gameexe_config.cpp
git commit -m "feat: add shared gameExeLocation.txt config parser with tests"
```

---

### Task 2: Patcher reads config to locate the executable

**Files:**
- Modify: `src/AwesomeWotlkPatch/Main.cpp` (includes; `findGameClientExecutable`;
  `applyPatches` Ascension check)

**Interfaces:**
- Consumes: `GameExeConfig::readConfiguredValue`, `GameExeConfig::basename`,
  `GameExeConfig::defaultNames` (Task 1).
- Produces: no new symbols for later tasks.

- [ ] **Step 1: Add includes**

At the top of `src/AwesomeWotlkPatch/Main.cpp`, after the existing includes, add:

```cpp
#include "../Common/GameExeConfig.h"
#include <algorithm>
#include <cctype>
#include <vector>
```

- [ ] **Step 2: Replace `findGameClientExecutable`**

Replace the existing function (currently lines ~14-21) with a config-aware version:

```cpp
const char* findGameClientExecutable()
{
    // Config-derived candidates first, then built-in defaults.
    static std::vector<std::string> candidates;
    candidates.clear();

    const std::string cfg = GameExeConfig::readConfiguredValue();
    if (!cfg.empty()) {
        std::error_code ec;
        if (std::filesystem::is_directory(cfg, ec)) {
            for (const char* name : GameExeConfig::defaultNames())
                candidates.push_back((std::filesystem::path(cfg) / name).string());
        } else {
            candidates.push_back(cfg); // full path or bare filename
        }
    }
    for (const char* name : GameExeConfig::defaultNames())
        candidates.push_back(name);

    for (const std::string& c : candidates)
        if (std::filesystem::is_regular_file(c))
            return _strdup(c.c_str()); // caller uses it read-only for process lifetime
    return NULL;
}
```

Note: `_strdup` keeps the returned pointer valid after the static vector is reused; the
process is short-lived so the small leak is acceptable and matches the original contract
(a `const char*` outliving the call).

- [ ] **Step 3: Make the Ascension patch-set check basename-based**

In `applyPatches`, replace:

```cpp
    if (!strcmp(path, "Ascension.exe")) {
        patches = s_patches_ascension;
    }
```

with:

```cpp
    std::string base = GameExeConfig::basename(path);
    std::transform(base.begin(), base.end(), base.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    if (base == "ascension.exe") {
        patches = s_patches_ascension;
    }
```

- [ ] **Step 4: Build (CI or Windows)**

Local WSL cannot build the MSVC target. Verify on CI by pushing the branch, or on a Windows
box:
```
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -T host=x64
cmake --build build --config Release --target AwesomeWotlkPatch
```
Expected: `AwesomeWotlkPatch.exe` builds with no errors.

- [ ] **Step 5: Manual runtime verification**

On the Windows client folder:
1. Place `AwesomeWotlkPatch.exe` next to `Wow.exe` with no `gameExeLocation.txt` → run it →
   it finds and patches `Wow.exe`.
2. Move `Wow.exe` elsewhere, add `gameExeLocation.txt` containing that full path (or its
   folder) → run → it patches the configured file.
Expected: success message names the correct executable; no "executable not found" error.

- [ ] **Step 6: Commit**

```bash
git add src/AwesomeWotlkPatch/Main.cpp
git commit -m "feat: patcher locates game exe via gameExeLocation.txt with Wow.exe default"
```

---

### Task 3: Injector reads config for the target process name

**Files:**
- Modify: `src/AwesomeWotlkInjector/Injector.cpp` (include; process-name list in `wmain`)

**Interfaces:**
- Consumes: `GameExeConfig::readConfiguredValue`, `GameExeConfig::basename`,
  `GameExeConfig::widen` (Task 1).
- Produces: no new symbols.

- [ ] **Step 1: Add include**

After the existing includes at the top of `src/AwesomeWotlkInjector/Injector.cpp`, add:

```cpp
#include "../Common/GameExeConfig.h"
```

- [ ] **Step 2: Prepend the configured process name to the default list**

In `wmain`, immediately after the `processNames` vector is built from the decrypted defaults
(currently lines ~145-148), insert:

```cpp
    // A configured gameExeLocation.txt value takes priority; match by filename.
    const std::string cfgValue = GameExeConfig::readConfiguredValue();
    const std::string cfgName = GameExeConfig::basename(cfgValue);
    if (!cfgName.empty()) {
        processNames.insert(processNames.begin(), GameExeConfig::widen(cfgName));
    }
```

The existing `Wow.exe` default should also be present. In the same `processNames`
initializer, add a third entry so a bare default still matches the base client:

```cpp
    std::vector<std::wstring> processNames = {
        L"Wow.exe",
        DecryptString(ENCRYPTED_PROCESS_NAME_1), // "Project Epoch.exe"
        DecryptString(ENCRYPTED_PROCESS_NAME_2)  // "ascension.exe"
    };
```

(The explicit CLI arg `argv[1]` continues to override everything, unchanged.)

- [ ] **Step 3: Update the "not found" error message**

Replace the message at the no-process-found branch (currently
`L"Error: Neither 'Project Epoch.exe' nor 'ascension.exe' found."`) with:

```cpp
            LOG_ERROR(L"Error: No game client process found (looked for Wow.exe / Project Epoch.exe / ascension.exe, plus any gameExeLocation.txt value).");
```

- [ ] **Step 4: Build (CI or Windows)**

Push the branch to trigger the `Build Injector (x86)` workflow, or build locally on Windows:
```
cmake --build build --config Release --target AwesomeWotlkLib AwesomeWotlkInjector
```
Expected: `AwesomeWotlkInjector.exe` and `AwesomeWotlkLib.dll` build with no errors.

- [ ] **Step 5: Manual runtime verification**

1. Launch the base `Wow.exe`, run the injector with no `gameExeLocation.txt` → it reports
   `Target process 'Wow.exe' found` and injects.
2. Put a custom exe name in `gameExeLocation.txt` matching a running client → injector
   targets that name first.
Expected: injection succeeds against the running base client.

- [ ] **Step 6: Commit**

```bash
git add src/AwesomeWotlkInjector/Injector.cpp
git commit -m "feat: injector targets gameExeLocation.txt name, Wow.exe by default"
```

---

### Task 4: Re-enable `nameplateDistance` on the base client

**Files:**
- Modify: `src/AwesomeWotlkLib/NamePlates.cpp` (handler at ~113-119; registration at ~691)

**Interfaces:**
- Consumes: nothing new.
- Produces: no new symbols; restores existing CVar behavior.

- [ ] **Step 1: Restore the handler body**

Replace the current no-op `CVarHandler_NameplateDistance` (the block with the three
commented lines and bare `return 1;`) with:

```cpp
static int CVarHandler_NameplateDistance(Console::CVar*, const char*, const char* value, LPVOID)
{
    float f = (float)atof(value);
    if (!(f > 0.f)) f = 41.f;   // invalid / non-positive -> base-client default
    if (f < 41.f)  f = 41.f;    // clamp to sane base-client range
    if (f > 100.f) f = 100.f;
    *(float*)0x00ADAA7C = f * f; // squared view distance, base 3.3.5a 12340
    return 1;
}
```

- [ ] **Step 2: Update the registered default**

At the registration line (~691), change the default value string from `"43"` to `"41"`:

```cpp
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateDistance, "nameplateDistance", NULL, (Console::CVarFlags)1, "41", CVarHandler_NameplateDistance);
```

- [ ] **Step 3: Build (CI or Windows)**

```
cmake --build build --config Release --target AwesomeWotlkLib
```
Expected: `AwesomeWotlkLib.dll` builds with no errors.

- [ ] **Step 4: Manual runtime verification**

In-client after injection:
1. `/console nameplateDistance 41` → default nameplate range.
2. `/console nameplateDistance 80` → nameplates visible noticeably farther.
3. `/console nameplateDistance abc` (invalid) → falls back to 41, no crash.
Expected: render distance changes with the value; no client crash.

- [ ] **Step 5: Commit**

```bash
git add src/AwesomeWotlkLib/NamePlates.cpp
git commit -m "feat: re-enable nameplateDistance on base 3.3.5a client"
```

---

### Task 5: README correction, config template, release packaging

**Files:**
- Create: `gameExeLocation.txt` (repo-root commented template)
- Modify: `.github/workflows/windows-release.yml` (include the template in the zip)
- Modify: `README.md` (exe name, nameplate-distance notes, remove fictional launcher claims)

**Interfaces:**
- Consumes: nothing.
- Produces: nothing.

- [ ] **Step 1: Create the config template**

Create `gameExeLocation.txt` at the repo root:

```
# Set the path, folder, or filename of your game client executable.
# Remove the leading '#' on the line below and edit it. Leave it commented to use defaults
# (Wow.exe, then Project-Epoch.exe, then Ascension.exe), searched next to the tool.
# Wow.exe
```

- [ ] **Step 2: Ship the template in the release zip**

In `.github/workflows/windows-release.yml`, in the "Package artifact" step, add the template
to the archive. Change the `Compress-Archive` line to include it:

```powershell
          Copy-Item "${{ github.workspace }}\gameExeLocation.txt" "$src\gameExeLocation.txt" -Force
          Compress-Archive -Path "$src\AwesomeWotlkPatch.exe","$src\AwesomeWotlkLib.dll","$src\gameExeLocation.txt" -DestinationPath $zip -Force
```

- [ ] **Step 3: Correct the README**

Make these edits in `README.md`:
1. In the intro paragraph, replace `launching `Ascension.exe`` with `launching the base
   **World of Warcraft 3.3.5a** client (`Wow.exe`)`.
2. In **Custom Game Path**, replace `Set a custom path for `Ascension.exe`` with `Set a
   custom path, folder, or filename for the client executable (defaults to `Wow.exe`)`.
3. In **Additional QoL Features**, delete the parenthetical
   `(note: changing nameplate distance is disabled as it's considered cheating)`.
4. Delete the **Nameplate Distance Limitation** bullet
   (`You can't go past Epoch's limitation via this mod.`).
5. Remove or rewrite the **Usage → Automated Injection** steps that describe an
   `AwesomeEpoch.exe` launcher reading `gameExeLocation.txt` and auto-launching the client —
   that binary is not in this repo. Replace with the real flow: run the client, then run
   `AwesomeWotlkInjector.exe` (which reads `gameExeLocation.txt`), or patch with
   `AwesomeWotlkPatch.exe`.
6. Leave the top-of-file ban warning intact.

- [ ] **Step 4: Verify no stale references remain**

Run:
```bash
grep -niE "Ascension\.exe|nameplate distance is disabled|AwesomeEpoch\.exe" README.md
```
Expected: no matches referring to the game client as Ascension, no "disabled" nameplate note,
and no launcher claims (any remaining `Ascension` mention should only be historical/credits).

- [ ] **Step 5: Commit**

```bash
git add gameExeLocation.txt .github/workflows/windows-release.yml README.md
git commit -m "docs: correct README for base client, ship gameExeLocation.txt template"
```

---

### Task 6: Upstream gap-analysis report

Deliverable is a written report that enumerates and classifies the port work. It is the
prerequisite for the port follow-up plans; those cannot be written without it.

**Files:**
- Create: `docs/superpowers/specs/2026-08-07-upstream-gap-analysis.md`

**Interfaces:**
- Consumes: nothing.
- Produces: a classified feature list consumed by the follow-up port plans.

- [ ] **Step 1: Collect the upstream surface**

Enumerate `noname08662/awesome_wotlk` source modules and features:
```bash
gh api repos/noname08662/awesome_wotlk/git/trees/main?recursive=1 --jq '.tree[].path' | grep -E "^src/AwesomeWotlkLib/.*\.(cpp|h)$"
```
Fetch the upstream README feature/CVar/API/event lists for reference.

- [ ] **Step 2: Diff against the fork**

Compare against the fork's `src/AwesomeWotlkLib` modules and its registered CVars/APIs
(`grep -rn "registerCVar\|registerFunction\|registerEvent" src/AwesomeWotlkLib`). Identify:
modules the fork lacks (known to include at least `Camera`, `CombatLog`, `D3D`, `VFX`, the
`MSDF*` family, support headers `Enums.h`/`Lua.h`/`ReTools.h`/`Types.h`), plus any new CVars,
Lua APIs, events, and bugfixes.

- [ ] **Step 3: Write the classified report**

Write `docs/superpowers/specs/2026-08-07-upstream-gap-analysis.md` with a table of each
missing feature classified as:
- **safe** — self-contained, base-3.3.5a-compatible, no anti-detection/injector coupling;
- **conditional** — portable but needs care (shared hooks, `Entry.cpp` wiring);
- **defer** — large/invasive (the MSDF stack: `deps/freetype-2.14.1` + `deps/msdfgen` +
  `D3D` hook + `MSDF*` modules + CMake wiring).
For each entry note the source files and the estimated integration cost/risk against the
current Win32 build.

- [ ] **Step 4: Commit**

```bash
git add docs/superpowers/specs/2026-08-07-upstream-gap-analysis.md
git commit -m "docs: upstream awesome_wotlk gap analysis (fork vs noname08662)"
```

---

## Follow-up plans (written after Task 6)

The upstream feature port is intentionally **not** decomposed into tasks here. Its steps
depend on the classified list produced by Task 6, and writing "port the appropriate features"
would be a placeholder. After Task 6 lands, write two follow-up plans from the report:

1. **Safe-wins port plan** — one task per `safe`/`conditional` feature (build + manual verify
   each; keep changes additive to preserve anti-detection).
2. **MSDF fonts port plan** — the gated sub-phase: vendor `deps/freetype-2.14.1` and
   `deps/msdfgen`, wire CMake, port the `D3D` + `MSDF*` modules and `Entry.cpp` hooks. Gate:
   Win32 Release still builds and the DLL injects without regressions. Fallback: if it
   destabilizes the build within the effort, it stays its own spec and does not block the
   already-shipped Tasks 1–5.

## Self-Review Notes

- **Spec coverage:** Part 1 (exe config) → Tasks 1–3, 5; Part 2 (nameplate distance) →
  Task 4; Part 3 (upstream port) → Task 6 + follow-up plans; README correction → Task 5.
- **Types consistent:** `parseValue` / `parseValueFromText` / `basename` / `defaultNames` /
  `readConfiguredValue` / `widen` are named identically in Task 1's definition and Tasks 2–3's
  usage.
- **No placeholders:** every code step shows concrete code; the port is explicitly deferred
  with a stated reason rather than stubbed.
