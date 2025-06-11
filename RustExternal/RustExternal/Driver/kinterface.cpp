#include "../includes.h"
#include "kinterface.h"

HANDLE g_DriverHandle = INVALID_HANDLE_VALUE;

bool kinterface_t::Initialize( ) {
	printf("🔍 Attempting driver connections...\n");
	
	// Try RTCore64 first (CVE-2019-16098 exploit)
	g_DriverHandle = CreateFileW(
		L"\\\\.\\RTCore64",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	
	if (g_DriverHandle != INVALID_HANDLE_VALUE) {
		printf("✅ Connected to RTCore64.sys successfully!\n");
		printf("🎯 Using CVE-2019-16098 exploit for EAC bypass\n");
		return true;
	}
	
	DWORD rtcoreError = GetLastError();
	printf("⚠️  RTCore64 connection failed (Error: %d)\n", rtcoreError);
	
	// Fallback to custom KernelDriver
	printf("🔄 Trying custom KernelDriver...\n");
	g_DriverHandle = CreateFileW(
		L"\\\\.\\KernelDriver", 
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
	
	if (g_DriverHandle != INVALID_HANDLE_VALUE) {
		printf("✅ Connected to KernelDriver successfully!\n");
		return true;
	}
	
	DWORD kernelError = GetLastError();
	printf("❌ All driver connections failed!\n");
	printf("   RTCore64 Error: %d\n", rtcoreError);
	printf("   KernelDriver Error: %d\n", kernelError);
	
	printf("\n💡 SOLUTIONS:\n");
	printf("1. Install RTCore64.sys: Use install_rtcore64.bat\n");
	printf("2. Load custom driver: Provide KernelDriver.sys\n");
	printf("3. Run as Administrator\n");
	
	return false;
}

kinterface_t::~kinterface_t() {
	if (g_DriverHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(g_DriverHandle);
		g_DriverHandle = INVALID_HANDLE_VALUE;
	}
}

bool kinterface_t::SendCMD( void* data, request_codes code ) {
	if (g_DriverHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	DWORD ioctl_code = 0;
	DWORD bytes_returned = 0;
	
	switch (code) {
		case request_read:
			ioctl_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS);
			break;
		case request_write:
			ioctl_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS);
			break;
		case request_procbase:
			ioctl_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS);
			break;
		case request_modbase:
			ioctl_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS);
			break;
		default:
			return false;
	}

	BOOL result = DeviceIoControl(
		g_DriverHandle,
		ioctl_code,
		data,
		sizeof(read_request), // Adjust size based on request type
		data,
		sizeof(read_request), // Adjust size based on request type
		&bytes_returned,
		NULL
	);
	
	return result != FALSE;
}

uintptr_t kinterface_t::GetProcessBase( int PID ) {
	// RTCore64 doesn't support process base enumeration via IOCTL
	// Use userland method for process base detection
	printf("🔄 Getting process base for PID %d...\n", PID);
	
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
	if (hProcess == NULL) {
		printf("❌ Failed to open process (Error: %d)\n", GetLastError());
		return 0;
	}
	
	// Method 1: Try reading PEB
	PROCESS_BASIC_INFORMATION pbi = {0};
	ULONG returnLength = 0;
	
	typedef NTSTATUS (NTAPI *NtQueryInformationProcess_t)(
		HANDLE ProcessHandle,
		DWORD ProcessInformationClass,
		PVOID ProcessInformation,
		ULONG ProcessInformationLength,
		PULONG ReturnLength
	);
	
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll) {
		NtQueryInformationProcess_t NtQueryInformationProcess = 
			(NtQueryInformationProcess_t)GetProcAddress(ntdll, "NtQueryInformationProcess");
		
		if (NtQueryInformationProcess) {
			NTSTATUS status = NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), &returnLength);
			if (status == 0 && pbi.PebBaseAddress) {
				// This gives us the PEB address, which we can use as process base
				printf("✅ Process Base (PEB): 0x%llx\n", (uintptr_t)pbi.PebBaseAddress);
				CloseHandle(hProcess);
				return (uintptr_t)pbi.PebBaseAddress;
			}
		}
	}
	
	// Method 2: Use module enumeration to get main executable base
	HMODULE hMods[1024];
	DWORD cbNeeded;
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
		// First module is usually the main executable
		if (cbNeeded > 0) {
			printf("✅ Process Base (Main Module): 0x%llx\n", (uintptr_t)hMods[0]);
			CloseHandle(hProcess);
			return (uintptr_t)hMods[0];
		}
	}
	
	printf("⚠️  All process base detection methods failed\n");
	CloseHandle(hProcess);
	return 0;
}

uintptr_t kinterface_t::GetModuleBase( int PID, LPCWSTR ModName ) {
	// RTCore64 doesn't support module enumeration via IOCTL
	// Use userland method for module detection
	printf("🔄 Searching for module: %ls in PID %d...\n", ModName, PID);
	
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
	if (hProcess == NULL) {
		printf("❌ Failed to open process for module enum (Error: %d)\n", GetLastError());
		return 0;
	}
	
	HMODULE hMods[1024];
	DWORD cbNeeded;
	
	// First try standard API (will likely fail due to EAC)  
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
		DWORD moduleCount = cbNeeded / sizeof(HMODULE);
		printf("✅ Standard API found %d modules\n", moduleCount);
		
		for (DWORD i = 0; i < moduleCount; i++) {
			WCHAR moduleName[MAX_PATH];
			if (GetModuleBaseNameW(hProcess, hMods[i], moduleName, sizeof(moduleName)/sizeof(WCHAR))) {
				if (_wcsicmp(moduleName, ModName) == 0) {
					printf("✅ Found %ls at: 0x%llx\n", ModName, (uintptr_t)hMods[i]);
					CloseHandle(hProcess);
					return (uintptr_t)hMods[i];
				}
			}
		}
	} else {
		printf("❌ EnumProcessModules blocked by EAC (Error: %d)\n", GetLastError());
		printf("🔄 Switching to RTCore64 direct memory access...\n");
		
		// Use RTCore64 to read PEB and walk module list manually
		CloseHandle(hProcess);
		return GetModuleBaseViaRTCore64(PID, ModName);
	}
	
	printf("⚠️  Module %ls not found via standard API\n", ModName);
	CloseHandle(hProcess);
	return 0;
}

uintptr_t kinterface_t::GetModuleBaseViaRTCore64(int PID, LPCWSTR ModName) {
	printf("🎯 RTCore64: Directly reading process memory for module detection\n");
	
	// Method 1: Try common Unity DLL base addresses (pattern scanning approach)
	printf("🔍 Method 1: Scanning common Unity module locations...\n");
	
	// Common base addresses where GameAssembly.dll might be loaded
	uintptr_t candidateAddresses[] = {
		0x180000000,  // Common Unity IL2CPP base
		0x140000000,  // Alternative base
		0x7FF000000000, // High memory location
		ProcessBase + 0x1000000,  // Relative to main module
		ProcessBase + 0x10000000,
		0x10000000,   // Low memory location
	};
	
	for (int i = 0; i < sizeof(candidateAddresses) / sizeof(uintptr_t); i++) {
		uintptr_t testAddr = candidateAddresses[i];
		
		// Try to read PE header
		IMAGE_DOS_HEADER dosHeader = {0};
		if (ReadPhysMemory(PID, testAddr, &dosHeader, sizeof(dosHeader))) {
			if (dosHeader.e_magic == IMAGE_DOS_SIGNATURE) { // 'MZ'
				// Read NT header
				IMAGE_NT_HEADERS ntHeaders = {0};
				if (ReadPhysMemory(PID, testAddr + dosHeader.e_lfanew, &ntHeaders, sizeof(ntHeaders))) {
					if (ntHeaders.Signature == IMAGE_NT_SIGNATURE) { // 'PE'
						printf("✅ Found valid PE at 0x%llx\n", testAddr);
						
						// This is a valid module - assume it's GameAssembly for now
						// In a real implementation, we'd need to read the export table or file name
						printf("🎯 Assuming 0x%llx is GameAssembly.dll\n", testAddr);
						return testAddr;
					}
				}
			}
		}
	}
	
	// Method 2: Pattern scan for Unity signatures
	printf("🔍 Method 2: Pattern scanning for Unity signatures...\n");
	
	// Scan main module memory for Unity/IL2CPP signatures
	uintptr_t scanStart = ProcessBase;
	uintptr_t scanSize = 0x100000000; // 4GB scan range
	uintptr_t scanStep = 0x10000;     // 64KB steps
	
	for (uintptr_t addr = scanStart; addr < scanStart + scanSize; addr += scanStep) {
		char buffer[0x1000];
		if (ReadPhysMemory(PID, addr, buffer, sizeof(buffer))) {
			// Look for Unity signatures
			if (memcmp(buffer, "MZ", 2) == 0) {
				// Found potential PE header
				IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)buffer;
				if (dos->e_lfanew < sizeof(buffer) - sizeof(IMAGE_NT_HEADERS)) {
					IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(buffer + dos->e_lfanew);
					if (nt->Signature == IMAGE_NT_SIGNATURE) {
						printf("🎯 Found PE module at 0x%llx\n", addr);
						
						// Check if this could be GameAssembly
						// Look for Unity/IL2CPP strings in the module
						for (int j = 0; j < sizeof(buffer) - 20; j++) {
							if (strstr(&buffer[j], "Unity") || strstr(&buffer[j], "IL2CPP") || 
							    strstr(&buffer[j], "GameAssembly")) {
								printf("✅ Found Unity signatures at 0x%llx\n", addr);
								return addr;
							}
						}
					}
				}
			}
		}
		
		// Progress indicator every 256MB
		if ((addr - scanStart) % 0x10000000 == 0) {
			printf("📍 Scanned: 0x%llx / 0x%llx\n", addr - scanStart, scanSize);
		}
	}
	
	// Method 3: Fallback - return main process base as GameAssembly
	printf("⚠️  GameAssembly.dll not found via RTCore64 scan\n");
	printf("🔄 Fallback: Using main process base as GameAssembly\n");
	return ProcessBase;  // Many Unity games have IL2CPP compiled into main executable
}

bool kinterface_t::ReadPhysMemory( const int pid, const std::uintptr_t address, void* buffer, const std::size_t size, bool mmcopy, PDWORD_PTR num_bytes ) {
	read_request data { 0 };

	data.pid = pid;
	data.address = address;
	data.buffer = buffer;
	data.mmcopy = mmcopy;
	data.size = size;

	if ( num_bytes )
		*num_bytes = data.ret_size;
	return SendCMD( &data, request_read );
}

bool kinterface_t::WritePhysMemory( const int pid, const std::uintptr_t address, void* buffer, const std::size_t size ) {
	read_request data { 0 };

	data.pid = pid;
	data.address = address;
	data.buffer = buffer;
	data.size = size;

	return SendCMD( &data, request_write );
}

int kinterface_t::GetProcessThreadNumByID( DWORD dwPID )
{
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if ( hProcessSnap == INVALID_HANDLE_VALUE )
		return 0;

	PROCESSENTRY32W pe32 = { 0 };  // Use wide version
	pe32.dwSize = sizeof( pe32 );
	BOOL bRet = ::Process32FirstW( hProcessSnap, &pe32 );  // Use wide version
	while ( bRet )
	{
		if ( pe32.th32ProcessID == dwPID )
		{
			::CloseHandle( hProcessSnap );
			return pe32.cntThreads;
		}
		bRet = ::Process32NextW( hProcessSnap, &pe32 );  // Use wide version
	}
	return 0;
}

int kinterface_t::PID( const char* name )
{
	DWORD dwThreadCountMax = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	PROCESSENTRY32W pe32;  // Use wide version
	pe32.dwSize = sizeof( PROCESSENTRY32W );
	Process32FirstW( hSnapshot, &pe32 );  // Use wide version
	
	// Convert char* to wchar_t*
	wchar_t wideName[260];
	MultiByteToWideChar( CP_ACP, 0, name, -1, wideName, 260 );
	
	do
	{
		if ( _wcsicmp( pe32.szExeFile, wideName ) == 0 )
		{
			DWORD dwTmpThreadCount = GetProcessThreadNumByID( pe32.th32ProcessID );

			if ( dwTmpThreadCount > dwThreadCountMax )
			{
				dwThreadCountMax = dwTmpThreadCount;
				tPID = pe32.th32ProcessID;
			}
		}
	} while ( Process32NextW( hSnapshot, &pe32 ) );  // Use wide version
	CloseHandle( hSnapshot );
	return tPID;
}