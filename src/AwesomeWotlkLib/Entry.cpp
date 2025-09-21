#include "BugFixes.h"
#include "CommandLine.h"
#include "NamePlates.h"
#include "Misc.h"
#include "Hooks.h"
#include "Inventory.h"
#include "Item.h"
#include "Spell.h"
#include "UnitAPI.h"
#include <Windows.h>
#include <Detours/detours.h>
#include <TlHelp32.h>
#include "VoiceChat.h"
#include "AntiDetection.h"
#include "AdvancedEvasion.h"
#include "EvasionLogger.h"
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <cwchar>

// Forward declarations
static void OnRealAttach();

static int lua_debugbreak(lua_State* L)
{
    if (IsDebuggerPresent())
        DebugBreak();
    return 0;
}

static int lua_openawesomewotlk(lua_State* L)
{
    lua_pushnumber(L, 1.f);
    lua_setglobal(L, "AwesomeWotlk");
    
    // Enable WeakAuras awesome features by providing C_NamePlate API
    // This makes WeakAuras.IsAwesomeEnabled() return 1 (nameplate support)
    lua_newtable(L);  // Create C_NamePlate table
    
    // Add a dummy GetNamePlateForUnit function - WeakAuras just checks for existence
    lua_pushcfunction(L, [](lua_State* L) -> int {
        // Simple dummy implementation - just return nil for now
        // WeakAuras only checks if this function exists to enable nameplate features
        lua_pushnil(L);
        return 1;
    });
    lua_setfield(L, -2, "GetNamePlateForUnit");
    
    lua_setglobal(L, "C_NamePlate");  // Set as global C_NamePlate
    
#ifdef _DEBUG
    lua_pushcfunction(L, lua_debugbreak);
    lua_setglobal(L, "debugbreak");
#endif
    return 0;
}

// Hook common anti-cheat detection APIs to hide our presence
static HMODULE (WINAPI* OriginalGetModuleHandleA)(LPCSTR lpModuleName) = GetModuleHandleA;
static HMODULE (WINAPI* OriginalGetModuleHandleW)(LPCWSTR lpModuleName) = GetModuleHandleW;

// Our DLL name for hiding
static const char* OUR_DLL_NAME = "AwesomeWotlkLib.dll";
static HMODULE g_ourModule = nullptr;
static const wchar_t* OUR_DLL_NAME_W = L"AwesomeWotlkLib.dll";

// MODULEINFO is now defined in Psapi.h which is included via AntiDetection.h
// No need to redefine it here

// K32 Process Status API pointers (resolved at runtime)
static BOOL (WINAPI* OriginalK32EnumProcessModules)(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded) = nullptr;
static BOOL (WINAPI* OriginalK32GetModuleInformation)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb) = nullptr;

// Toolhelp32 enumeration pointers (to hook Module32First/Next)
static BOOL (WINAPI* OriginalModule32FirstW)(HANDLE hSnapshot, LPMODULEENTRY32W lpme) = Module32FirstW;
static BOOL (WINAPI* OriginalModule32NextW)(HANDLE hSnapshot, LPMODULEENTRY32W lpme) = Module32NextW;

HMODULE WINAPI HookedGetModuleHandleA(LPCSTR lpModuleName) {
    if (lpModuleName && _stricmp(lpModuleName, OUR_DLL_NAME) == 0) {
        return nullptr; // Hide our DLL from GetModuleHandle calls
    }
    return OriginalGetModuleHandleA(lpModuleName);
}

HMODULE WINAPI HookedGetModuleHandleW(LPCWSTR lpModuleName) {
    if (lpModuleName) {
        char narrowName[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, lpModuleName, -1, narrowName, MAX_PATH, NULL, NULL);
        if (_stricmp(narrowName, OUR_DLL_NAME) == 0) {
            return nullptr; // Hide our DLL from GetModuleHandle calls
        }
    }
    return OriginalGetModuleHandleW(lpModuleName);
}

// Additional stealth APIs
static DWORD (WINAPI* OriginalGetModuleFileNameA)(HMODULE hModule, LPSTR lpFilename, DWORD nSize) = GetModuleFileNameA;
static DWORD (WINAPI* OriginalGetModuleFileNameW)(HMODULE hModule, LPWSTR lpFilename, DWORD nSize) = GetModuleFileNameW;

// Hook GetModuleFileName to hide our DLL path
DWORD WINAPI HookedGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize) {
    if (hModule == g_ourModule) {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0; // Hide our filename
    }
    return OriginalGetModuleFileNameA(hModule, lpFilename, nSize);
}

DWORD WINAPI HookedGetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize) {
    if (hModule == g_ourModule) {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0; // Hide our filename
    }
    return OriginalGetModuleFileNameW(hModule, lpFilename, nSize);
}

// Hide our module from Toolhelp32 enumeration
BOOL WINAPI HookedModule32FirstW(HANDLE hSnapshot, LPMODULEENTRY32W lpme) {
    BOOL result = OriginalModule32FirstW(hSnapshot, lpme);
    if (!result) return result;
    while (result) {
        bool isUs = (lpme->hModule == g_ourModule) || (_wcsicmp(lpme->szModule, OUR_DLL_NAME_W) == 0);
        if (!isUs) break;
        result = OriginalModule32NextW(hSnapshot, lpme);
    }
    return result;
}

BOOL WINAPI HookedModule32NextW(HANDLE hSnapshot, LPMODULEENTRY32W lpme) {
    BOOL result;
    while ((result = OriginalModule32NextW(hSnapshot, lpme)) == TRUE) {
        bool isUs = (lpme->hModule == g_ourModule) || (_wcsicmp(lpme->szModule, OUR_DLL_NAME_W) == 0);
        if (!isUs) return TRUE;
    }
    return result;
}

// K32 variants of Psapi APIs without linking Psapi
BOOL WINAPI HookedK32EnumProcessModules(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded) {
    if (!OriginalK32EnumProcessModules) return FALSE;
    BOOL result = OriginalK32EnumProcessModules(hProcess, lphModule, cb, lpcbNeeded);
    if (result && lphModule && lpcbNeeded) {
        DWORD count = *lpcbNeeded / sizeof(HMODULE);
        for (DWORD i = 0; i < count; ++i) {
            if (lphModule[i] == g_ourModule) {
                for (DWORD j = i; j + 1 < count; ++j) {
                    lphModule[j] = lphModule[j + 1];
                }
                --count;
                *lpcbNeeded = count * sizeof(HMODULE);
                break;
            }
        }
    }
    return result;
}

BOOL WINAPI HookedK32GetModuleInformation(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb) {
    if (!OriginalK32GetModuleInformation) return FALSE;
    if (hModule == g_ourModule) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE; // Hide our module information
    }
    return OriginalK32GetModuleInformation(hProcess, hModule, lpmodinfo, cb);
}

// Memory pattern obfuscation - XOR our PE header
static void ObfuscatePEHeader() {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(g_ourModule, &mbi, sizeof(mbi))) {
        DWORD oldProtect;
        if (VirtualProtect(mbi.BaseAddress, 0x1000, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            // XOR the PE header to break signature detection
            BYTE* header = (BYTE*)mbi.BaseAddress;
            for (int i = 0; i < 0x400; i++) {
                header[i] ^= 0x42;
            }
            VirtualProtect(mbi.BaseAddress, 0x1000, oldProtect, &oldProtect);
        }
    }
}

// Install comprehensive API hooks to hide from ClientExtensions.dll
static void InstallAntiCheatEvasion() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    // Hook module enumeration APIs to hide our presence
    DetourAttach(&(PVOID&)OriginalGetModuleHandleA, HookedGetModuleHandleA);
    DetourAttach(&(PVOID&)OriginalGetModuleHandleW, HookedGetModuleHandleW);

    // Resolve K32 APIs at runtime and hook if present (no Psapi link needed)
    HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
    if (hKernel) {
        OriginalK32EnumProcessModules = reinterpret_cast<BOOL (WINAPI*)(HANDLE, HMODULE*, DWORD, LPDWORD)>(
            GetProcAddress(hKernel, "K32EnumProcessModules"));
        OriginalK32GetModuleInformation = reinterpret_cast<BOOL (WINAPI*)(HANDLE, HMODULE, LPMODULEINFO, DWORD)>(
            GetProcAddress(hKernel, "K32GetModuleInformation"));
        if (OriginalK32EnumProcessModules) {
            DetourAttach(&(PVOID&)OriginalK32EnumProcessModules, HookedK32EnumProcessModules);
        }
        if (OriginalK32GetModuleInformation) {
            DetourAttach(&(PVOID&)OriginalK32GetModuleInformation, HookedK32GetModuleInformation);
        }
    }

    // Hook Toolhelp32 module enumeration to hide our module
    DetourAttach(&(PVOID&)OriginalModule32FirstW, HookedModule32FirstW);
    DetourAttach(&(PVOID&)OriginalModule32NextW, HookedModule32NextW);

    // Hook GetModuleFileName to hide our filename
    DetourAttach(&(PVOID&)OriginalGetModuleFileNameA, HookedGetModuleFileNameA);
    DetourAttach(&(PVOID&)OriginalGetModuleFileNameW, HookedGetModuleFileNameW);
    
    DetourTransactionCommit();
    
    // Obfuscate our PE header after hooking
    ObfuscatePEHeader();
}

// Anti-debugging and stealth measures
static bool IsEnvironmentSafe() {
    __try {
        EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Starting function");
        
        // Enhanced anti-debugging checks using AntiDetection module
        EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to check IsDebuggerPresent");
        if (IsDebuggerPresent()) {
            EVASION_LOG_WARNING("ENV_SAFE", "IsEnvironmentSafe: Debugger detected, returning false");
            return false;
        }
        EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: IsDebuggerPresent check passed");
        
        // Check for VM/Sandbox
        //EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to check VM/Sandbox");
        if (AntiDetection::EnvironmentChecker::IsRunningInVM() ||
            AntiDetection::EnvironmentChecker::IsRunningInSandbox()) {
            //EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: VM/Sandbox detected, applying extra measures");
            // In VM or sandbox - be extra careful
            //EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to scramble unused memory");
            //AntiDetection::MemoryScrambler::ScrambleUnusedMemory();
            //EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to randomize stack patterns");
            //AntiDetection::MemoryScrambler::RandomizeStackPatterns();
            //EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: VM/Sandbox measures completed");
        } else {
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: VM/Sandbox check passed");
        }
        
        // Check hardware
        EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to check suspicious hardware");
        if (AntiDetection::EnvironmentChecker::HasSuspiciousHardware()) {
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Suspicious hardware detected, adding delays");
            // Suspicious hardware detected
            AntiDetection::TimingObfuscator::AddRandomDelays();
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Random delays added");
        } else {
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Hardware check passed");
        }
        
        // Check if ClientExtensions.dll is loaded (anti-cheat system)
        EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to check ClientExtensions.dll");
        HMODULE clientExt = GetModuleHandleA("ClientExtensions.dll");
        if (clientExt != nullptr) {
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: ClientExtensions.dll detected, installing evasion");
            // Install enhanced anti-cheat evasion hooks
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to install anti-cheat evasion");
            InstallAntiCheatEvasion();
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Anti-cheat evasion installed");
            
            // Additional evasion for anti-cheat
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to erase injection traces");
            AntiDetection::MemoryScrambler::EraseInjectionTraces();
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to insert dead code");
            AntiDetection::CodeMutator::InsertDeadCode();
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Anti-cheat measures completed");
        } else {
            EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: ClientExtensions.dll not found");
        }
        
        // Check for common analysis tools in memory
        EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to check analysis tools");
        HMODULE modules[] = {
            GetModuleHandleA("dbghelp.dll"),
            GetModuleHandleA("ntdll.dll")
        };
        EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Analysis tools check completed");
        
        // Scramble memory patterns
        //EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: About to scramble memory patterns");
        //AntiDetection::MemoryScrambler::ScrambleUnusedMemory();
        //EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Memory patterns scrambled");
        
        //EVASION_LOG_SUCCESS("ENV_SAFE", "IsEnvironmentSafe: Function completed successfully, returning true");
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        EVASION_LOG_ERROR("ENV_SAFE", "IsEnvironmentSafe: CRITICAL EXCEPTION CAUGHT!");
        return false; // Fail safe - assume environment is not safe if we crash
    }
}

// Hide our threads from thread enumeration
static HANDLE (WINAPI* OriginalCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID) = CreateToolhelp32Snapshot;
static std::vector<DWORD> g_ourThreadIds;

HANDLE WINAPI HookedCreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID) {
    HANDLE result = OriginalCreateToolhelp32Snapshot(dwFlags, th32ProcessID);
    
    if (dwFlags & TH32CS_SNAPTHREAD) {
        // If they're enumerating threads, we need to hide our threads
        // This is a simplified approach - in practice you'd need more sophisticated filtering
    }
    
    return result;
}

// Install additional thread/memory hiding hooks
static void InstallAdvancedEvasion() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    DetourAttach(&(PVOID&)OriginalCreateToolhelp32Snapshot, HookedCreateToolhelp32Snapshot);
    
    DetourTransactionCommit();
}

// Delayed initialization function
static void DelayedInitialization() {
    __try {
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitialization: Starting function");
        
        // Store our thread ID for hiding
        g_ourThreadIds.push_back(GetCurrentThreadId());
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitialization: Thread ID stored");
        
        // Install additional evasion measures
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitialization: About to call InstallAdvancedEvasion");
        InstallAdvancedEvasion();
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitialization: InstallAdvancedEvasion completed");
        
        // Ensure CVars are present before heavy initialization, in case we missed CVars_Initialize
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitialization: Ensuring custom CVars are registered (pre-OnRealAttach)");
        Hooks::ensureCustomCVarsRegistered();

        // Since user presses key when ready and in-world, initialize immediately
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitialization: About to call OnRealAttach");
        OnRealAttach();
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitialization: OnRealAttach completed");
        
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitialization: Function completed successfully");
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        EVASION_LOG_ERROR("DELAYED", "DelayedInitialization: CRITICAL EXCEPTION CAUGHT!");
    }
}

static void OnRealAttach()
{
    __try {
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Starting function");
        
        // Environment safety check
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to check environment safety");
        if (!IsEnvironmentSafe()) {
            EVASION_LOG_WARNING("REAL_ATTACH", "OnRealAttach: Environment not safe, aborting");
            return;
        }
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Environment safety check passed");
        
        // Initialize advanced evasion techniques
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize advanced evasion");
        AdvancedEvasion::EvasionManager::InitializeAll();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Advanced evasion initialized");
        
        // Apply adaptive evasion based on detected environment
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to apply adaptive evasion");
        AdvancedEvasion::EvasionManager::ApplyAdaptiveEvasion();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Adaptive evasion applied");

        // Removed fake TOS/EULA memory writes - they were causing crashes
        // Real TOS/EULA handling should be done through proper game APIs if needed

        // Initialize modules
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Starting module initialization");
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: DetourTransactionBegin");
        DetourTransactionBegin();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: DetourUpdateThread(GetCurrentThread())");
        DetourUpdateThread(GetCurrentThread());
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize Hooks");
        Hooks::initialize();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Hooks initialized");
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize BugFixes");
        BugFixes::initialize();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: BugFixes initialized");
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize CommandLine");
        CommandLine::initialize();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: CommandLine initialized");
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize Inventory");
        Inventory::initialize();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Inventory initialized");
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize Item");
        Item::initialize();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Item initialized");
        
        // NamePlates and Misc were initialized early in OnAttach to ensure CVars and handlers are queued before UI init
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Skipping NamePlates/Misc (already initialized in OnAttach)");
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize UnitAPI");
        UnitAPI::initialize();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: UnitAPI initialized");
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize Spell");
        Spell::initialize();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Spell initialized");
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to initialize VoiceChat");
        VoiceChat::initialize();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: VoiceChat initialized");
        
        // Ensure all queued custom CVars are registered even if CVars_Initialize ran before our detour
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Ensuring custom CVars are registered");
        Hooks::ensureCustomCVarsRegistered();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Custom CVars ensured");

        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to commit Detour transaction");
        {
            LONG detErr = DetourTransactionCommit();
            if (detErr == NO_ERROR) {
                EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Detour transaction committed (status=0)");
            } else {
                EVASION_LOG_ERROR("REAL_ATTACH", std::string("OnRealAttach: Detour transaction commit failed (status=") + std::to_string(detErr) + ")");
            }
        }

        // Register base
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to register Lua library");
        Hooks::FrameXML::registerLuaLib(lua_openawesomewotlk);
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Lua library registered");
        
        // Final evasion check after initialization
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: About to apply final adaptive evasion");
        AdvancedEvasion::EvasionManager::ApplyAdaptiveEvasion();
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Final adaptive evasion applied");
        
        EVASION_LOG_SUCCESS("REAL_ATTACH", "OnRealAttach: Function completed successfully");
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        EVASION_LOG_ERROR("REAL_ATTACH", "OnRealAttach: CRITICAL EXCEPTION CAUGHT!");
    }
}

static void OnAttach()
{
    // Set up basic hooks only - no world detection needed
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: DetourTransactionBegin");
    DetourTransactionBegin();
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: DetourUpdateThread(GetCurrentThread())");
    DetourUpdateThread(GetCurrentThread());
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: Initializing Hooks");
    Hooks::initialize(); // Basic hook setup
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: Hooks initialized");
    // Initialize modules that only register CVars/handlers and one small detour, safe pre-world
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: Initializing NamePlates");
    NamePlates::initialize();
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: NamePlates initialized");
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: Initializing Misc");
    Misc::initialize();
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: Misc initialized");
    {
        LONG detErr = DetourTransactionCommit();
        if (detErr == NO_ERROR) {
            EVASION_LOG_SUCCESS("ATTACH", "OnAttach: Detour transaction committed (status=0)");
        } else {
            EVASION_LOG_ERROR("ATTACH", std::string("OnAttach: Detour transaction commit failed (status=") + std::to_string(detErr) + ")");
        }
    }

    // Ensure CVars right after basic hooks are in place
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: Ensuring custom CVars are registered (early)");
    Hooks::ensureCustomCVarsRegistered();
    EVASION_LOG_SUCCESS("ATTACH", "OnAttach: Early CVars ensure completed");
}

static DWORD WINAPI AttachThread(LPVOID) {
    __try {
        EVASION_LOG_SUCCESS("ATTACH", "AttachThread: Starting execution");
        g_ourThreadIds.push_back(GetCurrentThreadId());
        EVASION_LOG_SUCCESS("ATTACH", "AttachThread: Thread ID stored");
        OnAttach();
        EVASION_LOG_SUCCESS("ATTACH", "AttachThread: OnAttach completed");
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        EVASION_LOG_ERROR("ATTACH", "AttachThread: EXCEPTION CAUGHT!");
    }
    return 0;
}

static DWORD WINAPI DelayedInitThread(LPVOID) {
    __try {
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitThread: Starting execution");
        g_ourThreadIds.push_back(GetCurrentThreadId());
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitThread: Thread ID stored");
        
        // Add extra delay for full initialization
        Sleep(3000); // 3 more seconds for total 5 second delay
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitThread: Pre-initialization delay complete");
        
        DelayedInitialization();
        EVASION_LOG_SUCCESS("DELAYED", "DelayedInitThread: DelayedInitialization completed");
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        EVASION_LOG_ERROR("DELAYED", "DelayedInitThread: EXCEPTION CAUGHT!");
    }
    return 0;
}

int __stdcall DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        // Emergency crash protection
        __try {
            // Check verbosity setting from environment variable
            bool verboseMode = false; // Default to verbose
            char* envVar = nullptr;
            size_t len = 0;
            if (_dupenv_s(&envVar, &len, "AWESOME_VERBOSE") == 0 && envVar != nullptr) {
                verboseMode = (strcmp(envVar, "1") == 0 || _stricmp(envVar, "true") == 0);
                free(envVar);
            }
            
            // Initialize logging FIRST - respect verbosity setting
            EvasionLogger::Logger::Initialize(verboseMode, verboseMode); // Console + file logging based on verbosity
            if (verboseMode) {
                EVASION_LOG_SUCCESS("INIT", "DLL_PROCESS_ATTACH started");
            }
            
            DisableThreadLibraryCalls(hModule);
            
            // Store our module handle for hiding
            g_ourModule = hModule;
            EVASION_LOG_SUCCESS("INIT", "Module handle stored");
            
            // Let the game stabilize before doing anything
            Sleep(2000); // 2 second delay to let game settle
            EVASION_LOG_SUCCESS("INIT", "Post-attachment stabilization complete");
            
            // Basic attachment and hook setup
            HANDLE h1 = CreateThread(nullptr, 0, AttachThread, nullptr, 0, nullptr);
            if (h1) {
                CloseHandle(h1);
                EVASION_LOG_SUCCESS("INIT", "AttachThread started successfully");
            } else {
                EVASION_LOG_ERROR("INIT", "AttachThread creation failed");
            }
            
            // Delayed initialization thread
            HANDLE h2 = CreateThread(nullptr, 0, DelayedInitThread, nullptr, 0, nullptr);
            if (h2) {
                CloseHandle(h2);
                EVASION_LOG_SUCCESS("INIT", "DelayedInitThread started successfully");
            } else {
                EVASION_LOG_ERROR("INIT", "DelayedInitThread creation failed");
            }
            
            EVASION_LOG_SUCCESS("INIT", "DLL_PROCESS_ATTACH completed successfully");
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Log the crash if possible
            EVASION_LOG_ERROR("INIT", "CRITICAL: Exception in DllMain!");
            return 0; // Fail DLL load to prevent further crashes
        }
    }
    else if (reason == DLL_PROCESS_DETACH) {
        __try {
            EVASION_LOG_SUCCESS("CLEANUP", "DLL_PROCESS_DETACH started");
            // Cleanup advanced evasion and show final status
            AdvancedEvasion::EvasionManager::Cleanup();
            EvasionLogger::Logger::Cleanup();
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Silent cleanup failure
        }
    }
    return 1;
}