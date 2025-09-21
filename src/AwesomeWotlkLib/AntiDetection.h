#pragma once
#include <Windows.h>
#include <vector>
#include <random>
#include <string>
#include <unordered_map>
#include <TlHelp32.h>
#include <Psapi.h>
#include <algorithm>

namespace AntiDetection {
    // Memory pattern scrambling
    class MemoryScrambler {
    public:
        static void ScrambleUnusedMemory();
        static void RandomizeStackPatterns();
        static void EraseInjectionTraces();
    };

    // Advanced thread hiding
    class ThreadHider {
    public:
        static void HideThread(DWORD threadId);
        static void RandomizeThreadStack();
        static void SpoofThreadStartAddress(LPVOID newAddress);
    };

    // System call hooking at lower level
    class SyscallHooker {
    public:
        static void HookNtQuerySystemInformation();
        static void HookNtQueryVirtualMemory();
        static void HookNtOpenProcess();
    };

    // Polymorphic code generation
    class CodeMutator {
    public:
        static std::vector<BYTE> GenerateJunkCode(size_t size);
        static void InsertDeadCode();
        static void ObfuscateControlFlow();
    };

    // Anti-VM and anti-sandbox
    class EnvironmentChecker {
    public:
        static bool IsRunningInVM();
        static bool IsRunningInSandbox();
        static bool HasSuspiciousHardware();
        static bool CheckCPUFeatures();
    };

    // Dynamic API resolution
    class APIResolver {
    public:
        static FARPROC ResolveAPIHash(DWORD moduleHash, DWORD functionHash);
        static HMODULE GetModuleByHash(DWORD hash);
        static void LoadAPIsIndirectly();
    };

    // Integrity checks
    class IntegrityMonitor {
    public:
        static void CalculateModuleChecksums();
        static bool VerifyIntegrity();
        static void SetupIntegrityCallbacks();
    };

    // Timing attack prevention
    class TimingObfuscator {
    public:
        static void AddRandomDelays();
        static void NormalizeExecutionTime();
        static void InsertDummyOperations();
    };

    // Advanced string encryption
    template<typename T, size_t N>
    class EncryptedString {
    private:
        T data[N];
        T key;
        
    public:
        constexpr EncryptedString(const T* str, T k) : key(k) {
            for (size_t i = 0; i < N; ++i) {
                data[i] = str[i] ^ key;
            }
        }
        
        std::basic_string<T> decrypt() const {
            std::basic_string<T> result;
            for (size_t i = 0; i < N; ++i) {
                result += data[i] ^ key;
            }
            return result;
        }
    };
}
