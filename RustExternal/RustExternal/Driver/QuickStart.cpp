#include "../includes.h"

// Quick Start: Using RTCore64 (MSI Afterburner) for Ring 0 Access
// No custom driver needed!

int main() {
    printf("=== Quick Start: Ring 0 Memory Access ===\n\n");
    
    // Step 1: Initialize the vulnerable driver interface
    printf("[1] Initializing vulnerable driver interface...\n");
    
    if (!kinterface->Initialize()) {
        printf("[-] Failed to connect to vulnerable driver!\n");
        printf("[!] Make sure you're running as Administrator\n");
        printf("[!] Make sure MSI Afterburner is installed\n");
        return 1;
    }
    
    printf("[+] Successfully connected to vulnerable driver!\n\n");
    
    // Step 2: Find target process (Rust in this case)
    printf("[2] Finding target process...\n");
    
    // For demonstration, let's target our own process first
    DWORD testPID = GetCurrentProcessId();
    printf("[*] Using current process PID: %d for testing\n", testPID);
    
    // Step 3: Test memory reading
    printf("\n[3] Testing memory read capabilities...\n");
    
    // Read our own DOS header to verify functionality
    HMODULE hModule = GetModuleHandle(NULL);
    IMAGE_DOS_HEADER dosHeader;
    
    bool success = kinterface->ReadPhysMemory(
        testPID,
        (uintptr_t)hModule,
        &dosHeader,
        sizeof(dosHeader)
    );
    
    if (success && dosHeader.e_magic == IMAGE_DOS_SIGNATURE) {
        printf("[+] Memory read working! DOS signature: 0x%04X\n", dosHeader.e_magic);
        
        // Step 4: Try to find Rust process
        printf("\n[4] Looking for Rust process...\n");
        
        int rustPID = kinterface->PID("RustClient.exe");
        if (rustPID > 0) {
            printf("[+] Found RustClient.exe! PID: %d\n", rustPID);
            
            // Now you can read Rust memory directly!
            printf("[*] Ready to read Rust memory with kernel privileges!\n");
            
            // Example: Read first 64 bytes of Rust executable
            char buffer[64];
            if (kinterface->ReadPhysMemory(rustPID, 0x140000000, buffer, sizeof(buffer))) {
                printf("[+] Successfully read Rust memory!\n");
                printf("[*] First few bytes: ");
                for (int i = 0; i < 16; i++) {
                    printf("%02X ", (unsigned char)buffer[i]);
                }
                printf("\n");
            }
            
        } else {
            printf("[*] RustClient.exe not running (start Rust to test)\n");
            
            // Show available processes for reference
            printf("\n[*] Some running processes:\n");
            int notepadPID = kinterface->PID("notepad.exe");
            int chromeReindeerPID = kinterface->PID("chrome.exe");
            
            if (notepadPID > 0) printf("    - notepad.exe: %d\n", notepadPID);
            if (chromeReindeerPID > 0) printf("    - chrome.exe: %d\n", chromeReindeerPID);
        }
        
    } else {
        printf("[-] Memory read failed!\n");
        return 1;
    }
    
    printf("\n=== Integration Guide ===\n");
    printf("[*] To use in your existing project:\n");
    printf("    1. Replace your old kinterface calls with the updated version\n");
    printf("    2. Make sure to run your program as Administrator\n");
    printf("    3. Use kinterface->ReadPhysMemory() / WritePhysMemory() as before\n");
    printf("    4. No driver loading or installation needed!\n");
    
    printf("\n[+] SUCCESS! Vulnerable driver approach is working!\n");
    printf("[+] You now have Ring 0 memory access without custom drivers!\n");
    
    return 0;
}

// Example usage functions for your hack:

void ExampleRustHack() {
    // Initialize once at startup
    if (!kinterface->Initialize()) {
        printf("Failed to initialize driver interface\n");
        return;
    }
    
    // Find Rust process
    int rustPID = kinterface->PID("RustClient.exe");
    if (rustPID <= 0) {
        printf("Rust not running\n");
        return;
    }
    
    // Read player health (example address)
    uintptr_t healthAddress = 0x12345678; // Replace with real address
    float health = 0;
    
    if (kinterface->ReadPhysMemory(rustPID, healthAddress, &health, sizeof(health))) {
        printf("Player health: %.1f\n", health);
    }
    
    // Write god mode (example)
    float maxHealth = 100.0f;
    if (kinterface->WritePhysMemory(rustPID, healthAddress, &maxHealth, sizeof(maxHealth))) {
        printf("God mode activated!\n");
    }
}

// Template functions for easy use
template<typename T>
T ReadMemory(uintptr_t address) {
    T value{};
    kinterface->ReadPhysMemory(kinterface->tPID, address, &value, sizeof(T));
    return value;
}

template<typename T>
bool WriteMemory(uintptr_t address, T value) {
    return kinterface->WritePhysMemory(kinterface->tPID, address, &value, sizeof(T));
} 