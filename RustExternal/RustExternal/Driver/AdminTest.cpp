#include <Windows.h>
#include <iostream>

#pragma comment(lib, "advapi32.lib")

bool IsRunningAsAdministrator() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    
    // Allocate and initialize a SID of the administrators group
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        // Determine whether the SID of administrators group is enabled in the primary access token
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }
    
    return isAdmin == TRUE;
}

int main() {
    printf("=== Administrator Privilege Test ===\n\n");
    
    bool isAdmin = IsRunningAsAdministrator();
    
    printf("Running as Administrator: %s\n", isAdmin ? "✅ YES" : "❌ NO");
    
    if (!isAdmin) {
        printf("\n🚨 SOLUTION FOUND: You need to run as Administrator!\n\n");
        printf("This explains why ALL memory operations are failing with Access Denied.\n");
        printf("Rust + EasyAntiCheat requires Administrator privileges for memory access.\n\n");
        printf("📋 Steps to fix:\n");
        printf("1. Close this window\n");
        printf("2. Right-click on Command Prompt\n");
        printf("3. Select 'Run as administrator'\n");
        printf("4. Navigate back to: E:\\Development\\RustExternal\\RustExternal\\RustExternal\\Driver\n");
        printf("5. Run the tests again\n\n");
        printf("🔧 Alternative: Run from Administrator PowerShell:\n");
        printf("   Start-Process cmd -Verb RunAs\n\n");
        printf("✅ Once running as Admin, all our userland memory access should work!\n");
    } else {
        printf("\n✅ Perfect! You're already running as Administrator.\n");
        printf("The memory access failures must be due to other protection mechanisms.\n");
        printf("Let's investigate further...\n");
    }
    
    printf("\nPress any key to exit...\n");
    getchar();
    return 0;
} 