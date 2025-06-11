#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include <string>

int main() {
    printf("=== Process Enumeration Test ===\n\n");
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        printf("Failed to create process snapshot!\n");
        return 1;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    std::vector<std::string> rustProcesses;
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            // Convert to string for easier searching
            std::string processName = pe32.szExeFile;
            
            // Look for any Rust-related processes
            if (processName.find("rust") != std::string::npos || 
                processName.find("Rust") != std::string::npos ||
                processName.find("RUST") != std::string::npos) {
                rustProcesses.push_back(processName);
                printf("[RUST] PID: %d, Name: %s\n", pe32.th32ProcessID, pe32.szExeFile);
            }
            
            // Also show some common processes for reference
            if (processName.find("steam") != std::string::npos ||
                processName.find("Steam") != std::string::npos ||
                processName.find("game") != std::string::npos ||
                processName.find("Game") != std::string::npos) {
                printf("[GAME] PID: %d, Name: %s\n", pe32.th32ProcessID, pe32.szExeFile);
            }
            
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    
    printf("\n=== Summary ===\n");
    if (rustProcesses.empty()) {
        printf("❌ No Rust processes found!\n");
        printf("💡 Make sure Rust game is running\n");
        printf("💡 Process might be named differently (RustClient.exe, Rust.exe, etc.)\n");
    } else {
        printf("✅ Found %d Rust-related processes\n", (int)rustProcesses.size());
        for (const auto& proc : rustProcesses) {
            printf("  - %s\n", proc.c_str());
        }
    }
    
    printf("\nPress any key to exit...\n");
    getchar();
    return 0;
} 