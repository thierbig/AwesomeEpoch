// AwesomeEpoch launcher (AwesomeEpoch.exe)
//
// Native replacement for the old batch-compiled launcher. Reads gameExeLocation.txt (via the
// shared GameExeConfig helper, defaulting to Wow.exe), makes sure the game client is running,
// waits for it to initialize, then runs AwesomeWotlkInjector.exe against it. Unlike a
// bat-to-exe build, this uses the launcher's own directory (GetModuleFileName), so it works
// wherever the folder lives. Progress is appended to AutoInject_Awesome.log next to the exe.

#include "../Common/GameExeConfig.h"
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static const int   INJECT_ATTEMPTS   = 3;
static const DWORD GAME_INIT_WAIT_MS  = 4000;
static const DWORD INJECT_WAIT_MS     = 4000;

static fs::path     g_baseDir;
static std::ofstream g_log;

static std::string timestampNow()
{
    SYSTEMTIME t; GetLocalTime(&t);
    char s[40] = {0};
    std::snprintf(s, sizeof(s), "%04d-%02d-%02d %02d:%02d:%02d",
        t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    return s;
}

static void logLine(const char* tag, const std::string& msg)
{
    std::string line = "[" + std::string(tag) + "] " + msg;
    std::printf("%s\n", line.c_str());
    if (g_log) { g_log << line << "\n"; g_log.flush(); }
}

static fs::path moduleDir()
{
    wchar_t buf[MAX_PATH] = {0};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return fs::current_path();
    return fs::path(buf).parent_path();
}

static bool isProcessRunning(const std::wstring& exeName)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W e; e.dwSize = sizeof(e);
    bool found = false;
    if (Process32FirstW(snap, &e)) {
        do {
            if (_wcsicmp(e.szExeFile, exeName.c_str()) == 0) { found = true; break; }
        } while (Process32NextW(snap, &e));
    }
    CloseHandle(snap);
    return found;
}

// Resolve the game executable from gameExeLocation.txt: a full path, a folder (probe defaults
// inside it), or a bare filename (probe default install folders). Empty path on failure.
static fs::path resolveGameExe()
{
    const std::string cfg = GameExeConfig::readConfiguredValue(); // reads gameExeLocation.txt next to us
    std::error_code ec;

    if (!cfg.empty()) {
        logLine("INFO", "gameExeLocation.txt => " + cfg);
        fs::path p(cfg);
        if (fs::is_directory(p, ec)) {
            for (const char* name : GameExeConfig::defaultNames())
                if (fs::is_regular_file(p / name, ec)) return p / name;
            logLine("WARN", "Configured folder has no known client exe: " + p.string());
        } else if (fs::is_regular_file(p, ec)) {
            return p;
        } else {
            logLine("WARN", "Configured path does not exist yet: " + p.string());
            return p; // trust it; may become valid, or the injector still auto-detects a running client
        }
    } else {
        logLine("INFO", "gameExeLocation.txt empty/missing; probing common install folders.");
    }

    // Fallback probe: common Epoch/WoW install folders for the default client names.
    static const char* probeDirs[] = {
        "C:\\Games\\epoch_live", "C:\\Games\\Wow", "C:\\Program Files\\Wow",
        "D:\\Games\\epoch_live", "D:\\Games\\Wow",
    };
    for (const char* dir : probeDirs)
        for (const char* name : GameExeConfig::defaultNames())
            if (fs::is_regular_file(fs::path(dir) / name, ec))
                return fs::path(dir) / name;
    return {};
}

static bool startGame(const fs::path& gameExe, const fs::path& gameDir)
{
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    const std::wstring exe = gameExe.wstring();
    const std::wstring dir = gameDir.wstring();
    sei.lpFile = exe.c_str();
    sei.lpDirectory = dir.c_str();
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&sei)) {
        logLine("ERROR", "Failed to start game (ShellExecuteEx error " + std::to_string(GetLastError()) + ").");
        return false;
    }
    if (sei.hProcess) CloseHandle(sei.hProcess);
    return true;
}

// Runs AwesomeWotlkInjector.exe <processName> from our own directory; returns its exit code
// (0 = success). Returns -1 if it could not be launched.
static int runInjector(const fs::path& injector, const std::wstring& processName)
{
    std::wstring cmd = L"\"" + injector.wstring() + L"\" \"" + processName + L"\"";
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    const std::wstring workDir = g_baseDir.wstring();
    if (!CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        workDir.c_str(), &si, &pi)) {
        logLine("ERROR", "Failed to launch injector (error " + std::to_string(GetLastError()) + ").");
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(code);
}

int main()
{
    g_baseDir = moduleDir();
    g_log.open((g_baseDir / "AutoInject_Awesome.log").wstring(), std::ios::app);
    logLine("INFO", "==== " + timestampNow() + " : AwesomeEpoch launcher start ====");
    logLine("INFO", "BASE_DIR=" + g_baseDir.string());

    const fs::path injector = g_baseDir / "AwesomeWotlkInjector.exe";
    if (!fs::is_regular_file(injector)) {
        logLine("ERROR", "AwesomeWotlkInjector.exe not found next to this launcher.");
        MessageBoxA(nullptr, "AwesomeWotlkInjector.exe is missing from this folder.", "AwesomeEpoch", MB_ICONERROR | MB_OK);
        return 1;
    }
    // Helpful warning: the injector needs the DLL (and skia.dll for MSDF fonts) beside it.
    if (!fs::is_regular_file(g_baseDir / "AwesomeWotlkLib.dll"))
        logLine("WARN", "AwesomeWotlkLib.dll is not in this folder - injection will fail until you add it.");
    if (!fs::is_regular_file(g_baseDir / "skia.dll"))
        logLine("WARN", "skia.dll is not in this folder - MSDF vector fonts (MSDFMode) need it beside the DLL.");

    const fs::path gameExe = resolveGameExe();
    if (gameExe.empty()) {
        logLine("ERROR", "Could not find the game client. Edit gameExeLocation.txt to point at your Wow.exe.");
        MessageBoxA(nullptr,
            "Could not find the game client.\n\nEdit gameExeLocation.txt (next to AwesomeEpoch.exe) so it\n"
            "points at your Wow.exe, e.g.:\n\nC:\\Users\\you\\Desktop\\EpochNewClient\\Wow.exe",
            "AwesomeEpoch", MB_ICONERROR | MB_OK);
        return 1;
    }

    const std::wstring exeName = gameExe.filename().wstring();
    const fs::path     gameDir = gameExe.parent_path();
    logLine("OK", "Using client: " + gameExe.string());

    if (isProcessRunning(exeName)) {
        logLine("OK", "Client already running.");
    } else {
        logLine("INFO", "Client not running; starting it...");
        if (!startGame(gameExe, gameDir)) {
            MessageBoxA(nullptr, "Failed to start the game client.", "AwesomeEpoch", MB_ICONERROR | MB_OK);
            return 1;
        }
        logLine("INFO", "Waiting " + std::to_string(GAME_INIT_WAIT_MS / 1000) + "s for the client to initialize...");
        Sleep(GAME_INIT_WAIT_MS);
    }

    for (int attempt = 1; attempt <= INJECT_ATTEMPTS; ++attempt) {
        if (!isProcessRunning(exeName)) {
            logLine("INFO", "Client not visible yet; waiting before inject attempt " + std::to_string(attempt) + "...");
            Sleep(INJECT_WAIT_MS);
        }
        logLine("INFO", "Inject attempt " + std::to_string(attempt) + " of " + std::to_string(INJECT_ATTEMPTS) + "...");
        const int code = runInjector(injector, exeName);
        if (code == 0) {
            logLine("OK", "Injection succeeded. You can enter the game.");
            return 0;
        }
        logLine("WARN", "Injector returned " + std::to_string(code) + "; see injector_output.log.");
        Sleep(INJECT_WAIT_MS);
    }

    logLine("ERROR", "Injection failed after " + std::to_string(INJECT_ATTEMPTS) + " attempts. See injector_output.log.");
    MessageBoxA(nullptr,
        "Injection failed. Make sure AwesomeWotlkLib.dll and skia.dll are in this folder,\n"
        "run as Administrator, and that your client is the base 3.3.5a Wow.exe.",
        "AwesomeEpoch", MB_ICONWARNING | MB_OK);
    return 1;
}
