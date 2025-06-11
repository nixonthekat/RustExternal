#pragma once
#ifndef __OFFSETS_H__
#define __OFFSETS_H__

#include <stdint.h>

namespace IOffset
{
	// CURRENT RUST OFFSETS - January 2025 (CONFIRMED WORKING)
	inline uintptr_t Il2CppGetHandle = 0xC191CA0;
	inline uintptr_t BaseNetworkable_TypeInfo = 0xBE7FB88;
	inline uintptr_t MainCamera_TypeInfo = 0xBEDD6A8;
	inline uintptr_t BasePlayer_TypeInfo = 0xBF26B08;
	inline uintptr_t TOD_Sky_TypeInfo = 0xBEF9CB0;
	inline uintptr_t ConvarAdmin_TypeInfo = 0xBEFE9D8;
	inline uintptr_t Input_TypeInfo = 0xBEA5C88;
	inline uintptr_t ConvarGraphics_TypeInfo = 0xBE933A8;

	// Legacy names for compatibility
	inline uintptr_t dwLocalPlayer = BasePlayer_TypeInfo;
	inline uintptr_t dwBaseNetworkable = BaseNetworkable_TypeInfo;
	inline uintptr_t dwCameraManager = MainCamera_TypeInfo;
	inline uintptr_t dwTOD_Sky = TOD_Sky_TypeInfo;
	inline uintptr_t dwConvarAdmin = ConvarAdmin_TypeInfo;
	inline uintptr_t dwInput = Input_TypeInfo;
	inline uintptr_t dwConvarGraphics = ConvarGraphics_TypeInfo;
	inline uintptr_t dwIl2CppGetHandle = Il2CppGetHandle;

	// Camera chain offsets
	namespace Camera {
		inline uintptr_t Camera = 0xBEDD6A8;  // MainCamera_TypeInfo
		inline uintptr_t Camera1 = 0xB8;
		inline uintptr_t Camera2 = 0x8;
		inline uintptr_t Camera3 = 0x10;
	}

	// TOD Sky chain
	namespace TODSky {
		inline uintptr_t Base = 0xBEF9CB0;  // TOD_Sky_TypeInfo
		inline uintptr_t Offset1 = 0xB8;
		inline uintptr_t Offset2 = 0x28;
		inline uintptr_t Offset3 = 0x10;
	}

	// BaseNetworkable structure
	namespace BaseNetworkable {
		inline uintptr_t ClientEntities = 0x30;
		namespace EntityRealm {
			inline uintptr_t EntityList = 0x10;
		}
	}

	// BasePlayer structure  
	namespace BasePlayer {
		inline uintptr_t ClActiveItem = 0x460;
		inline uintptr_t PlayerEyes = 0x2A8;
		inline uintptr_t PlayerInventory = 0x478;
		inline uintptr_t CurrentTeam = 0x430;
		inline uintptr_t BaseMovement = 0x598;
		inline uintptr_t PlayerModel = 0x288;
		inline uintptr_t PlayerFlags = 0x558;
		inline uintptr_t DisplayName = 0x3F8;
		inline uintptr_t PlayerInput = 0x5C0;
	}
}

#endif