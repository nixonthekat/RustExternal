#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "psapi.lib")

class RobustMemoryScanner {
private:
    HANDLE hProcess = INVALID_HANDLE_VALUE;
    DWORD processId = 0;
    
public:
    ~RobustMemoryScanner() {
        if (hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(hProcess);
        }
    }
    
    bool Initialize(const char* processName) {
        printf("=== Robust Memory Scanner for Issue #2 ===\n\n");
        
        processId = FindProcessByName(processName);
        if (processId == 0) {
            printf("[-] Process %s not found!\n", processName);
            return false;
        }
        printf("[+] Found %s (PID: %d)\n", processName, processId);
        
        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
        if (hProcess == INVALID_HANDLE_VALUE) {
            printf("[-] Failed to open process! Error: %d\n", GetLastError());
            return false;
        }
        printf("[+] Process opened successfully\n");
        
        return true;
    }
    
    void ScanForGameAssembly() {
        printf("\n=== Comprehensive GameAssembly.dll Search ===\n");
        
        // Method 1: Query memory regions using VirtualQueryEx
        printf("\n[1] Scanning via VirtualQueryEx...\n");
        ScanMemoryRegions();
        
        // Method 2: Systematic address space scanning
        printf("\n[2] Systematic address space scanning...\n");
        ScanAddressSpace();
        
        // Method 3: Known address patterns
        printf("\n[3] Testing known Unity/GameAssembly patterns...\n");
        TestKnownPatterns();
    }
    
private:
    DWORD FindProcessByName(const char* processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
        
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        DWORD targetPid = 0;
        DWORD maxThreads = 0;
        
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (_stricmp(pe32.szExeFile, processName) == 0) {
                    if (pe32.cntThreads > maxThreads) {
                        maxThreads = pe32.cntThreads;
                        targetPid = pe32.th32ProcessID;
                    }
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        
        CloseHandle(hSnapshot);
        return targetPid;
    }
    
    void ScanMemoryRegions() {
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t address = 0;
        int regionCount = 0;
        int validPECount = 0;
        
        while (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi))) {
            regionCount++;
            
            // Look for committed memory regions that could contain modules
            if (mbi.State == MEM_COMMIT && 
                (mbi.Type == MEM_IMAGE || mbi.Type == MEM_PRIVATE) &&
                mbi.RegionSize > 0x100000) { // At least 1MB
                
                printf("  Region %d: 0x%llx - 0x%llx (Size: 0x%zx, Type: %s)\n",
                       regionCount, (uintptr_t)mbi.BaseAddress, 
                       (uintptr_t)mbi.BaseAddress + mbi.RegionSize,
                       mbi.RegionSize,
                       (mbi.Type == MEM_IMAGE) ? "IMAGE" : "PRIVATE");
                
                // Check if this region contains a PE header
                if (CheckForPEHeader((uintptr_t)mbi.BaseAddress)) {
                    validPECount++;
                    
                    // Check if this might be GameAssembly based on size
                    if (mbi.RegionSize > 0x1000000 && mbi.RegionSize < 0x20000000) {
                        printf("    ✅ Potential GameAssembly candidate!\n");
                        AnalyzePEModule((uintptr_t)mbi.BaseAddress, mbi.RegionSize);
                    } else {
                        printf("    📋 Valid PE module (size: %.1f MB)\n", 
                               mbi.RegionSize / (1024.0 * 1024.0));
                    }
                }
            }
            
            address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            
            // Safety check to avoid infinite loop
            if (address >= 0x7FFFFFFFFFFF || regionCount > 1000) break;
        }
        
        printf("  Scanned %d regions, found %d PE modules\n", regionCount, validPECount);
    }
    
    void ScanAddressSpace() {
        // Scan common 64-bit address ranges where Unity/GameAssembly typically loads
        std::vector<std::pair<uintptr_t, uintptr_t>> ranges = {
            {0x140000000, 0x150000000},    // Common Unity range
            {0x7FF000000000, 0x7FF800000000}, // High memory range
            {0x180000000, 0x200000000},    // Alternative range
        };
        
        for (const auto& range : ranges) {
            printf("  Scanning range 0x%llx - 0x%llx...\n", range.first, range.second);
            
            for (uintptr_t addr = range.first; addr < range.second; addr += 0x10000) {
                if (CheckForPEHeader(addr)) {
                    uint32_t size = GetPESize(addr);
                    if (size > 0x1000000 && size < 0x20000000) { // 16MB - 512MB
                        printf("    🎯 Found large PE at 0x%llx (Size: %.1f MB)\n", 
                               addr, size / (1024.0 * 1024.0));
                        AnalyzePEModule(addr, size);
                    }
                }
            }
        }
    }
    
    void TestKnownPatterns() {
        // Test some common Unity/GameAssembly loading addresses
        std::vector<uintptr_t> testAddresses = {
            0x140000000,
            0x180000000,
            0x1C0000000,
            0x200000000,
            0x7FF640000000,
            0x7FF680000000,
            0x7FF6C0000000,
            0x7FF700000000
        };
        
        for (uintptr_t addr : testAddresses) {
            if (CheckForPEHeader(addr)) {
                uint32_t size = GetPESize(addr);
                printf("  ✅ PE found at 0x%llx (Size: %.1f MB)\n", 
                       addr, size / (1024.0 * 1024.0));
                
                if (size > 0x1000000) {
                    AnalyzePEModule(addr, size);
                }
            }
        }
    }
    
    bool CheckForPEHeader(uintptr_t address) {
        // Read DOS header
        uint16_t dosSignature = 0;
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)address, &dosSignature, sizeof(dosSignature), &bytesRead)) {
            return false;
        }
        
        if (dosSignature != 0x5A4D) return false; // "MZ"
        
        // Read PE offset
        uint32_t peOffset = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)(address + 0x3C), &peOffset, sizeof(peOffset), &bytesRead)) {
            return false;
        }
        
        if (peOffset >= 0x1000) return false; // PE offset too large
        
        // Read PE signature
        uint32_t peSignature = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)(address + peOffset), &peSignature, sizeof(peSignature), &bytesRead)) {
            return false;
        }
        
        return peSignature == 0x00004550; // "PE\0\0"
    }
    
    uint32_t GetPESize(uintptr_t address) {
        uint32_t peOffset = 0;
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)(address + 0x3C), &peOffset, sizeof(peOffset), &bytesRead)) {
            return 0;
        }
        
        uint32_t sizeOfImage = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)(address + peOffset + 0x18 + 0x38), &sizeOfImage, sizeof(sizeOfImage), &bytesRead)) {
            return 0;
        }
        
        return sizeOfImage;
    }
    
    void AnalyzePEModule(uintptr_t address, size_t size) {
        printf("      🔍 Analyzing PE module at 0x%llx...\n", address);
        
        // Try to read some characteristics to identify if it's GameAssembly
        
        // 1. Check for Unity-specific patterns in the first few MB
        std::vector<uint8_t> buffer(0x100000); // 1MB buffer
        SIZE_T bytesRead = 0;
        if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer.data(), buffer.size(), &bytesRead)) {
            
            // Look for "Unity" string
            std::string unityStr = "Unity";
            for (size_t i = 0; i <= bytesRead - unityStr.length(); i++) {
                if (memcmp(buffer.data() + i, unityStr.c_str(), unityStr.length()) == 0) {
                    printf("      ✅ Found 'Unity' string - likely Unity module!\n");
                    break;
                }
            }
            
            // Look for "GameAssembly" string
            std::string gameAssemblyStr = "GameAssembly";
            for (size_t i = 0; i <= bytesRead - gameAssemblyStr.length(); i++) {
                if (memcmp(buffer.data() + i, gameAssemblyStr.c_str(), gameAssemblyStr.length()) == 0) {
                    printf("      🎯 Found 'GameAssembly' string - THIS IS IT!\n");
                    printf("      🚀 GameAssembly.dll located at: 0x%llx (Size: %.1f MB)\n", 
                           address, size / (1024.0 * 1024.0));
                    return;
                }
            }
            
            // Look for Il2Cpp patterns
            std::string il2cppStr = "il2cpp";
            for (size_t i = 0; i <= bytesRead - il2cppStr.length(); i++) {
                if (memcmp(buffer.data() + i, il2cppStr.c_str(), il2cppStr.length()) == 0) {
                    printf("      ✅ Found 'il2cpp' - likely GameAssembly!\n");
                    break;
                }
            }
        }
    }
};

int main() {
    printf("=== Robust Memory Scanner - Solving Issue #2 ===\n\n");
    
    RobustMemoryScanner scanner;
    
    if (!scanner.Initialize("RustClient.exe")) {
        printf("❌ Failed to initialize scanner!\n");
        getchar();
        return 1;
    }
    
    scanner.ScanForGameAssembly();
    
    printf("\n=== Scan Complete ===\n");
    printf("🎯 This scan should reveal where GameAssembly.dll is located!\n");
    
    printf("\nPress any key to exit...\n");
    getchar();
    return 0;
} 