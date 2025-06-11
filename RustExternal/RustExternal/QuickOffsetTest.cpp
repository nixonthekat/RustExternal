#include "includes.h"

int main() {
    printf("🚀 Quick Rust Offset Tester\n");
    printf("============================\n");

    Sleep(1000);

    if (kinterface->Initialize() == FALSE) {
        printf("❌ Driver initialization failed!\n");
        system("pause");
        return 1;
    }

    kinterface->PID("RustClient.exe");
    kinterface->ProcessBase = kinterface->GetProcessBase(kinterface->tPID);
    kinterface->ModuleBase = kinterface->GetModuleBase(kinterface->tPID, L"GameAssembly.dll");

    printf("✅ PID: %d\n", kinterface->tPID);
    printf("✅ ProcessBase: 0x%llx\n", kinterface->ProcessBase);
    printf("✅ ModuleBase: 0x%llx\n", kinterface->ModuleBase);

    if (kinterface->tPID == 0 || kinterface->ModuleBase == 0) {
        printf("❌ Failed to get basic info\n");
        system("pause");
        return 1;
    }

    printf("\n🔍 Testing known Rust offset ranges...\n");

    // Test ranges of offsets commonly used in recent Rust builds
    std::vector<uintptr_t> testOffsets = {
        // Recent Rust builds (2024)
        0x3B64E00, 0x3B64E08, 0x3B65000, 0x3B65008,
        0x3B50000, 0x3B51000, 0x3B52000, 0x3B53000,
        0x3A00000, 0x3A10000, 0x3A20000, 0x3A30000,
        
        // Alternative ranges
        0x4000000, 0x4100000, 0x4200000,
        0x5000000, 0x5100000, 0x5200000,
        
        // Lower ranges
        0x2000000, 0x2100000, 0x2200000,
        0x1000000, 0x1100000, 0x1200000
    };

    printf("📍 Testing %d potential offsets...\n", (int)testOffsets.size());

    int validOffsets = 0;
    for (uintptr_t offset : testOffsets) {
        uintptr_t testAddr = kinterface->ModuleBase + offset;
        uintptr_t value = 0;
        
        if (kinterface->ReadPhysMemory(kinterface->tPID, testAddr, &value, sizeof(value))) {
            if (value > 0x10000 && value < 0x7FFFFFFFFFFF) {
                printf("✅ Valid pointer at offset 0x%08llx -> 0x%llx\n", offset, value);
                validOffsets++;
                
                // Test if this points to valid memory
                char buffer[0x100];
                if (kinterface->ReadPhysMemory(kinterface->tPID, value, buffer, sizeof(buffer))) {
                    float* floats = (float*)buffer;
                    bool hasReasonableFloats = false;
                    
                    for (int i = 0; i < 20; i++) {
                        float f = floats[i];
                        if (f > -10000.0f && f < 10000.0f && f != 0.0f && 
                            !isnan(f) && !isinf(f)) {
                            hasReasonableFloats = true;
                            break;
                        }
                    }
                    
                    if (hasReasonableFloats) {
                        printf("🎯 *** POTENTIAL PLAYER/ENTITY DATA *** at 0x%08llx\n", offset);
                        
                        // Show some sample data
                        printf("    Sample floats: ");
                        for (int i = 0; i < 8; i++) {
                            printf("%.2f ", floats[i]);
                        }
                        printf("\n");
                    }
                }
            }
        }
        
        // Progress indicator
        if ((offset == testOffsets[testOffsets.size()/4]) || 
            (offset == testOffsets[testOffsets.size()/2]) ||
            (offset == testOffsets[3*testOffsets.size()/4])) {
            printf("📍 Progress: %d%% complete...\n", 
                   (int)((std::find(testOffsets.begin(), testOffsets.end(), offset) - testOffsets.begin()) * 100 / testOffsets.size()));
        }
    }

    printf("\n📊 SUMMARY:\n");
    printf("✅ Found %d valid pointers out of %d tested\n", validOffsets, (int)testOffsets.size());
    
    if (validOffsets > 0) {
        printf("\n🎯 RECOMMENDATION:\n");
        printf("Try using one of the '*** POTENTIAL PLAYER/ENTITY DATA ***' offsets\n");
        printf("Replace the static offsets in Offsets.h with these values\n");
    } else {
        printf("\n⚠️  No valid offsets found in common ranges\n");
        printf("The game might be using a different memory layout\n");
    }

    printf("\nPress any key to exit...\n");
    system("pause");
    return 0;
} 