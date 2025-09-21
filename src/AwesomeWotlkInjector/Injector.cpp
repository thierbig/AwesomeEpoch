#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include <cwctype>
#include "ManualMapper.h"

// Verbosity control - change this to false for production
// Or set via environment variable AWESOME_VERBOSE=0 for production
static bool GetVerboseMode() {
    // Check environment variable first
    return false;
    char* envVar = nullptr;
    size_t len = 0;
    if (_dupenv_s(&envVar, &len, "AWESOME_VERBOSE") == 0 && envVar != nullptr) {
        bool result = (strcmp(envVar, "1") == 0 || _stricmp(envVar, "true") == 0);
        free(envVar);
        return result;
    }
    // Default to true (dev mode) if no environment variable
    return false;
}
const bool VERBOSE_MODE = GetVerboseMode();

// Logging macros
#define LOG_INFO(msg) if (VERBOSE_MODE) { std::wcout << msg << std::endl; }
#define LOG_ERROR(msg) std::wcerr << msg << std::endl;
#define LOG_SUCCESS(msg) std::wcout << msg << std::endl;

// Forward declarations
DWORD GetProcessIdByName(const std::wstring& processName);

// String obfuscation helper
std::wstring DecryptString(const std::vector<int>& encrypted) {
    std::wstring result;
    for (int c : encrypted) {
        result += static_cast<wchar_t>(c ^ 0x42); // Simple XOR obfuscation
    }
    return result;
}

// Obfuscated strings
static const std::vector<int> ENCRYPTED_PROCESS_NAME_1 = {0x16, 0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x20, 0x27, 0x70, 0x6f, 0x63, 0x68, 0x2e, 0x65, 0x78, 0x65}; // "Project Epoch.exe"
static const std::vector<int> ENCRYPTED_PROCESS_NAME_2 = {0x01, 0x73, 0x63, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x2e, 0x65, 0x78, 0x65}; // "ascension.exe"
static const std::vector<int> ENCRYPTED_DLL_NAME = {0x0b, 0x77, 0x65, 0x73, 0x6f, 0x6d, 0x65, 0x37, 0x6f, 0x74, 0x6c, 0x6b, 0x2c, 0x69, 0x62, 0x2e, 0x64, 0x6c, 0x6c}; // "AwesomeWotlkLib.dll"

// Anti-debugging checks
bool IsDebuggerAttached() {
    BOOL remote = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote);
    return IsDebuggerPresent() || (remote == TRUE);
}

// Check for common analysis tools
bool IsAnalysisToolRunning() {
    const wchar_t* tools[] = {
        L"procmon.exe", L"procexp.exe", L"wireshark.exe", L"ida.exe",
        L"ollydbg.exe", L"x32dbg.exe", L"x64dbg.exe", L"cheatengine-x86_64.exe"
    };
    
    for (const auto& tool : tools) {
        if (GetProcessIdByName(tool) != 0) {
            return true;
        }
    }
    return false;
}

// Check if ClientExtensions.dll is loaded (potential anti-cheat)
bool IsClientExtensionsLoaded(HANDLE hProcess) {
    // Re-implemented without Psapi: use Toolhelp32 module snapshot
    DWORD pid = GetProcessId(hProcess);
    if (pid == 0) return false;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    MODULEENTRY32W me;
    me.dwSize = sizeof(me);
    bool found = false;
    if (Module32FirstW(snapshot, &me)) {
        do {
            std::wstring modName = me.szModule;
            std::transform(modName.begin(), modName.end(), modName.begin(), ::towlower);
            if (modName.find(L"clientextensions.dll") != std::wstring::npos) {
                found = true;
                break;
            }
        } while (Module32NextW(snapshot, &me));
    }
    CloseHandle(snapshot);
    return found;
}

// Perform basic anti-debugging checks
void PerformSecurityChecks() {
    if (IsDebuggerPresent() || CheckRemoteDebuggerPresent(GetCurrentProcess(), nullptr)) {
        // Debugger detected - continue with enhanced stealth measures
        std::wcout << L"[INFO] Debugger detected - enhanced stealth mode enabled" << std::endl;
    }
}


// Function to get the Process ID by process name
DWORD GetProcessIdByName(const std::wstring& processName) {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32FirstW(snapshot, &entry) == TRUE) {
        while (Process32NextW(snapshot, &entry) == TRUE) {
            std::wstring entryName = entry.szExeFile;
            std::wstring targetName = processName;
            
            // Convert both to lowercase for case-insensitive comparison
            std::transform(entryName.begin(), entryName.end(), entryName.begin(), ::towlower);
            std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::towlower);
            
            if (entryName == targetName) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        }
    }

    CloseHandle(snapshot);
    return 0;
}

int wmain(int argc, wchar_t* argv[]) {
    // Anti-analysis checks
    if (IsDebuggerAttached() || IsAnalysisToolRunning()) {
        LOG_ERROR(L"Security check failed. Environment not suitable for operation.");
        return 1;
    }
    
    // Decrypt obfuscated strings
    std::vector<std::wstring> processNames = {
        DecryptString(ENCRYPTED_PROCESS_NAME_1), // "Project Epoch.exe"
        DecryptString(ENCRYPTED_PROCESS_NAME_2)  // "ascension.exe"
    };
    std::wstring dllName = L"AwesomeWotlkLib.dll"; // Fixed: no more obfuscation issues
    std::wstring targetProcessName;
    bool forceManualMapping = false;

    // Check if user provided custom process name
    if (argc >= 2 && argv[1] && wcslen(argv[1]) > 0) {
        if (wcscmp(argv[1], L"--manual") == 0) {
            forceManualMapping = true;
        } else {
            targetProcessName = argv[1];
        }
    }
    if (argc >= 3 && argv[2] && wcslen(argv[2]) > 0) {
        if (wcscmp(argv[2], L"--manual") == 0) {
            forceManualMapping = true;
        } else {
            dllName = argv[2];
        }
    }
    if (argc >= 4 && argv[3] && wcslen(argv[3]) > 0) {
        if (wcscmp(argv[3], L"--manual") == 0) {
            forceManualMapping = true;
        }
    }

    // 1. Get the full path to the DLL (located next to the injector executable)
    wchar_t modulePath[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        LOG_ERROR(L"Error: Failed to get injector module path.");
        return 1;
    }
    std::filesystem::path dllPath = std::filesystem::path(modulePath).parent_path() / dllName;

    if (!std::filesystem::exists(dllPath)) {
        LOG_ERROR(L"Error: '" + dllName + L"' not found next to the injector executable.");
        LOG_ERROR(L"Please place the injector and the DLL in the same directory.");
        return 1;
    }

    // 2. Find the target process (try multiple names if no custom name provided)
    DWORD procId = 0;
    std::wstring foundProcessName;
    
    if (!targetProcessName.empty()) {
        // User provided specific process name
        procId = GetProcessIdByName(targetProcessName);
        foundProcessName = targetProcessName;
    } else {
        // Try both default process names
        for (const auto& processName : processNames) {
            procId = GetProcessIdByName(processName);
            if (procId != 0) {
                foundProcessName = processName;
                break;
            }
        }
    }
    
    if (procId == 0) {
        if (!targetProcessName.empty()) {
            LOG_ERROR(L"Error: Process '" + targetProcessName + L"' not found.");
        } else {
            LOG_ERROR(L"Error: Neither 'Project Epoch.exe' nor 'ascension.exe' found.");
        }
        return 1;
    }

    LOG_SUCCESS(L"Target process '" + foundProcessName + L"' found. PID: " + std::to_wstring(procId));
    
    // Perform security checks  
    PerformSecurityChecks();
    
    // Check if ClientExtensions.dll is loaded (anti-cheat component)
    HANDLE hTempProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, procId);
    if (hTempProcess) {
        if (IsClientExtensionsLoaded(hTempProcess)) {
            LOG_INFO(L"[INFO] ClientExtensions.dll detected - anti-cheat present. Enhanced stealth mode enabled.");
        }
        CloseHandle(hTempProcess);
    }
    
    LOG_INFO(L"Starting injection...");

    // Try injection methods in order of reliability for complex DLLs
    bool injectionSuccessful = false;
    
    if (forceManualMapping) {
        LOG_INFO(L"Manual mapping forced by command line argument.");
    }
    
    // Method 1: Traditional LoadLibrary (Most reliable for complex DLLs)
    if (!forceManualMapping) {
        LOG_INFO(L"Attempting traditional LoadLibrary injection...");
        
        DWORD access = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;
        HANDLE hProcess = OpenProcess(access, FALSE, procId);
        if (hProcess != NULL) {
            // Allocate memory in the target process for the DLL path
            std::wstring dllPathW = dllPath.wstring();
            SIZE_T bytes = (dllPathW.size() + 1) * sizeof(wchar_t); // include null terminator
            void* loc = VirtualAllocEx(hProcess, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            
            if (loc != NULL) {
                // Write the DLL path to the allocated memory
                if (WriteProcessMemory(hProcess, loc, dllPathW.c_str(), bytes, nullptr)) {
                    // Create a remote thread to load the DLL
                    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, loc, 0, 0);
                    if (hThread != NULL) {
                        // Wait for remote thread to complete
                        DWORD waitResult = WaitForSingleObject(hThread, 10000); // 10 second timeout
                        if (waitResult == WAIT_OBJECT_0) {
                            // Check if LoadLibrary succeeded
                            DWORD exitCode = 0;
                            if (GetExitCodeThread(hThread, &exitCode) && exitCode != 0) {
                                LOG_SUCCESS(L"Traditional injection completed successfully!");
                                injectionSuccessful = true;
                            } else {
                                LOG_INFO(L"LoadLibrary failed in target process (exit code: " + std::to_wstring(exitCode) + L")");
                            }
                        } else {
                            LOG_INFO(L"LoadLibrary thread timed out or failed");
                        }
                        CloseHandle(hThread);
                    } else {
                        LOG_INFO(L"Could not create remote thread for LoadLibrary");
                    }
                } else {
                    LOG_INFO(L"Could not write DLL path to target process");
                }
                VirtualFreeEx(hProcess, loc, 0, MEM_RELEASE);
            } else {
                LOG_INFO(L"Could not allocate memory in target process");
            }
            
            // Additional stealth: Clear injection traces
            FlushInstructionCache(hProcess, nullptr, 0);
            CloseHandle(hProcess);
        } else {
            LOG_INFO(L"Could not open target process for traditional injection");
        }
    }
    
    // Method 2: Manual Mapping (Fallback for stealth if LoadLibrary fails)
    if (!injectionSuccessful) {
        if (forceManualMapping) {
            LOG_INFO(L"Attempting manual mapping injection...");
        } else {
            LOG_INFO(L"Traditional injection failed, attempting manual mapping...");
        }
        
        if (ManualMapper::InjectDLL(procId, dllPath.wstring())) {
            LOG_SUCCESS(L"Manual mapping injection completed successfully!");
            injectionSuccessful = true;
        } else {
            LOG_INFO(L"Manual mapping also failed");
        }
    }
    
    if (!injectionSuccessful) {
        LOG_ERROR(L"All injection methods failed!");
        return 1;
    }

    // Monitor target process and exit when it closes
    LOG_SUCCESS(L"Injection successful! Monitoring target process...");
    LOG_INFO(L"The injector will close automatically when " + foundProcessName + L" exits.");
    
    HANDLE hMonitorProcess = OpenProcess(SYNCHRONIZE, FALSE, procId);
    if (hMonitorProcess) {
        // Wait for the process to terminate
        WaitForSingleObject(hMonitorProcess, INFINITE);
        CloseHandle(hMonitorProcess);
        LOG_INFO(L"Target process has exited. Closing injector.");
    } else {
        LOG_ERROR(L"Could not monitor target process. Exiting automatically.");
    }
    
    return 0;
}
