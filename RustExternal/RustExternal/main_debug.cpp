#include "includes.h"

void _HackThread( void )
{
	using framerate = std::chrono::duration<int, std::ratio<1, 30>>;
	auto tp = std::chrono::system_clock::now( ) + framerate { 1 };

	while ( true )
	{
		g_pLocalPlayer = ReadPhysMemory<CLocalPlayer*>( kinterface->ModuleBase + IOffset::dwLocalPlayer );
		g_pBaseNetworkable = ReadPhysMemory<CBaseNetworkable*>( kinterface->ModuleBase + IOffset::dwBaseNetworkable );

		g_pEntity->Update( );

		std::this_thread::sleep_until( tp );
		tp += framerate { 1 };
	}
}

int main( ) {
	printf("=== RustExternal Debug Version ===\n");
	printf("Starting initialization...\n");
	
	Sleep( 1000 );

	printf("Attempting kernel interface initialization...\n");
	if ( kinterface->Initialize( ) == FALSE ) {
		printf("❌ KERNEL INTERFACE INITIALIZATION FAILED!\n");
		printf("Error: Could not connect to \\\\\.\\KernelDriver\n");
		printf("Error Code: %d\n", GetLastError());
		
		printf("\n💡 TROUBLESHOOTING:\n");
		printf("1. Custom kernel driver is not loaded\n");
		printf("2. Wrong device name (expecting KernelDriver, got RTCore64)\n");
		printf("3. Insufficient privileges\n");
		printf("4. Driver file missing\n");
		
		printf("\n🔧 SOLUTIONS:\n");
		printf("A. Load your custom KernelDriver\n");
		printf("B. Modify code to use RTCore64 instead\n");
		printf("C. Run as Administrator\n");
		
		printf("\nPress any key to exit...\n");
		system("pause");
		return 1;
	}

	printf("✅ Kernel interface initialized successfully!\n");

	printf("Finding RustClient.exe process...\n");
	int pid = kinterface->PID( xorstr_( "RustClient.exe" ) );
	if (pid == 0) {
		printf("❌ RustClient.exe not found!\n");
		printf("Make sure Rust game is running.\n");
		system("pause");
		return 1;
	}
	
	printf("✅ Found RustClient.exe PID: %d\n", pid);

	printf("Getting process base address...\n");
	kinterface->ProcessBase = kinterface->GetProcessBase( kinterface->tPID );
	printf("Process Base: 0x%llx\n", kinterface->ProcessBase);

	printf("Getting GameAssembly.dll base address...\n");
	kinterface->ModuleBase = kinterface->GetModuleBase( kinterface->tPID, xorstr_( L"GameAssembly.dll" ) );
	printf("GameAssembly Base: 0x%llx\n", kinterface->ModuleBase);

	if (kinterface->ModuleBase == 0) {
		printf("❌ GameAssembly.dll not found!\n");
		printf("This indicates:\n");
		printf("1. EAC is blocking access\n");
		printf("2. Module enumeration failed\n");
		printf("3. Wrong process targeted\n");
	}

	printf( "\n=== FINAL STATUS ===\n" );
	printf( "PID: %d\n", kinterface->tPID );
	printf( "ProcessBase: 0x%llx\n", kinterface->ProcessBase );
	printf( "ModuleBase: 0x%llx\n", kinterface->ModuleBase );

	printf("Finding Rust game window...\n");
	overlay->hTargetWindow = FindWindowA( xorstr_( "UnityWndClass" ), xorstr_( "Rust" ) );
	if (overlay->hTargetWindow) {
		printf("✅ Found Rust window\n");
	} else {
		printf("❌ Rust window not found\n");
	}

	printf("\n🎯 Starting overlay and hack threads...\n");

	const auto overlay_thread { [ & ]( )->void {
		overlay->run( );
	} };

	create_thread( _HackThread );

	std::thread( overlay_thread ).join( );
} 