#include "includes.h"

void _HackThread( void )
{
	using framerate = std::chrono::duration<int, std::ratio<1, 30>>;
	auto tp = std::chrono::system_clock::now( ) + framerate { 1 };

	printf("🔍 EAC Memory Access Diagnostic\n");
	printf("📍 GameAssembly Base: 0x%llx\n", kinterface->ModuleBase);
	printf("🧪 Testing what memory regions EAC allows...\n\n");
	
	static bool diagnosticComplete = false;
	static DWORD lastTest = 0;
	
	while (true) {
		DWORD currentTime = GetTickCount();
		
		if (!diagnosticComplete && (currentTime - lastTest) > 3000) {
			printf("🧪 COMPREHENSIVE MEMORY ACCESS TEST:\n");
			
			// Test 1: Basic module memory (we know this works)
			printf("\n1️⃣ Testing basic module access...\n");
			uint64_t testAddr = kinterface->ModuleBase + 0x1000;
			uint32_t testValue = 0;
			bool basicRead = kinterface->ReadPhysMemory(kinterface->tPID, testAddr, &testValue, sizeof(testValue));
			printf("   Basic read (0x%llx): %s\n", testAddr, basicRead ? "✅ SUCCESS" : "❌ FAILED");
			
			// Test 2: Different memory regions
			printf("\n2️⃣ Testing different memory regions...\n");
			struct MemoryRegion {
				uint64_t offset;
				const char* name;
			};
			
			MemoryRegion regions[] = {
				{0x1000000, "Early region (16MB)"},
				{0x5000000, "Mid region (80MB)"},
				{0x10000000, "High region (256MB)"},
				{0x20000000, "Very high region (512MB)"},
				{0x30000000, "Ultra high region (768MB)"}
			};
			
			for (auto& region : regions) {
				uint64_t addr = kinterface->ModuleBase + region.offset;
				uint32_t value = 0;
				bool success = kinterface->ReadPhysMemory(kinterface->tPID, addr, &value, sizeof(value));
				printf("   %s (0x%llx): %s\n", region.name, addr, success ? "✅ READABLE" : "❌ BLOCKED");
			}
			
			// Test 3: Sequential access test
			printf("\n3️⃣ Testing sequential access patterns...\n");
			uint64_t startAddr = kinterface->ModuleBase + 0x5000000;
			int successCount = 0, failCount = 0;
			
			for (int i = 0; i < 100; i++) {
				uint64_t addr = startAddr + (i * 0x1000);
				uint32_t value = 0;
				if (kinterface->ReadPhysMemory(kinterface->tPID, addr, &value, sizeof(value))) {
					successCount++;
				} else {
					failCount++;
				}
			}
			printf("   Sequential reads: %d✅ success, %d❌ failed (%.1f%% blocked)\n", 
				   successCount, failCount, (failCount / 100.0f) * 100.0f);
			
			// Test 4: Data type access test
			printf("\n4️⃣ Testing different data types...\n");
			uint64_t dataTestAddr = kinterface->ModuleBase + 0x8000000;
			
			uint8_t byteVal = 0;
			uint16_t shortVal = 0;
			uint32_t intVal = 0;
			uint64_t longVal = 0;
			float floatVal = 0.0f;
			double doubleVal = 0.0;
			
			bool byteRead = kinterface->ReadPhysMemory(kinterface->tPID, dataTestAddr, &byteVal, sizeof(byteVal));
			bool shortRead = kinterface->ReadPhysMemory(kinterface->tPID, dataTestAddr, &shortVal, sizeof(shortVal));
			bool intRead = kinterface->ReadPhysMemory(kinterface->tPID, dataTestAddr, &intVal, sizeof(intVal));
			bool longRead = kinterface->ReadPhysMemory(kinterface->tPID, dataTestAddr, &longVal, sizeof(longVal));
			bool floatRead = kinterface->ReadPhysMemory(kinterface->tPID, dataTestAddr, &floatVal, sizeof(floatVal));
			bool doubleRead = kinterface->ReadPhysMemory(kinterface->tPID, dataTestAddr, &doubleVal, sizeof(doubleVal));
			
			printf("   uint8_t: %s, uint16_t: %s, uint32_t: %s\n", 
				   byteRead ? "✅" : "❌", shortRead ? "✅" : "❌", intRead ? "✅" : "❌");
			printf("   uint64_t: %s, float: %s, double: %s\n",
				   longRead ? "✅" : "❌", floatRead ? "✅" : "❌", doubleRead ? "✅" : "❌");
			
			// Test 5: Identify working regions
			printf("\n5️⃣ Finding working memory regions...\n");
			std::vector<uint64_t> workingRegions;
			
			for (uint64_t offset = 0x1000000; offset <= 0x50000000; offset += 0x2000000) {
				uint64_t addr = kinterface->ModuleBase + offset;
				uint32_t value = 0;
				if (kinterface->ReadPhysMemory(kinterface->tPID, addr, &value, sizeof(value))) {
					workingRegions.push_back(offset);
				}
			}
			
			printf("   Working regions found: %zu\n", workingRegions.size());
			for (uint64_t offset : workingRegions) {
				printf("   ✅ 0x%llx (+0x%llx)\n", kinterface->ModuleBase + offset, offset);
			}
			
			// Final verdict
			printf("\n🏁 DIAGNOSTIC COMPLETE!\n");
			if (workingRegions.size() > 10) {
				printf("✅ Memory access is GOOD - EAC allows most regions\n");
				printf("💡 Issue: Game data might be in unscanned regions or encrypted\n");
				printf("🎯 Recommendation: Manual Cheat Engine analysis needed\n");
			} else if (workingRegions.size() > 3) {
				printf("⚠️  Memory access is LIMITED - EAC blocks some regions\n");
				printf("💡 Issue: Selective memory protection active\n");
				printf("🎯 Recommendation: Focus on working regions only\n");
			} else {
				printf("❌ Memory access is HEAVILY BLOCKED by EAC\n");
				printf("💡 Issue: EAC prevents access to game object memory\n");
				printf("🎯 Recommendation: Different bypass method needed\n");
			}
			
			diagnosticComplete = true;
			lastTest = currentTime;
		}
		
		// Minimal placeholder
		g_pLocalPlayer = nullptr;
		g_pBaseNetworkable = nullptr;
		g_pEntity->Update();

		std::this_thread::sleep_until( tp );
		tp += framerate { 1 };
	}
}

int main( ) {
	Sleep( 1000 );

	if ( kinterface->Initialize( ) == FALSE ) {
		ExitProcess( 0 );
		__fastfail( 0 );
	}

	kinterface->PID( xorstr_( "RustClient.exe" ) );
	kinterface->ProcessBase = kinterface->GetProcessBase( kinterface->tPID );
	kinterface->ModuleBase = kinterface->GetModuleBase( kinterface->tPID, xorstr_( L"GameAssembly.dll" ) );

	printf( xorstr_( "kinterface->tPID: %d\n" ), kinterface->tPID );
	printf( xorstr_( "kinterface->ProcessBase: %llx\n" ), kinterface->ProcessBase );
	printf( xorstr_( "kinterface->ModuleBase: %llx\n" ), kinterface->ModuleBase );

	overlay->hTargetWindow = FindWindowA( xorstr_( "UnityWndClass" ), xorstr_( "Rust" ) );

	const auto overlay_thread { [ & ]( )->void {
		overlay->run( );
	} };

	create_thread( _HackThread );

	std::thread( overlay_thread ).join( );
}