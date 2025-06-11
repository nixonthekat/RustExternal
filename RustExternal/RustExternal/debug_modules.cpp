#include <Windows.h>
#include <psapi.h>
#include <iostream>
#include <TlHelp32.h>
#include <string>

int main() {
    printf("=== RustClient.exe Module Debug Tool ===\n");
    
    // Find RustClient.exe
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    Process32FirstW(hSnapshot, &pe32);
    
    DWORD targetPID = 0;
    do {
        if (_wcsicmp(pe32.szExeFile, L"RustClient.exe") == 0) {
            targetPID = pe32.th32ProcessID;
            break;
        }
    } while (Process32NextW(hSnapshot, &pe32));
    CloseHandle(hSnapshot);
    
    if (targetPID == 0) {
        printf("❌ RustClient.exe not found!\n");
        system("pause");
        return 1;
    }
    
    printf("✅ Found RustClient.exe PID: %d\n", targetPID);
    
    // Open process
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetPID);
    if (hProcess == NULL) {
        printf("❌ Failed to open process (Error: %d)\n", GetLastError());
        printf("💡 Try running as Administrator\n");
        system("pause");
        return 1;
    }
    
    // Enumerate modules
    HMODULE hMods[1024];
    DWORD cbNeeded;
    
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        DWORD moduleCount = cbNeeded / sizeof(HMODULE);
        printf("📋 Found %d modules:\n\n", moduleCount);
        
        for (DWORD i = 0; i < moduleCount; i++) {
            WCHAR moduleName[MAX_PATH];
            WCHAR moduleFullPath[MAX_PATH];
            
            // Get module name
            if (GetModuleBaseNameW(hProcess, hMods[i], moduleName, sizeof(moduleName)/sizeof(WCHAR))) {
                // Get full path
                if (GetModuleFileNameExW(hProcess, hMods[i], moduleFullPath, sizeof(moduleFullPath)/sizeof(WCHAR))) {
                    printf("[%2d] %-30ls @ 0x%016llx\n", i, moduleName, (uintptr_t)hMods[i]);
                    printf("     Path: %ls\n", moduleFullPath);
                } else {
                    printf("[%2d] %-30ls @ 0x%016llx\n", i, moduleName, (uintptr_t)hMods[i]);
                }
            }
            printf("\n");
        }
        
        // Specifically search for Unity/GameAssembly related modules
        printf("🔍 Searching for Unity/GameAssembly modules:\n");
        for (DWORD i = 0; i < moduleCount; i++) {
            WCHAR moduleName[MAX_PATH];
            if (GetModuleBaseNameW(hProcess, hMods[i], moduleName, sizeof(moduleName)/sizeof(WCHAR))) {
                std::wstring name(moduleName);
                std::transform(name.begin(), name.end(), name.begin(), ::towlower);
                
                if (name.find(L"gameassembly") != std::wstring::npos ||
                    name.find(L"unity") != std::wstring::npos ||
                    name.find(L"il2cpp") != std::wstring::npos ||
                    name.find(L"mono") != std::wstring::npos) {
                    printf("🎯 FOUND: %ls @ 0x%016llx\n", moduleName, (uintptr_t)hMods[i]);
                }
            }
        }
        
    } else {
        printf("❌ EnumProcessModules failed (Error: %d)\n", GetLastError());
        printf("💡 This indicates EAC is blocking module enumeration\n");
    }
    
    CloseHandle(hProcess);
    printf("\nPress any key to exit...\n");
    system("pause");
    return 0;
} 