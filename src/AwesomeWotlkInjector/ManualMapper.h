#pragma once
#include <Windows.h>
#include <vector>
#include <string>

// External verbosity control from Injector.cpp
extern const bool VERBOSE_MODE;

class ManualMapper {
public:
    // Manual mapping without CreateRemoteThread
    static bool InjectDLL(DWORD processId, const std::wstring& dllPath);
    
private:
    // PE structure parsing
    static bool ParsePEHeaders(const std::vector<BYTE>& dllData, 
                              IMAGE_DOS_HEADER*& dosHeader,
                              IMAGE_NT_HEADERS*& ntHeaders);
    
    // Memory allocation techniques
    static LPVOID AllocateMemoryStealthily(HANDLE hProcess, SIZE_T size);
    
    // Section mapping
    static bool MapSections(HANDLE hProcess, LPVOID baseAddress, 
                           const std::vector<BYTE>& dllData,
                           IMAGE_NT_HEADERS* ntHeaders);
    
    // Relocation processing
    static bool ProcessRelocations(HANDLE hProcess, LPVOID baseAddress,
                                  IMAGE_NT_HEADERS* ntHeaders,
                                  DWORD_PTR deltaBase,
                                  const std::vector<BYTE>& dllData);
    
    // Import resolution
    static bool ResolveImports(HANDLE hProcess, LPVOID baseAddress,
                              IMAGE_NT_HEADERS* ntHeaders);
    
    // TLS callback handling
    static bool HandleTLSCallbacks(HANDLE hProcess, LPVOID baseAddress,
                                  IMAGE_NT_HEADERS* ntHeaders);
    
    // Execution techniques (alternatives to CreateRemoteThread)
    static bool ExecuteViaThreadHijacking(HANDLE hProcess, LPVOID entryPoint);
    static bool ExecuteViaAPCInjection(HANDLE hProcess, LPVOID entryPoint);
    static bool ExecuteViaSetWindowsHookEx(HANDLE hProcess, LPVOID entryPoint);
    static bool ExecuteViaQueueUserAPC(HANDLE hProcess, LPVOID entryPoint);
    
    // Cleanup and anti-forensics
    static void EraseHeaders(HANDLE hProcess, LPVOID baseAddress);
    static void UnlinkFromPEB(HANDLE hProcess, LPVOID baseAddress);
};
