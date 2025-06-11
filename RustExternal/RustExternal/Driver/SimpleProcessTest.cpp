#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

int main() {
    printf("=== Simple PID Test ===\n");
    
    const char* targetProcess = "RustClient.exe";
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    // Convert to wide char
    wchar_t wideName[260];
    MultiByteToWideChar(CP_ACP, 0, targetProcess, -1, wideName, 260);
    
    printf("Looking for: %s\n", targetProcess);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            // Debug: check every process
            if (strstr(pe32.szExeFile, "Rust") != NULL) {
                printf("Found Rust process: %s (PID: %d)\n", pe32.szExeFile, pe32.th32ProcessID);
            }
            
            // Test string comparison
            if (_wcsicmp((const wchar_t*)pe32.szExeFile, wideName) == 0) {
                printf("✅ MATCH FOUND!\n");
                printf("   Process: %s\n", pe32.szExeFile);
                printf("   PID: %d\n", pe32.th32ProcessID);
                printf("   Threads: %d\n", pe32.cntThreads);
                CloseHandle(hSnapshot);
                return 0;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    printf("❌ No match found!\n");
    return 1;
} 