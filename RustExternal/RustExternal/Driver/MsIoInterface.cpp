#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>

// MsIo64.sys IOCTL codes for CVE-2019-18845
#define MSIO_CTL_CODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN, i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_MSIO_MAPPHYS     MSIO_CTL_CODE(0x803)
#define IOCTL_MSIO_UNMAPPHYS   MSIO_CTL_CODE(0x804)

// MsIo data structures
typedef struct _MSIO_PHYSICAL_MEMORY {
    LARGE_INTEGER PhysicalAddress;
    DWORD Size;
    HANDLE SectionHandle;
    PVOID VirtualAddress;
} MSIO_PHYSICAL_MEMORY, *PMSIO_PHYSICAL_MEMORY;

class MsIoInterface {
private:
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    DWORD rustPID = 0;
    
public:
    bool Initialize() {
        printf("=== MsIo64.sys CVE-2019-18845 Interface ===\n\n");
        
        // Find RustClient.exe
        rustPID = FindProcessByName("RustClient.exe");
        if (rustPID == 0) {
            printf("[-] RustClient.exe not found!\n");
            return false;
        }
        printf("[+] Found RustClient.exe (PID: %d)\n", rustPID);
        
        // Try to connect to MsIo driver
        hDriver = CreateFileW(L"\\\\.\\MsIo", 
                             GENERIC_READ | GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING, 
                             FILE_ATTRIBUTE_NORMAL, NULL);
        
        if (hDriver == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            printf("[-] Failed to connect to MsIo driver (Error: %d)\n", error);
            printf("[!] Make sure MsIo64.sys driver is available\n");
            printf("[*] This driver is often found on ASRock motherboards\n");
            return false;
        }
        
        printf("[+] Successfully connected to MsIo driver!\n");
        return true;
    }
    
    bool MapPhysicalMemory(LARGE_INTEGER physAddr, DWORD size, PVOID* mappedAddr) {
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        
        MSIO_PHYSICAL_MEMORY mapRequest = {0};
        mapRequest.PhysicalAddress = physAddr;
        mapRequest.Size = size;
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     IOCTL_MSIO_MAPPHYS,
                                     &mapRequest,
                                     sizeof(mapRequest),
                                     &mapRequest,
                                     sizeof(mapRequest),
                                     &bytesReturned,
                                     NULL);
        
        if (!result) {
            printf("[-] MsIo MapPhysicalMemory failed (Error: %d)\n", GetLastError());
            return false;
        }
        
        *mappedAddr = mapRequest.VirtualAddress;
        printf("[+] Mapped physical 0x%llX to virtual %p\n", physAddr.QuadPart, *mappedAddr);
        return true;
    }
    
    bool UnmapPhysicalMemory(HANDLE sectionHandle) {
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        
        MSIO_PHYSICAL_MEMORY unmapRequest = {0};
        unmapRequest.SectionHandle = sectionHandle;
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     IOCTL_MSIO_UNMAPPHYS,
                                     &unmapRequest,
                                     sizeof(unmapRequest),
                                     &unmapRequest,
                                     sizeof(unmapRequest),
                                     &bytesReturned,
                                     NULL);
        
        return result != FALSE;
    }
    
    void TestMsIoCapabilities() {
        printf("\n=== Testing MsIo Physical Memory Access ===\n");
        
        // Test mapping low physical memory (safe regions)
        LARGE_INTEGER testPhysAddr;
        testPhysAddr.QuadPart = 0x1000; // 4KB physical address
        
        PVOID mappedAddr = nullptr;
        if (MapPhysicalMemory(testPhysAddr, 0x1000, &mappedAddr)) {
            printf("[+] Successfully mapped physical memory!\n");
            printf("[*] This means we can bypass EAC using physical memory access\n");
            
            // Read some data from mapped memory
            if (mappedAddr) {
                DWORD* dataPtr = (DWORD*)mappedAddr;
                printf("[+] Read from mapped memory: 0x%08X\n", *dataPtr);
            }
        } else {
            printf("[-] Failed to map physical memory\n");
        }
    }
    
    DWORD FindProcessByName(const char* processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
        
        PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
        DWORD pid = 0;
        
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (_stricmp(pe32.szExeFile, processName) == 0) {
                    pid = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        
        CloseHandle(hSnapshot);
        return pid;
    }
    
    ~MsIoInterface() {
        if (hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(hDriver);
        }
    }
};

int main() {
    MsIoInterface msio;
    
    if (!msio.Initialize()) {
        printf("\n[!] MsIo interface failed to initialize\n");
        printf("[*] To use MsIo64.sys (CVE-2019-18845):\n");
        printf("    1. Often found on ASRock motherboards\n");
        printf("    2. Check if MsIo64.sys exists in drivers folder\n");
        printf("    3. May need to load manually if not already loaded\n");
        printf("\n[*] This vulnerability allows direct physical memory access\n");
        printf("[*] Should bypass EAC protection completely!\n");
        system("pause");
        return 1;
    }
    
    // Test MsIo capabilities
    msio.TestMsIoCapabilities();
    
    printf("\n[!] If this works, MsIo is much better than RTCore64!\n");
    printf("[*] Physical memory access bypasses all userland protections\n");
    system("pause");
    return 0;
} 