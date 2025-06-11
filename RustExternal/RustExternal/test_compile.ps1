# Test compilation of main project
Write-Host "=== Testing Main Project Compilation ===" -ForegroundColor Yellow

# Try to find Visual Studio
$vsPath = $null
$vsPaths = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "${env:ProgramFiles}\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)

foreach ($path in $vsPaths) {
    if (Test-Path $path) {
        $vsPath = $path
        break
    }
}

if ($vsPath) {
    Write-Host "[+] Found Visual Studio" -ForegroundColor Green
    
    # Try to compile just the files that had errors to test fixes
    $batchContent = @"
@echo off
call "$vsPath"
echo Testing kinterface.cpp compilation...
cl /EHsc /c Driver\kinterface.cpp /I. 
if %ERRORLEVEL% EQU 0 (
    echo [+] kinterface.cpp compiles successfully!
) else (
    echo [-] kinterface.cpp still has errors
)

echo Testing main.cpp compilation...
cl /EHsc /c main.cpp /I. 
if %ERRORLEVEL% EQU 0 (
    echo [+] main.cpp compiles successfully!
) else (
    echo [-] main.cpp still has errors
)

echo Testing Overlay.cpp compilation...
cl /EHsc /c Overlay\Overlay.cpp /I. 
if %ERRORLEVEL% EQU 0 (
    echo [+] Overlay.cpp compiles successfully!
) else (
    echo [-] Overlay.cpp still has errors
)
"@
    
    $batchContent | Out-File -FilePath "test_compile.bat" -Encoding ASCII
    & cmd /c "test_compile.bat"
    Remove-Item "test_compile.bat" -ErrorAction SilentlyContinue
    
} else {
    Write-Host "[-] Visual Studio not found!" -ForegroundColor Red
}

Write-Host "`n=== Compilation Test Complete ===" -ForegroundColor Yellow 