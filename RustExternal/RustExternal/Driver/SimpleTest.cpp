#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <TlHelp32.h>

// Minimal kinterface implementation for testing
HANDLE g_DriverHandle = INVALID_HANDLE_VALUE;

// CTL_CODE macro
#ifndef CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x00000022
#endif

#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED 0
#endif

#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS 0
#endif

bool InitializeVulnerableDriver() {
    // Try RTCore64 first (MSI Afterburner/RivaTuner)
    g_DriverHandle = CreateFileW(
        L"\\\\.\\RTCore64",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (g_DriverHandle != INVALID_HANDLE_VALUE) {
        printf("[+] Successfully connected to RTCore64 driver!\n");
        return true;
    }
    
    // Try alternative vulnerable drivers
    const wchar_t* drivers[] = {
        L"\\\\.\\NTIOLib_X64",  // MSI Afterburner alternative
        L"\\\\.\\EthDiag",      // Intel Ethernet Diagnostic
        L"\\\\.\\GIO",          // Gigabyte driver
        L"\\\\.\\DBUtil_2_3"    // Dell BIOS utility
    };
    
    const char* names[] = {
        "MSI Afterburner (alternative)",
        "Intel Ethernet Diagnostic", 
        "Gigabyte driver",
        "Dell BIOS utility"
    };
    
    for (int i = 0; i < 4; i++) {
        g_DriverHandle = CreateFileW(
            drivers[i],
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        
        if (g_DriverHandle != INVALID_HANDLE_VALUE) {
            printf("[+] Successfully connected to vulnerable driver: %s\n", names[i]);
            return true;
        }
    }
    
    printf("[-] No vulnerable drivers found!\n");
    printf("[-] Make sure MSI Afterburner or similar software is installed\n");
    return false;
}

// RTCore64 memory request structure
struct RTCORE64_MEMORY_REQUEST {
    DWORD64 Address;
    DWORD64 Buffer;
    DWORD Size;
};

bool ReadProcessMemoryVuln(DWORD pid, uintptr_t address, void* buffer, size_t size) {
    if (g_DriverHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    RTCORE64_MEMORY_REQUEST request = { 0 };
    request.Address = address;
    request.Buffer = (DWORD64)buffer;
    request.Size = (DWORD)size;
    
    DWORD bytesReturned = 0;
    BOOL result = DeviceIoControl(
        g_DriverHandle,
        0x80002048, // RTCore64 read memory IOCTL
        &request,
        sizeof(request),
        &request,
        sizeof(request),
        &bytesReturned,
        NULL
    );
    
    return result != FALSE;
}

int GetProcessID(const char* processName) {
    DWORD dwThreadCountMax = 0;
    DWORD foundPID = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return 0;
    }
    
    // Convert char* to wchar_t*
    wchar_t wideName[260];
    MultiByteToWideChar(CP_ACP, 0, processName, -1, wideName, 260);
    
    do {
        if (_wcsicmp((const wchar_t*)pe32.szExeFile, wideName) == 0) {
            HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hProcessSnap != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe32_count = { 0 };
                pe32_count.dwSize = sizeof(pe32_count);
                BOOL bRet = Process32First(hProcessSnap, &pe32_count);
                
                int threadCount = 0;
                while (bRet) {
                    if (pe32_count.th32ProcessID == pe32.th32ProcessID) {
                        threadCount = pe32_count.cntThreads;
                        break;
                    }
                    bRet = Process32Next(hProcessSnap, &pe32_count);
                }
                CloseHandle(hProcessSnap);
                
                if (threadCount > dwThreadCountMax) {
                    dwThreadCountMax = threadCount;
                    foundPID = pe32.th32ProcessID;
                }
            }
        }
    } while (Process32Next(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    return foundPID;
}

int main() {
    printf("=== Simple Vulnerable Driver Test ===\n\n");
    
    // Step 1: Initialize the vulnerable driver interface
    printf("[1] Initializing vulnerable driver interface...\n");
    
    if (!InitializeVulnerableDriver()) {
        printf("[-] Failed to connect to vulnerable driver!\n");
        printf("[!] Make sure you're running as Administrator\n");
        printf("[!] Make sure MSI Afterburner is installed\n");
        return 1;
    }
    
    printf("[+] Successfully connected to vulnerable driver!\n\n");
    
    // Step 2: Test memory reading on our own process
    printf("[2] Testing memory read capabilities...\n");
    
    DWORD currentPID = GetCurrentProcessId();
    printf("[*] Current Process ID: %d\n", currentPID);
    
    // Read our own DOS header to verify functionality
    HMODULE hModule = GetModuleHandle(NULL);
    printf("[*] Executable base: %p\n", hModule);
    
    IMAGE_DOS_HEADER dosHeader;
    bool success = ReadProcessMemoryVuln(
        currentPID,
        (uintptr_t)hModule,
        &dosHeader,
        sizeof(dosHeader)
    );
    
    if (success && dosHeader.e_magic == IMAGE_DOS_SIGNATURE) {
        printf("[+] SUCCESS! Memory read working - DOS signature: 0x%04X\n", dosHeader.e_magic);
        printf("[+] DOS header e_lfanew: 0x%08X\n", dosHeader.e_lfanew);
        
        // Step 3: Try to find Rust process
        printf("\n[3] Looking for target processes...\n");
        
        int rustPID = GetProcessID("RustClient.exe");
        if (rustPID > 0) {
            printf("[+] Found RustClient.exe! PID: %d\n", rustPID);
            printf("[*] Ready to read Rust memory with kernel privileges!\n");
            
            // Test reading first few bytes of Rust executable
            char buffer[16];
            if (ReadProcessMemoryVuln(rustPID, 0x140000000, buffer, sizeof(buffer))) {
                printf("[+] Successfully read Rust memory!\n");
                printf("[*] First few bytes: ");
                for (int i = 0; i < sizeof(buffer); i++) {
                    printf("%02X ", (unsigned char)buffer[i]);
                }
                printf("\n");
            } else {
                printf("[*] Could not read Rust memory at 0x140000000 (may be wrong address)\n");
            }
        } else {
            printf("[*] RustClient.exe not running\n");
        }
        
        // Test other processes
        int notepadPID = GetProcessID("notepad.exe");
        if (notepadPID > 0) {
            printf("[+] Found notepad.exe: %d\n", notepadPID);
        }
        
        int chromePID = GetProcessID("chrome.exe");
        if (chromePID > 0) {
            printf("[+] Found chrome.exe: %d\n", chromePID);
        }
        
    } else {
        printf("[-] FAILED! Memory read not working properly\n");
        printf("[-] Expected DOS signature 0x5A4D, got: 0x%04X\n", dosHeader.e_magic);
        return 1;
    }
    
    printf("\n=== SUCCESS! ===\n");
    printf("[+] Vulnerable driver interface is working!\n");
    printf("[+] You now have Ring 0 memory access without custom drivers!\n");
    printf("[+] Ready to integrate into your main application!\n");
    
    if (g_DriverHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_DriverHandle);
    }
    
    printf("\nPress any key to continue...\n");
    getchar();
    
    return 0;
} 