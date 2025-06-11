#include <Windows.h>
#include <iostream>

class UltimateRTCore64Brute {
private:
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    
public:
    bool Initialize() {
        printf("╔════════════════════════════════════════════════════════════════╗\n");
        printf("║          ULTIMATE RTCore64 IOCTL BRUTE FORCER                 ║\n");
        printf("║     Since connection works, let's find the RIGHT codes!        ║\n");
        printf("╚════════════════════════════════════════════════════════════════╝\n\n");
        
        hDriver = CreateFileW(L"\\\\.\\RTCore64",
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                             
        if (hDriver == INVALID_HANDLE_VALUE) {
            printf("❌ RTCore64 connection failed (Error: %d)\n", GetLastError());
            return false;
        }
        
        printf("✅ RTCore64 connected successfully!\n");
        return true;
    }
    
    void BruteForceSystematic() {
        printf("\n🔍 SYSTEMATIC IOCTL BRUTE FORCE\n");
        printf("Testing comprehensive IOCTL code ranges...\n\n");
        
        // Test common IOCTL patterns for device drivers
        TestIOCTLRange("Standard Range 1", 0x80000000, 0x80010000, 0x4);
        TestIOCTLRange("Standard Range 2", 0x90000000, 0x90010000, 0x4);  
        TestIOCTLRange("RTCore Specific", 0x80002000, 0x80003000, 0x4);
        TestIOCTLRange("Method_Buffered", 0x20000000, 0x20010000, 0x4);
        TestIOCTLRange("Alternative", 0x9C400000, 0x9C410000, 0x4);
        
        // Test very specific known working codes from other RTCore research
        TestSpecificIOCTLs();
    }
    
    void TestIOCTLRange(const char* rangeName, DWORD start, DWORD end, DWORD step) {
        printf("🔍 Testing %s (0x%08X - 0x%08X)...\n", rangeName, start, end);
        
        for (DWORD ioctl = start; ioctl < end; ioctl += step) {
            if (TestSingleIOCTL(ioctl)) {
                printf("🎯 JACKPOT! Working IOCTL found: 0x%08X\n", ioctl);
                ExtensiveTest(ioctl);
                return; // Found working IOCTL, stop here
            }
            
            // Progress indicator for long ranges
            if ((ioctl - start) % 0x1000 == 0) {
                printf("   Progress: 0x%08X...\n", ioctl);
            }
        }
        printf("   ❌ No working IOCTLs in %s range\n\n", rangeName);
    }
    
    void TestSpecificIOCTLs() {
        printf("🎯 Testing SPECIFIC known RTCore64 IOCTLs...\n");
        
        // These are IOCTLs found in actual RTCore64.sys analysis
        DWORD specificIOCTLs[] = {
            // CTL_CODE patterns for RTCore64
            0x80002048, 0x8000204C, 0x80002050, 0x80002054,
            0x80002044, 0x80002040, 0x80002058, 0x8000205C,
            
            // Alternative method codes
            0x80102048, 0x8010204C, 0x80202048, 0x8020204C,
            
            // Different access patterns  
            0x9C402084, 0x9C402088, 0x9C40208C, 0x9C402090,
            0x9C406084, 0x9C406088, 0x9C40A084, 0x9C40A088,
            
            // Simple sequential codes
            0x222004, 0x222008, 0x22200C, 0x222010, 
            0x222014, 0x222018, 0x22201C, 0x222020,
            
            // FILE_DEVICE_UNKNOWN variations
            0x22E004, 0x22E008, 0x22E00C, 0x22E010,
            
            // More CTL_CODE possibilities
            0x2222004, 0x2222008, 0x222200C, 0x2222010
        };
        
        for (int i = 0; i < sizeof(specificIOCTLs)/sizeof(DWORD); i++) {
            DWORD ioctl = specificIOCTLs[i];
            printf("[%d/%d] Testing 0x%08X... ", i+1, sizeof(specificIOCTLs)/sizeof(DWORD), ioctl);
            
            if (TestSingleIOCTL(ioctl)) {
                printf("SUCCESS! 🎉\n");
                ExtensiveTest(ioctl);
                return;
            } else {
                printf("Failed\n");
            }
        }
    }
    
    bool TestSingleIOCTL(DWORD ioctl) {
        // Try multiple structure formats for each IOCTL
        
        // Format 1: Standard
        if (TestFormat1(ioctl)) return true;
        
        // Format 2: With padding
        if (TestFormat2(ioctl)) return true;
        
        // Format 3: Different order
        if (TestFormat3(ioctl)) return true;
        
        // Format 4: Minimal
        if (TestFormat4(ioctl)) return true;
        
        return false;
    }
    
    bool TestFormat1(DWORD ioctl) {
        struct {
            DWORD64 Address;
            DWORD64 Value;
            DWORD Size;
        } request = {0x1000, 0, 4};
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request),
                                     &request, sizeof(request), &bytesReturned, NULL);
        
        return result && bytesReturned > 0 && request.Value != 0;
    }
    
    bool TestFormat2(DWORD ioctl) {
        struct {
            DWORD64 PhysicalAddress;
            DWORD64 Value;
            DWORD Size;
            DWORD Pad1, Pad2, Pad3;
        } request = {0x1000, 0, 4, 0, 0, 0};
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request),
                                     &request, sizeof(request), &bytesReturned, NULL);
        
        return result && bytesReturned > 0 && request.Value != 0;
    }
    
    bool TestFormat3(DWORD ioctl) {
        struct {
            DWORD Size;
            DWORD64 Address;  
            DWORD64 Value;
        } request = {4, 0x1000, 0};
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request),
                                     &request, sizeof(request), &bytesReturned, NULL);
        
        return result && bytesReturned > 0 && request.Value != 0;
    }
    
    bool TestFormat4(DWORD ioctl) {
        struct {
            DWORD64 Address;
            DWORD Size;
            DWORD64 Buffer;
        } request = {0x1000, 4, 0};
        
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request),
                                     &request, sizeof(request), &bytesReturned, NULL);
        
        return result && bytesReturned > 0 && request.Buffer != 0;
    }
    
    void ExtensiveTest(DWORD workingIOCTL) {
        printf("\n╔════════════════════════════════════════════════════════════════╗\n");
        printf("║                    🎉 WORKING IOCTL FOUND! 🎉                  ║\n");
        printf("║                        0x%08X                           ║\n", workingIOCTL);
        printf("╚════════════════════════════════════════════════════════════════╝\n\n");
        
        printf("🧪 EXTENSIVE TESTING of IOCTL 0x%08X:\n", workingIOCTL);
        
        // Test which format works
        printf("\n📋 Testing structure formats:\n");
        if (TestFormat1(workingIOCTL)) {
            printf("✅ Format 1 (Standard) works!\n");
            GenerateCode(workingIOCTL, 1);
        }
        if (TestFormat2(workingIOCTL)) {
            printf("✅ Format 2 (With padding) works!\n");
            GenerateCode(workingIOCTL, 2);
        }
        if (TestFormat3(workingIOCTL)) {
            printf("✅ Format 3 (Different order) works!\n");
            GenerateCode(workingIOCTL, 3);
        }
        if (TestFormat4(workingIOCTL)) {
            printf("✅ Format 4 (Minimal) works!\n");
            GenerateCode(workingIOCTL, 4);
        }
        
        // Test reading from multiple addresses
        printf("\n🔍 Testing memory reads:\n");
        TestMemoryReads(workingIOCTL);
    }
    
    void TestMemoryReads(DWORD ioctl) {
        DWORD64 testAddresses[] = {
            0x1000, 0x2000, 0x3000, 0x4000, 0x5000,
            0x10000, 0x20000, 0x100000, 0x500000
        };
        
        for (int i = 0; i < sizeof(testAddresses)/sizeof(DWORD64); i++) {
            struct {
                DWORD64 Address;
                DWORD64 Value;
                DWORD Size;
            } request = {testAddresses[i], 0, 4};
            
            DWORD bytesReturned = 0;
            BOOL result = DeviceIoControl(hDriver, ioctl, &request, sizeof(request),
                                         &request, sizeof(request), &bytesReturned, NULL);
            
            if (result && request.Value != 0) {
                printf("  📖 0x%08llX: 0x%08llX ✅\n", testAddresses[i], request.Value);
            }
        }
    }
    
    void GenerateCode(DWORD ioctl, int format) {
        printf("\n╔════════════════════════════════════════════════════════════════╗\n");
        printf("║                    FINAL WORKING CODE                          ║\n");
        printf("╚════════════════════════════════════════════════════════════════╝\n");
        
        printf("\n// 🎯 WORKING RTCore64 CONFIGURATION:\n");
        printf("#define RTCORE64_MEMORY_READ    0x%08X\n\n", ioctl);
        
        printf("// Working structure (Format %d):\n", format);
        switch (format) {
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
                printf("    DWORD Pad1, Pad2, Pad3;\n");
                printf("} RTCORE_REQUEST;\n");
                break;
            case 3:
                printf("typedef struct _RTCORE_REQUEST {\n");
                printf("    DWORD Size;\n");
                printf("    DWORD64 Address;\n");
                printf("    DWORD64 Value;\n");
                printf("} RTCORE_REQUEST;\n");
                break;
            case 4:
                printf("typedef struct _RTCORE_REQUEST {\n");
                printf("    DWORD64 Address;\n");
                printf("    DWORD Size;\n");
                printf("    DWORD64 Buffer;\n");
                printf("} RTCORE_REQUEST;\n");
                break;
        }
        
        printf("\n🚀 RTCore64 is now ready for EAC bypass!\n");
        printf("🎯 Use these values in your Rust game hacking implementation!\n");
    }
    
    ~UltimateRTCore64Brute() {
        if (hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(hDriver);
        }
    }
};

int main() {
    printf("Since RTCore64 connection works and we get Error 87,\n");
    printf("we just need to find the RIGHT IOCTL codes!\n\n");
    
    UltimateRTCore64Brute bruter;
    
    if (!bruter.Initialize()) {
        system("pause");
        return 1;
    }
    
    bruter.BruteForceSystematic();
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║   If no working IOCTL found, RTCore64 may use custom codes     ║\n");
    printf("║   Consider reverse engineering RTCore64.sys for exact codes    ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    system("pause");
    return 0;
} 