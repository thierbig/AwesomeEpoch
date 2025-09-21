#include "AntiDetection.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <intrin.h>  // For __cpuid intrinsic

#pragma comment(lib, "Psapi.lib")

namespace AntiDetection {
    
    // Memory pattern scrambling implementation
    void MemoryScrambler::ScrambleUnusedMemory() {
        MEMORY_BASIC_INFORMATION mbi;
        LPVOID address = nullptr;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        while (VirtualQuery(address, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                
                // Fill unused areas with random data
                BYTE* ptr = (BYTE*)mbi.BaseAddress;
                for (SIZE_T i = 0; i < mbi.RegionSize; i += 4096) {
                    __try {
                        // Check if page is actually unused
                        BYTE test = ptr[i];
                        if (test == 0) {
                            // Fill with random pattern
                            for (int j = 0; j < 16; j++) {
                                ptr[i + j] = dis(gen);
                            }
                        }
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                        // Skip inaccessible pages
                    }
                }
            }
            address = (LPVOID)((DWORD_PTR)mbi.BaseAddress + mbi.RegionSize);
        }
    }

    void MemoryScrambler::RandomizeStackPatterns() {
        // Get current stack limits
        NT_TIB* tib = (NT_TIB*)NtCurrentTeb();
        LPVOID stackBase = tib->StackBase;
        LPVOID stackLimit = tib->StackLimit;
        
        // Calculate safe area to scramble
        BYTE* currentSP;
        __asm { mov currentSP, esp }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        // Scramble unused stack space
        BYTE* scrambleStart = currentSP + 0x1000; // Leave 4KB safety margin
        BYTE* scrambleEnd = (BYTE*)stackBase;
        
        for (BYTE* p = scrambleStart; p < scrambleEnd; p += 16) {
            __try {
                for (int i = 0; i < 16; i++) {
                    p[i] = dis(gen);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                break;
            }
        }
    }

    void MemoryScrambler::EraseInjectionTraces() {
        // Clear environment variables that might reveal injection
        SetEnvironmentVariableA("INJECTED_DLL", nullptr);
        SetEnvironmentVariableA("INJECTION_METHOD", nullptr);
        
        // Clear any temporary files
        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((std::wstring(tempPath) + L"inj_*.tmp").c_str(), &findData);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                DeleteFileW((std::wstring(tempPath) + findData.cFileName).c_str());
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    // Thread hiding implementation
    void ThreadHider::HideThread(DWORD threadId) {
        // Attempt to remove thread from various system structures
        HANDLE hThread = OpenThread(THREAD_SET_INFORMATION, FALSE, threadId);
        if (hThread) {
            // Set thread to lowest priority to avoid suspicion
            SetThreadPriority(hThread, THREAD_PRIORITY_IDLE);
            
            // Hide from debugger
            typedef LONG NTSTATUS;  // Define NTSTATUS if not already defined
            typedef NTSTATUS(WINAPI* NtSetInformationThread_t)(HANDLE, ULONG, PVOID, ULONG);
            NtSetInformationThread_t NtSetInformationThread = (NtSetInformationThread_t)
                GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");
            
            if (NtSetInformationThread) {
                ULONG hideFromDebugger = 0;
                NtSetInformationThread(hThread, 0x11, &hideFromDebugger, sizeof(ULONG));
            }
            
            CloseHandle(hThread);
        }
    }

    void ThreadHider::RandomizeThreadStack() {
        // Allocate random amount of stack space to change stack patterns
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(64, 4096);
        
        volatile BYTE* stackBuffer = (BYTE*)_alloca(dis(gen));
        
        // Fill with random data
        for (int i = 0; i < dis(gen); i++) {
            stackBuffer[i] = dis(gen) & 0xFF;
        }
    }

    void ThreadHider::SpoofThreadStartAddress(LPVOID newAddress) {
        // This would require kernel-level access in practice
        // Here we just document the technique
        
        // In a real implementation, you would:
        // 1. Hook NtQueryInformationThread
        // 2. Intercept ThreadQuerySetWin32StartAddress queries
        // 3. Return spoofed address instead of real start address
    }

    // Anti-VM and anti-sandbox checks
    bool EnvironmentChecker::IsRunningInVM() {
        // Check for VM artifacts
        
        // 1. Check CPUID hypervisor bit
        int cpuInfo[4] = { -1 };
        __cpuid(cpuInfo, 1);
        if ((cpuInfo[2] >> 31) & 1) {
            return true; // Hypervisor bit set
        }
        
        // 2. Check for VM-specific registry keys
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
            "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true; // VirtualBox detected
        }
        
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\VMware, Inc.\\VMware Tools", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true; // VMware detected
        }
        
        // 3. Check for VM-specific files
        if (GetFileAttributesA("C:\\Windows\\System32\\drivers\\vmmouse.sys") != INVALID_FILE_ATTRIBUTES ||
            GetFileAttributesA("C:\\Windows\\System32\\drivers\\vmhgfs.sys") != INVALID_FILE_ATTRIBUTES) {
            return true; // VMware files found
        }
        
        // 4. Check MAC address prefixes
        // VM vendors use specific MAC prefixes
        // This would require more complex implementation
        
        return false;
    }

    bool EnvironmentChecker::IsRunningInSandbox() {
        // Check for sandbox indicators
        
        // 1. Check for limited user privileges
        HANDLE hToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD size = sizeof(elevation);
            if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) {
                if (!elevation.TokenIsElevated) {
                    // Running with limited privileges (possible sandbox)
                }
            }
            CloseHandle(hToken);
        }
        
        // 2. Check for sandbox-specific DLLs
        if (GetModuleHandleA("SbieDll.dll") != nullptr) {
            return true; // Sandboxie detected
        }
        
        if (GetModuleHandleA("cuckoomon.dll") != nullptr) {
            return true; // Cuckoo Sandbox detected
        }
        
        // 3. Check for unusually low number of processes
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            int processCount = 0;
            
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    processCount++;
                } while (Process32Next(hSnapshot, &pe32));
            }
            
            CloseHandle(hSnapshot);
            
            if (processCount < 20) {
                return true; // Suspiciously few processes
            }
        }
        
        // 4. Check for fast sleep skip (sandbox acceleration)
        DWORD startTime = GetTickCount();
        Sleep(500);
        DWORD elapsed = GetTickCount() - startTime;
        
        if (elapsed < 450) {
            return true; // Sleep was skipped/accelerated
        }
        
        return false;
    }

    bool EnvironmentChecker::HasSuspiciousHardware() {
        // Check for analysis/debugging hardware
        
        // 1. Check CPU core count
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        if (sysInfo.dwNumberOfProcessors < 2) {
            return true; // Single core is suspicious in modern systems
        }
        
        // 2. Check available RAM
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        GlobalMemoryStatusEx(&memStatus);
        
        if (memStatus.ullTotalPhys < (2ULL * 1024 * 1024 * 1024)) {
            return true; // Less than 2GB RAM is suspicious
        }
        
        // 3. Check disk size
        ULARGE_INTEGER totalBytes;
        if (GetDiskFreeSpaceExA("C:\\", nullptr, &totalBytes, nullptr)) {
            if (totalBytes.QuadPart < (60ULL * 1024 * 1024 * 1024)) {
                return true; // Less than 60GB disk is suspicious
            }
        }
        
        return false;
    }

    bool EnvironmentChecker::CheckCPUFeatures() {
        // Check for expected CPU features
        int cpuInfo[4];
        
        // Check for SSE2 support (expected in modern CPUs)
        __cpuid(cpuInfo, 1);
        if (!(cpuInfo[3] & (1 << 26))) {
            return false; // No SSE2 support is suspicious
        }
        
        // Check for RDRAND support (common in modern CPUs)
        __cpuid(cpuInfo, 1);
        if (!(cpuInfo[2] & (1 << 30))) {
            // RDRAND not supported - might be older VM
        }
        
        return true;
    }

    // Polymorphic code generation
    std::vector<BYTE> CodeMutator::GenerateJunkCode(size_t size) {
        std::vector<BYTE> junk;
        junk.reserve(size);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 5);
        
        while (junk.size() < size) {
            switch (dis(gen)) {
            case 0: // NOP
                junk.push_back(0x90);
                break;
            case 1: // MOV reg, reg (same register)
                junk.push_back(0x89);
                junk.push_back(0xC0 + (dis(gen) & 7) * 9); // MOV EAX,EAX etc
                break;
            case 2: // PUSH + POP
                junk.push_back(0x50 + (dis(gen) & 7)); // PUSH reg
                junk.push_back(0x58 + (dis(gen) & 7)); // POP reg
                break;
            case 3: // XOR reg, 0
                junk.push_back(0x31);
                junk.push_back(0xC0 + (dis(gen) & 7) * 9);
                break;
            case 4: // JMP +0
                junk.push_back(0xEB);
                junk.push_back(0x00);
                break;
            case 5: // CLC, STC, CLD, STD
                junk.push_back(0xF8 + (dis(gen) & 3));
                break;
            }
        }
        
        return junk;
    }

    void CodeMutator::InsertDeadCode() {
        // This would be inserted at compile time with macros
        // Example dead code that never executes
        
        volatile int dummy = 0;
        if (dummy == 12345) {
            // Never executed
            for (int i = 0; i < 1000; i++) {
                dummy += i * i;
            }
        }
        
        // Opaque predicates
        int x = rand();
        int y = rand();
        if ((x * x) + (y * y) >= 0) {
            // Always true for real numbers
            // Real code here
        } else {
            // Never executed
            dummy = 99999;
        }
    }

    void CodeMutator::ObfuscateControlFlow() {
        // Control flow flattening example
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 5);
        
        int state = dis(gen);
        bool done = false;
        
        while (!done) {
            switch (state) {
            case 1:
                // Do something
                state = 3;
                break;
            case 2:
                // Do something else
                state = 4;
                break;
            case 3:
                // Another action
                state = 2;
                break;
            case 4:
                // More actions
                state = 5;
                break;
            case 5:
                done = true;
                break;
            }
        }
    }

    // Timing obfuscation
    void TimingObfuscator::AddRandomDelays() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 10);
        
        // Random micro-delays
        std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
        
        // Random busy wait
        volatile int dummy = 0;
        for (int i = 0; i < dis(gen) * 100; i++) {
            dummy += i;
        }
    }

    void TimingObfuscator::NormalizeExecutionTime() {
        static auto lastExecutionTime = std::chrono::high_resolution_clock::now();
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = now - lastExecutionTime;
        
        // Target consistent 16ms intervals (60 FPS)
        const auto targetInterval = std::chrono::milliseconds(16);
        
        if (elapsed < targetInterval) {
            std::this_thread::sleep_for(targetInterval - elapsed);
        }
        
        lastExecutionTime = std::chrono::high_resolution_clock::now();
    }

    void TimingObfuscator::InsertDummyOperations() {
        // CPU-intensive dummy operations to mask real operations
        volatile double result = 0;
        for (int i = 1; i < 100; i++) {
            result += sqrt((double)i) * sin((double)i);
        }
        
        // Memory-intensive dummy operations
        std::vector<int> dummy(1000);
        for (int i = 0; i < 1000; i++) {
            dummy[i] = i * i;
        }
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(dummy.begin(), dummy.end(), g);
    }

    // API resolution by hash
    DWORD HashString(const char* str) {
        DWORD hash = 0x811C9DC5;
        while (*str) {
            hash ^= (DWORD)*str++;
            hash *= 0x01000193;
        }
        return hash;
    }

    FARPROC APIResolver::ResolveAPIHash(DWORD moduleHash, DWORD functionHash) {
        // Enumerate all modules
        HMODULE hMods[1024];
        DWORD cbNeeded;
        
        if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                char szModName[MAX_PATH];
                if (GetModuleFileNameA(hMods[i], szModName, sizeof(szModName))) {
                    // Extract just the module name
                    char* moduleName = strrchr(szModName, '\\');
                    if (moduleName) moduleName++;
                    else moduleName = szModName;
                    
                    if (HashString(moduleName) == moduleHash) {
                        // Found the module, now search for function
                        IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)hMods[i];
                        IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);
                        IMAGE_EXPORT_DIRECTORY* exportDir = (IMAGE_EXPORT_DIRECTORY*)((BYTE*)dosHeader +
                            ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
                        
                        DWORD* names = (DWORD*)((BYTE*)dosHeader + exportDir->AddressOfNames);
                        for (DWORD j = 0; j < exportDir->NumberOfNames; j++) {
                            char* funcName = (char*)((BYTE*)dosHeader + names[j]);
                            if (HashString(funcName) == functionHash) {
                                WORD* ordinals = (WORD*)((BYTE*)dosHeader + exportDir->AddressOfNameOrdinals);
                                DWORD* functions = (DWORD*)((BYTE*)dosHeader + exportDir->AddressOfFunctions);
                                return (FARPROC)((BYTE*)dosHeader + functions[ordinals[j]]);
                            }
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    // Integrity monitoring
    static std::unordered_map<HMODULE, DWORD> g_moduleChecksums;

    DWORD CalculateChecksum(LPVOID data, SIZE_T size) {
        DWORD checksum = 0;
        BYTE* bytes = (BYTE*)data;
        for (SIZE_T i = 0; i < size; i++) {
            checksum = ((checksum << 1) | (checksum >> 31)) ^ bytes[i];
        }
        return checksum;
    }

    void IntegrityMonitor::CalculateModuleChecksums() {
        HMODULE hMods[1024];
        DWORD cbNeeded;
        
        if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                MODULEINFO modInfo;
                if (GetModuleInformation(GetCurrentProcess(), hMods[i], &modInfo, sizeof(modInfo))) {
                    DWORD checksum = CalculateChecksum(modInfo.lpBaseOfDll, 
                        min(modInfo.SizeOfImage, 0x1000)); // Check first page
                    g_moduleChecksums[hMods[i]] = checksum;
                }
            }
        }
    }

    bool IntegrityMonitor::VerifyIntegrity() {
        for (auto it = g_moduleChecksums.begin(); it != g_moduleChecksums.end(); ++it) {
            HMODULE hModule = it->first;
            DWORD originalChecksum = it->second;
            MODULEINFO modInfo;
            if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) {
                DWORD currentChecksum = CalculateChecksum(modInfo.lpBaseOfDll,
                    min(modInfo.SizeOfImage, (SIZE_T)0x1000));
                if (currentChecksum != originalChecksum) {
                    return false;
                }
            }
        }
        return true;
    }
}
