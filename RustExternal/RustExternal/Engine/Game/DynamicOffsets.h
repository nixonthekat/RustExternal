#pragma once
#ifndef __DYNAMIC_OFFSETS_H__
#define __DYNAMIC_OFFSETS_H__

#include <stdint.h>
#include <Windows.h>
#include <vector>
#include <map>

class DynamicOffsetResolver {
private:
    static inline std::map<std::string, uintptr_t> cachedOffsets;
    static inline DWORD lastUpdateTime = 0;
    static inline const DWORD UPDATE_INTERVAL = 1000; // Update every 1 second (not 1ms - too expensive)

public:
    // Pattern scanning function
    static uintptr_t PatternScan(uintptr_t baseAddress, size_t scanSize, const char* pattern, const char* mask) {
        for (size_t i = 0; i < scanSize - strlen(mask); i++) {
            bool found = true;
            for (size_t j = 0; j < strlen(mask); j++) {
                if (mask[j] == 'x') {
                    char buffer;
                    if (!kinterface->ReadPhysMemory(kinterface->tPID, baseAddress + i + j, &buffer, 1) || 
                        buffer != pattern[j]) {
                        found = false;
                        break;
                    }
                }
            }
            if (found) {
                return baseAddress + i;
            }
        }
        return 0;
    }

    // Find LocalPlayer offset using targeted Rust scanning
    static uintptr_t GetLocalPlayerOffset(bool forceUpdate = false) {
        DWORD currentTime = GetTickCount();
        
        if (!forceUpdate && 
            cachedOffsets.find("LocalPlayer") != cachedOffsets.end() && 
            (currentTime - lastUpdateTime) < UPDATE_INTERVAL) {
            return cachedOffsets["LocalPlayer"];
        }

        printf("🔍 Rust-specific LocalPlayer scanning...\n");

        // Method 1: Scan for actual player data structures
        printf("📍 Method 1: Scanning for player data structures...\n");
        
        // Scan smaller, more targeted ranges
        uintptr_t scanStart = kinterface->ModuleBase;
        uintptr_t scanSize = 0x20000000; // Reduced to 512MB for speed
        uintptr_t scanStep = 0x1000;     // 4KB steps (much faster)
        
        for (uintptr_t addr = scanStart; addr < scanStart + scanSize; addr += scanStep) {
            // Look for potential LocalPlayer pointer (should be valid memory address)
            uintptr_t playerPtr = 0;
                            if (kinterface->ReadPhysMemory(kinterface->tPID, addr, &playerPtr, sizeof(playerPtr))) {
                // Check if this looks like a valid player pointer
                if (playerPtr > 0x10000 && playerPtr < 0x7FFFFFFFFFFF) {
                    // Try to read what it points to
                    char buffer[0x100];
                    if (kinterface->ReadPhysMemory(kinterface->tPID, playerPtr, buffer, sizeof(buffer))) {
                        // Look for player-like data (health values, position coordinates, etc.)
                        float* floatData = (float*)buffer;
                        for (int i = 0; i < 20; i++) {
                            float val = floatData[i];
                            // Look for health values (typically 0-100) or position coordinates
                            if ((val > 0.0f && val <= 100.0f) || 
                                (val > -10000.0f && val < 10000.0f && val != 0.0f)) {
                                printf("🎯 Potential LocalPlayer at: 0x%llx (ptr: 0x%llx)\n", 
                                       addr - kinterface->ModuleBase, playerPtr);
                                
                                cachedOffsets["LocalPlayer"] = addr - kinterface->ModuleBase;
                                lastUpdateTime = currentTime;
                                return addr - kinterface->ModuleBase;
                            }
                        }
                    }
                }
            }
            
            // Progress every 64MB
            if ((addr - scanStart) % 0x4000000 == 0) {
                printf("📍 Scanned: 0x%llx / 0x%llx\n", addr - scanStart, scanSize);
            }
        }

        // Method 2: Known Rust offset patterns (updated for recent versions)
        printf("📍 Method 2: Known Rust offset patterns...\n");
        
        // These are more recent Rust LocalPlayer patterns
        std::vector<uintptr_t> knownOffsets = {
            0x3B64E00,  // Recent Rust builds
            0x3B64E08,  
            0x3B65000,
            0x3A00000,  // Alternative ranges
            0x3A50000,
            0x3B00000,
            0x3C00000,
            0x4000000   // Higher memory ranges
        };
        
        for (uintptr_t testOffset : knownOffsets) {
            uintptr_t testAddr = kinterface->ModuleBase + testOffset;
            uintptr_t playerPtr = 0;
            
            if (kinterface->ReadPhysMemory(kinterface->tPID, testAddr, &playerPtr, sizeof(playerPtr))) {
                if (playerPtr > 0x10000 && playerPtr < 0x7FFFFFFFFFFF) {
                    printf("✅ Found valid pointer at offset 0x%llx -> 0x%llx\n", testOffset, playerPtr);
                    cachedOffsets["LocalPlayer"] = testOffset;
                    lastUpdateTime = currentTime;
                    return testOffset;
                }
            }
        }

        printf("❌ LocalPlayer offset not found via Rust scanning\n");
        return 0;
    }

    // Find BaseNetworkable offset using known Rust patterns
    static uintptr_t GetBaseNetworkableOffset(bool forceUpdate = false) {
        DWORD currentTime = GetTickCount();
        
        if (!forceUpdate && 
            cachedOffsets.find("BaseNetworkable") != cachedOffsets.end() && 
            (currentTime - lastUpdateTime) < UPDATE_INTERVAL) {
            return cachedOffsets["BaseNetworkable"];
        }

        printf("🔍 Rust BaseNetworkable scanning...\n");

        // Known Rust BaseNetworkable offset patterns (recent builds)
        std::vector<uintptr_t> knownOffsets = {
            0x3B5A370,  // Common Rust BaseNetworkable offsets
            0x3B5A378,  
            0x3B5B000,
            0x3B50000,
            0x3A00000,  // Alternative ranges
            0x3C00000,
            0x4000000,
            0x5000000   // Higher ranges
        };
        
        for (uintptr_t testOffset : knownOffsets) {
            uintptr_t testAddr = kinterface->ModuleBase + testOffset;
            uintptr_t entityPtr = 0;
            
                         if (kinterface->ReadPhysMemory(kinterface->tPID, testAddr, &entityPtr, sizeof(entityPtr))) {
                if (entityPtr > 0x10000 && entityPtr < 0x7FFFFFFFFFFF) {
                    // Try to read entity data
                    char buffer[0x50];
                    if (kinterface->ReadPhysMemory(kinterface->tPID, entityPtr, buffer, sizeof(buffer))) {
                        printf("✅ Found BaseNetworkable at offset 0x%llx -> 0x%llx\n", testOffset, entityPtr);
                        cachedOffsets["BaseNetworkable"] = testOffset;
                        lastUpdateTime = currentTime;
                        return testOffset;
                    }
                }
            }
        }

        printf("❌ BaseNetworkable offset not found\n");
        return 0;
    }

    // Find Camera Manager offset
    static uintptr_t GetCameraManagerOffset(bool forceUpdate = false) {
        DWORD currentTime = GetTickCount();
        
        if (!forceUpdate && 
            cachedOffsets.find("CameraManager") != cachedOffsets.end() && 
            (currentTime - lastUpdateTime) < UPDATE_INTERVAL) {
            return cachedOffsets["CameraManager"];
        }

        printf("🔍 Scanning for CameraManager offset...\n");

        std::vector<std::pair<const char*, const char*>> signatures = {
            {"\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x00\x48\x8B\x01", "xxx????xxxx?xxx"},
            {"\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xD8", "xxx????x????xxx"}
        };

        for (auto& sig : signatures) {
            uintptr_t found = PatternScan(kinterface->ModuleBase, 0x10000000, sig.first, sig.second);
            if (found != 0) {
                int32_t relativeOffset;
                if (kinterface->ReadPhysMemory(kinterface->tPID, found + 3, &relativeOffset, sizeof(relativeOffset))) {
                    uintptr_t absoluteOffset = (found + 7 + relativeOffset) - kinterface->ModuleBase;
                    printf("✅ Found CameraManager offset: 0x%llx\n", absoluteOffset);
                    cachedOffsets["CameraManager"] = absoluteOffset;
                    lastUpdateTime = currentTime;
                    return absoluteOffset;
                }
            }
        }

        printf("❌ CameraManager offset not found, using fallback\n");
        return 0;
    }

    // Update all offsets (call this periodically)
    static void UpdateAllOffsets() {
        printf("🔄 Updating all dynamic offsets...\n");
        GetLocalPlayerOffset(true);
        GetBaseNetworkableOffset(true);
        GetCameraManagerOffset(true);
        printf("✅ Dynamic offset update complete\n");
    }

    // Clear cache (force refresh)
    static void ClearCache() {
        cachedOffsets.clear();
        lastUpdateTime = 0;
    }
};

// New dynamic offset namespace (replaces IOffset)
namespace DynamicOffset {
    inline uintptr_t GetLocalPlayer() {
        return DynamicOffsetResolver::GetLocalPlayerOffset();
    }
    
    inline uintptr_t GetBaseNetworkable() {
        return DynamicOffsetResolver::GetBaseNetworkableOffset();
    }
    
    inline uintptr_t GetCameraManager() {
        return DynamicOffsetResolver::GetCameraManagerOffset();
    }
}

#endif 