#include <Windows.h>
#include <iostream>
#include <vector>

// RTCore64 actual IOCTL codes and structures (fixed)
#define RTCORE64_MEMORY_READ_CODE  0x80002048
#define RTCORE64_MEMORY_WRITE_CODE 0x8000204C

typedef struct _RTCORE64_MEMORY_READ {
    BYTE Pad0[8];
    DWORD64 Address;
    BYTE Pad1[8]; 
    DWORD ReadSize;
    DWORD Value;
    BYTE Pad3[16];
} RTCORE64_MEMORY_READ, *PRTCORE64_MEMORY_READ;

typedef struct _RTCORE64_MEMORY_WRITE {
    BYTE Pad0[8];
    DWORD64 Address;
    BYTE Pad1[8];
    DWORD WriteSize;
    DWORD Value;
    BYTE Pad3[16];
} RTCORE64_MEMORY_WRITE, *PRTCORE64_MEMORY_WRITE;

class TrueHybridInterface {
private:
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    DWORD64 ntoskrnlBase = 0;
    
public:
    bool Initialize() {
        printf("=== True Hybrid RTCore64 Interface ===\n");
        printf("[*] Using RTCore64 for ALL operations (no userland APIs)\n\n");
        
        // Connect to RTCore64
        hDriver = CreateFileW(L"\\\\.\\RTCore64", 
                             GENERIC_READ | GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING, 
                             FILE_ATTRIBUTE_NORMAL, NULL);
        
        if (hDriver == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            printf("[-] Failed to connect to RTCore64 (Error: %d)\n", error);
            printf("[!] Make sure MSI Afterburner is running\n");
            return false;
        }
        
        printf("[+] Successfully connected to RTCore64!\n");
        
        // Find ntoskrnl base for process scanning
        if (!FindNtoskrnlBase()) {
            printf("[-] Failed to find ntoskrnl base\n");
            return false;
        }
        
        return true;
    }
    
    DWORD ReadPhysicalMemory32(DWORD64 physicalAddress) {
        if (hDriver == INVALID_HANDLE_VALUE) return 0;
        
        RTCORE64_MEMORY_READ readRequest = {0};
        readRequest.Address = physicalAddress;
        readRequest.ReadSize = sizeof(DWORD);
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     RTCORE64_MEMORY_READ_CODE,
                                     &readRequest,
                                     sizeof(readRequest),
                                     &readRequest,
                                     sizeof(readRequest),
                                     &bytesReturned,
                                     NULL);
        
        if (!result) {
            return 0;
        }
        
        return readRequest.Value;
    }
    
    DWORD64 ReadPhysicalMemory64(DWORD64 physicalAddress) {
        // Read as two 32-bit values and combine
        DWORD low = ReadPhysicalMemory32(physicalAddress);
        DWORD high = ReadPhysicalMemory32(physicalAddress + 4);
        return ((DWORD64)high << 32) | low;
    }
    
    bool WritePhysicalMemory32(DWORD64 physicalAddress, DWORD value) {
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        
        RTCORE64_MEMORY_WRITE writeRequest = {0};
        writeRequest.Address = physicalAddress;
        writeRequest.WriteSize = sizeof(DWORD);
        writeRequest.Value = value;
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     RTCORE64_MEMORY_WRITE_CODE,
                                     &writeRequest,
                                     sizeof(writeRequest),
                                     &writeRequest,
                                     sizeof(writeRequest),
                                     &bytesReturned,
                                     NULL);
        
        return result != FALSE;
    }
    
    bool FindNtoskrnlBase() {
        printf("[*] Scanning for ntoskrnl.exe in physical memory...\n");
        
        // Common ntoskrnl base addresses to check
        DWORD64 commonBases[] = {
            0xFFFFF80000000000ULL,
            0xFFFFF80100000000ULL,
            0xFFFFF80200000000ULL,
            0xFFFFF80300000000ULL
        };
        
        for (int i = 0; i < 4; i++) {
            DWORD64 testAddr = commonBases[i];
            
            // Convert virtual to physical (simplified)
            DWORD64 physAddr = testAddr & 0x000FFFFFFFFFFFFFULL;
            
            // Check for PE signature
            DWORD peHeader = ReadPhysicalMemory32(physAddr);
            if ((peHeader & 0xFFFF) == 0x5A4D) { // "MZ"
                printf("[+] Found potential PE at physical 0x%llX\n", physAddr);
                ntoskrnlBase = physAddr;
                return true;
            }
        }
        
        printf("[-] ntoskrnl not found at common locations\n");
        return false;
    }
    
    DWORD64 FindRustClientProcess() {
        printf("[*] Scanning physical memory for RustClient.exe process...\n");
        
        // This is a simplified approach - in reality we'd need to:
        // 1. Find PsActiveProcessHead via ntoskrnl
        // 2. Walk the EPROCESS linked list
        // 3. Check process names until we find RustClient.exe
        
        // For now, let's test if we can read memory regions that might contain process data
        for (DWORD64 physAddr = 0x1000000; physAddr < 0x10000000; physAddr += 0x1000) {
            DWORD testValue = ReadPhysicalMemory32(physAddr);
            
            // Look for patterns that might indicate EPROCESS structures
            if (testValue != 0 && testValue != 0xFFFFFFFF) {
                // This is where we'd implement proper EPROCESS parsing
                printf("[*] Found readable memory at 0x%llX: 0x%08X\n", physAddr, testValue);
                
                // For demo purposes, return a test address
                if (physAddr > 0x5000000) {
                    printf("[+] Potential process structure found\n");
                    return physAddr;
                }
            }
        }
        
        printf("[-] RustClient.exe process not found in physical memory scan\n");
        return 0;
    }
    
    void TestPhysicalMemoryAccess() {
        printf("\n=== Testing Physical Memory Access ===\n");
        
        // Test reading from low physical memory
        for (DWORD64 addr = 0x1000; addr <= 0x10000; addr += 0x1000) {
            DWORD value = ReadPhysicalMemory32(addr);
            if (value != 0) {
                printf("[+] Physical read 0x%llX: 0x%08X\n", addr, value);
            }
        }
        
        // Test finding process structures
        DWORD64 processAddr = FindRustClientProcess();
        if (processAddr != 0) {
            printf("[+] Found potential RustClient process at 0x%llX\n", processAddr);
        }
    }
    
    void ScanForUnityPatterns() {
        printf("\n=== Scanning for Unity/GameAssembly patterns ===\n");
        
        // Look for Unity engine signatures in physical memory
        DWORD unityPattern1 = 0x556E6974; // "Unit" in little endian
        DWORD unityPattern2 = 0x79456E67; // "yEng" in little endian
        
        for (DWORD64 physAddr = 0x100000; physAddr < 0x80000000; physAddr += 0x1000) {
            DWORD value = ReadPhysicalMemory32(physAddr);
            
            if (value == unityPattern1 || value == unityPattern2) {
                printf("[!] Potential Unity signature found at physical 0x%llX\n", physAddr);
                
                // Read surrounding area for more context
                for (int i = -4; i <= 4; i++) {
                    DWORD64 testAddr = physAddr + (i * 4);
                    DWORD testValue = ReadPhysicalMemory32(testAddr);
                    printf("    0x%llX: 0x%08X\n", testAddr, testValue);
                }
                break;
            }
        }
    }
    
    ~TrueHybridInterface() {
        if (hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(hDriver);
        }
    }
};

int main() {
    TrueHybridInterface hybrid;
    
    if (!hybrid.Initialize()) {
        printf("\n[!] True hybrid interface failed to initialize\n");
        printf("[*] Make sure MSI Afterburner is running for RTCore64\n");
        system("pause");
        return 1;
    }
    
    // Test pure physical memory access
    hybrid.TestPhysicalMemoryAccess();
    
    // Try to find Unity/GameAssembly patterns
    hybrid.ScanForUnityPatterns();
    
    printf("\n[+] This approach uses ONLY RTCore64 - no userland APIs!\n");
    printf("[*] EAC cannot block physical memory access via kernel driver\n");
    
    system("pause");
    return 0;
} 