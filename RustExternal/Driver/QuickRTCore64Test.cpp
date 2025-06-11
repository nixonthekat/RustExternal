#include <Windows.h>
#include <iostream>

// Most likely RTCore64 IOCTL codes based on research
#define IOCTL_TEST_1    0x80002048  // Most common
#define IOCTL_TEST_2    0x9C402084  // Alternative
#define IOCTL_TEST_3    0x222004    // Simplified
#define IOCTL_TEST_4    0x80002044  // Mapping related

#pragma pack(push, 1)
typedef struct _RTCORE_MEMORY_REQUEST {
    DWORD64 Address;        // Physical address
    DWORD64 Value;          // Return value  
    DWORD Size;             // Size to read
    DWORD Pad1;             // Padding
    DWORD Pad2;             // Padding
    DWORD Pad3;             // Padding
} RTCORE_MEMORY_REQUEST;
#pragma pack(pop)

int main() {
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                    QUICK RTCore64 TEST                        ║\n");
    printf("║              Testing the most likely IOCTL codes             ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    // Connect to RTCore64
    HANDLE hDriver = CreateFileW(L"\\\\.\\RTCore64",
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
                               
    if (hDriver == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        printf("❌ RTCore64 connection failed (Error: %d)\n", error);
        if (error == 5) {
            printf("   Need Administrator privileges!\n");
        }
        system("pause");
        return 1;
    }
    
    printf("✅ RTCore64 connected successfully!\n\n");
    
    // Test IOCTL codes
    DWORD ioctls[] = {IOCTL_TEST_1, IOCTL_TEST_2, IOCTL_TEST_3, IOCTL_TEST_4};
    const char* names[] = {"0x80002048", "0x9C402084", "0x222004", "0x80002044"};
    
    for (int i = 0; i < 4; i++) {
        printf("🔍 Testing IOCTL %s...\n", names[i]);
        
        RTCORE_MEMORY_REQUEST request = {0};
        request.Address = 0x1000;  // Safe physical address
        request.Size = 4;
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     ioctls[i],
                                     &request,
                                     sizeof(request),
                                     &request,
                                     sizeof(request),
                                     &bytesReturned,
                                     NULL);
        
        if (result) {
            printf("   ✅ IOCTL call successful!\n");
            printf("   📊 Bytes returned: %d\n", bytesReturned);
            printf("   📖 Value read: 0x%llX\n", request.Value);
            
            if (request.Value != 0) {
                printf("   🎯 NON-ZERO VALUE! This IOCTL works!\n");
                printf("\n🎉 WORKING CONFIGURATION FOUND:\n");
                printf("#define RTCORE64_MEMORY_READ %s\n", names[i]);
                printf("Address 0x1000 contains: 0x%llX\n", request.Value);
                
                // Test a few more addresses to confirm
                printf("\n🧪 Testing additional addresses:\n");
                for (DWORD64 testAddr = 0x2000; testAddr <= 0x5000; testAddr += 0x1000) {
                    request.Address = testAddr;
                    request.Value = 0;
                    
                    if (DeviceIoControl(hDriver, ioctls[i], &request, sizeof(request),
                                      &request, sizeof(request), &bytesReturned, NULL)) {
                        printf("   Address 0x%llX: 0x%llX\n", testAddr, request.Value);
                    }
                }
                
                CloseHandle(hDriver);
                printf("\n💡 USE THIS IOCTL CODE IN YOUR FINAL IMPLEMENTATION!\n");
                system("pause");
                return 0;
            }
        } else {
            DWORD error = GetLastError();
            printf("   ❌ IOCTL call failed (Error: %d)\n", error);
        }
        
        printf("\n");
    }
    
    printf("🚨 None of the tested IOCTL codes worked.\n");
    printf("💡 RTCore64 might use different codes on this system.\n");
    printf("🔧 Try running the full brute force tester as Administrator.\n");
    
    CloseHandle(hDriver);
    system("pause");
    return 1;
} 