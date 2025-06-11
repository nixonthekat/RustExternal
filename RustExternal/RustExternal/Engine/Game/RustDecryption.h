#pragma once
#ifndef __RUST_DECRYPTION_H__
#define __RUST_DECRYPTION_H__

#include <stdint.h>

// Rust decryption functions (from message.txt)
namespace RustDecrypt {

    // Il2Cpp handle function (needed for decryption)
    inline uint64_t Il2cppGetHandle(uint64_t address) {
        // Simple implementation - in real Rust this is more complex
        return address;
    }

    // Decrypt ClientEntities (BaseNetworkable entities)
    inline uint64_t ClientEntities(uint64_t address) {
        uint64_t rax = 0;
        if (!kinterface->ReadPhysMemory(kinterface->tPID, address + 0x18, &rax, sizeof(rax))) {
            return 0;
        }
        
        uint64_t originalRax = rax;
        uint32_t* rdx = (uint32_t*)&rax;
        uint32_t r8d = 0x2;
        uint32_t eax, ecx;

        do {
            eax = *rdx;
            rdx++;
            eax = eax + 0x84B02EEE;
            ecx = eax;
            eax = eax << 0xD;
            ecx = ecx >> 0x13;
            ecx = ecx | eax;
            ecx = ecx - 0x2F1224FF;
            *(rdx - 1) = ecx;
            --r8d;
        } while (r8d);
        
        return Il2cppGetHandle(rax);
    }

    // Decrypt EntityList 
    inline uint64_t EntityList(uint64_t address) {
        uint64_t rax = 0;
        if (!kinterface->ReadPhysMemory(kinterface->tPID, address + 0x18, &rax, sizeof(rax))) {
            return 0;
        }
        
        uint32_t* rdx = (uint32_t*)&rax;
        uint32_t r8d = 0x2;
        uint32_t eax, ecx;

        do {
            ecx = *rdx;
            eax = *rdx;
            rdx++;
            ecx = ecx >> 0xA;
            eax = eax << 0x16;
            ecx = ecx | eax;
            ecx = ecx - 0x49064304;
            ecx = ecx ^ 0xFA11D865;
            *(rdx - 1) = ecx;
            --r8d;
        } while (r8d);
        return Il2cppGetHandle(rax);
    }

    // Decrypt PlayerEyes
    inline uint64_t PlayerEyes(uint64_t address) {
        uint64_t rax = 0;
        if (!kinterface->ReadPhysMemory(kinterface->tPID, address + 0x18, &rax, sizeof(rax))) {
            return 0;
        }
        
        uint32_t* rdx = (uint32_t*)&rax;
        uint32_t r8d = 0x2;
        uint32_t eax, ecx;

        do {
            eax = *rdx;
            rdx++;
            eax = eax ^ 0x5C4C7BF3;
            ecx = eax;
            eax = eax << 0x10;
            ecx = ecx >> 0x10;
            ecx = ecx | eax;
            ecx = ecx - 0x3E3A4010;
            *(rdx - 1) = ecx;
            --r8d;
        } while (r8d);
        return Il2cppGetHandle(rax);
    }

    // Decrypt ClActiveItem
    inline uint64_t ClActiveItem(uint64_t address) {
        uint64_t rsp = address;
        uint64_t* rdx = &rsp;
        uint32_t r8d = 0x2;
        uint32_t eax, ecx, edx;

        do {
            ecx = *(uint32_t*)(rdx);
            eax = *(uint32_t*)(rdx);
            rdx = (uint64_t*)((uint8_t*)rdx + 0x4);
            eax = eax << 0x18;
            ecx = ecx >> 0x8;
            ecx = ecx | eax;
            ecx = ecx + 0xD72E3265;
            eax = ecx;
            ecx = ecx << 0x1F;
            eax = eax >> 0x1;
            eax = eax | ecx;
            eax = eax - 0x9830B30;
            *((uint32_t*)rdx - 1) = eax;
            --r8d;
        } while (r8d);
        return rsp;
    }

    // Updated PlayerInventory decryption (current algorithm)
    inline uint64_t PlayerInventory(uint64_t address) {
        uint64_t rax = 0;
        if (!kinterface->ReadPhysMemory(kinterface->tPID, address + 0x18, &rax, sizeof(rax))) {
            return 0;
        }
        
        uint32_t* rdx = (uint32_t*)&rax;
        uint32_t r8d = 0x2;
        uint32_t eax, ecx;

        do {
            eax = *rdx;
            ecx = *rdx;
            rdx++;
            eax = eax << 0x10;
            ecx = ecx | eax;
            ecx = ecx + 0xD406ABD4;
            eax = ecx;
            ecx = ecx << 0x14;
            eax = eax >> 0xC;
            eax = eax | ecx;
            *(rdx - 1) = eax;
            --r8d;
        } while (r8d);
        return Il2cppGetHandle(rax);
    }

    // Get BaseNetworkable entities using current method
    inline uint64_t GetBaseNetworkableEntities() {
        // Read BaseNetworkable TypeInfo -> ClientEntities
        uint64_t baseNetworkableTypeInfo = kinterface->ModuleBase + IOffset::BaseNetworkable_TypeInfo;
        uint64_t staticInstance = 0;
        
        // First read the static instance pointer
        if (!kinterface->ReadPhysMemory(kinterface->tPID, baseNetworkableTypeInfo, &staticInstance, sizeof(staticInstance))) {
            return 0;
        }
        
        if (staticInstance != 0) {
            // Then read ClientEntities from the instance
            uint64_t clientEntitiesPtr = 0;
            if (kinterface->ReadPhysMemory(kinterface->tPID, staticInstance + IOffset::BaseNetworkable::ClientEntities, &clientEntitiesPtr, sizeof(clientEntitiesPtr))) {
                if (clientEntitiesPtr != 0) {
                    return ClientEntities(clientEntitiesPtr);
                }
            }
        }
        return 0;
    }

    // Get LocalPlayer using current method
    inline uint64_t GetLocalPlayer() {
        // Read BasePlayer TypeInfo -> static instance
        uint64_t basePlayerTypeInfo = kinterface->ModuleBase + IOffset::BasePlayer_TypeInfo;
        uint64_t staticInstance = 0;
        
        // Read the static instance pointer
        if (!kinterface->ReadPhysMemory(kinterface->tPID, basePlayerTypeInfo, &staticInstance, sizeof(staticInstance))) {
            return 0;
        }
        
        // The static instance IS the LocalPlayer
        if (staticInstance != 0 && staticInstance > 0x10000 && staticInstance < 0x7FFFFFFFFFFF) {
            return staticInstance;
        }
        return 0;
    }
}

#endif 