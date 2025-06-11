#include <Windows.h>
#include <iostream>

int main() {
    printf("=== RTCore64 Device Name Testing ===\n\n");
    
    // Try different device names that RTCore64 might use
    const wchar_t* deviceNames[] = {
        L"\\\\.\\RTCore64",
        L"\\\\.\\GlobalRoot\\Device\\RTCore64",
        L"\\\\.\\RTCore",
        L"\\\\.\\GlobalRoot\\Device\\RTCore",
        L"\\\\.\\RTCORE64",
        L"\\\\.\\MsIo",
        L"\\\\.\\MSIO64"
    };
    
    for (int i = 0; i < 7; i++) {
        printf("[*] Testing device: %ls\n", deviceNames[i]);
        
        HANDLE hDevice = CreateFileW(deviceNames[i],
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        
        if (hDevice != INVALID_HANDLE_VALUE) {
            printf("[+] SUCCESS! Connected to %ls\n", deviceNames[i]);
            CloseHandle(hDevice);
        } else {
            DWORD error = GetLastError();
            printf("[-] Failed: Error %d\n", error);
        }
        printf("\n");
    }
    
    // Check if MSI Afterburner process exists
    printf("=== Checking for MSI Afterburner processes ===\n");
    system("tasklist | findstr -i msi");
    system("tasklist | findstr -i afterburner");
    
    printf("\n=== Driver Status ===\n");
    system("sc query RTCore64");
    
    system("pause");
    return 0;
} 