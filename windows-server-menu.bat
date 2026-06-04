@echo off
setlocal EnableExtensions
chcp 65001 >nul

set "REPO=%~dp0"
if "%REPO:~-1%"=="\" set "REPO=%REPO:~0,-1%"

set "OUT_DIR=%REPO%\out"
set "MSYS_ROOT=C:\msys64"
set "MSYS_BASH=%MSYS_ROOT%\usr\bin\bash.exe"
set "SERVER_CONFIG=%OUT_DIR%\control-server.json"
set "SERVER_DB=%OUT_DIR%\server.db"
set "SOURCE_DB=%OUT_DIR%\original_server.db"
set "SERVER_HOOK_DLL=%OUT_DIR%\dinput8.dll"
set "RUNTIME_BIN=%OUT_DIR%\client\Binaries"
set "RUNTIME_HOOK_DLL=%RUNTIME_BIN%\dinput8.dll"
set "STALE_RUNTIME_VERSION=%RUNTIME_BIN%\version.dll"
set "STALE_RUNTIME_DINPUT8_ORIG=%RUNTIME_BIN%\dinput8_orig.dll"

call :ensure_server_db
if errorlevel 1 goto done
call :ensure_config
call :ensure_client_runtime
if errorlevel 1 goto done

:menu
cls
echo Global Agenda Private Server
echo.
echo Repo: %REPO%
echo Config: %SERVER_CONFIG%
if exist "%SERVER_DB%" (
    echo Server DB: %SERVER_DB%
) else (
    echo Server DB: missing - %SERVER_DB%
)
if exist "%MSYS_BASH%" (
    echo Build tools: %MSYS_ROOT%
) else (
    echo Build tools: not installed - option 1 installs MSYS2
)
echo.
echo Current config:
if exist "%SERVER_CONFIG%" (
    type "%SERVER_CONFIG%"
) else (
    echo Missing config file.
)
echo.
if exist "%MSYS_BASH%" (
    echo 1. Build all and update server hook DLL
) else (
    echo 1. Install build tools, build all, and update server hook DLL
)
echo 2. Run server
echo 3. Run server with game log window
echo 4. Exit
echo.
set /p "choice=Choose an option: "

if "%choice%"=="" goto done
if "%choice%"=="1" call :build_and_update & goto menu
if "%choice%"=="2" call :run_server_hidden & goto menu
if "%choice%"=="3" call :run_server_visible & goto menu
if "%choice%"=="4" goto done
goto menu

:ensure_server_db
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
if exist "%SERVER_DB%" exit /b 0
if exist "%SOURCE_DB%" (
    echo Creating server DB from:
    echo %SOURCE_DB%
    copy /Y "%SOURCE_DB%" "%SERVER_DB%" >nul
    exit /b %ERRORLEVEL%
)
echo No server DB found under:
echo %SERVER_DB%
echo The control server will create one on first startup.
exit /b 0

:ensure_config
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
if exist "%SERVER_CONFIG%" goto ensure_config_db_path
echo Creating default Windows config:
echo %SERVER_CONFIG%
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$cfg = [ordered]@{ " ^
  "wine_binary = ''; wine_prefix = ''; " ^
  "game_binary = 'out/client/Binaries/GlobalAgenda.exe'; " ^
  "host = '127.0.0.1'; hostdns = '127.0.0.1'; " ^
  "home_map_name = 'Dome3_VR_Arena_P'; home_map_game_mode = 'TgGame.TgGame_Mission'; " ^
  "wine_debug = $false; fix_package_guids = $false; " ^
  "udp_port_range = [ordered]@{ lo = 9002; hi = 9020 }; " ^
  "tcp_port = 9000; chat_port = 9001; ipc_port = 9011; " ^
  "admin_token = ''; db_path = $env:SERVER_DB; " ^
  "crash_dir = 'out/crashes'; log_dir = 'out/logs'; " ^
  "enabled_channels = [object[]]@(); enabled_crash_channels = [object[]]@(); " ^
  "clear_logs = $true; show_game_console = $false; allow_duplicate_account_logins = $false; " ^
  "use_docker = $false; docker_debug = $false; docker_image = 'ga-server:latest'; docker_memory = '0'; " ^
  "docker_extra_mounts = [object[]]@(); dll_overrides = ''; cores_per_instance = 1; per_slot_prefix = $false " ^
  "}; $json = $cfg | ConvertTo-Json -Depth 20; [IO.File]::WriteAllText($env:SERVER_CONFIG, $json + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))"
if errorlevel 1 exit /b %ERRORLEVEL%

:ensure_config_db_path
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference = 'Stop'; " ^
  "$cfg = Get-Content -LiteralPath $env:SERVER_CONFIG -Raw | ConvertFrom-Json; " ^
  "if ($cfg.PSObject.Properties.Name -contains 'db_path') { $cfg.db_path = $env:SERVER_DB } else { $cfg | Add-Member -NotePropertyName db_path -NotePropertyValue $env:SERVER_DB }; " ^
  "$json = $cfg | ConvertTo-Json -Depth 20; " ^
  "[IO.File]::WriteAllText($env:SERVER_CONFIG, $json + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))"
exit /b %ERRORLEVEL%

:ensure_client_runtime
powershell -NoProfile -ExecutionPolicy Bypass -Command "$client = Join-Path $env:OUT_DIR 'client'; $bin = Join-Path $client 'Binaries'; if ((Test-Path -LiteralPath (Join-Path $bin 'GlobalAgenda.exe')) -and (Test-Path -LiteralPath (Join-Path $client 'Engine')) -and (Test-Path -LiteralPath (Join-Path $client 'TgGame')) -and -not ((Get-Item -LiteralPath $client -Force).LinkType) -and -not ((Get-Item -LiteralPath $bin -Force).LinkType)) { exit 0 } exit 1"
if not errorlevel 1 exit /b 0

echo Missing local server runtime layout under:
echo %OUT_DIR%\client
echo.
echo Enter the full path to GlobalAgenda.exe from your real game install.
echo Example: F:\Games\Steam\steamapps\common\Global Agenda Live\Binaries\GlobalAgenda.exe
echo.
set "GAME_EXE_PATH="
set /p "GAME_EXE_PATH=GlobalAgenda.exe path: "
set "GAME_EXE_PATH=%GAME_EXE_PATH:"=%"
if "%GAME_EXE_PATH%"=="" (
    echo No path entered.
    pause
    exit /b 1
)
if not exist "%GAME_EXE_PATH%" (
    echo File not found:
    echo %GAME_EXE_PATH%
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference = 'Stop'; " ^
  "$gameExePath = $env:GAME_EXE_PATH.Trim().Trim([char]34); " ^
  "$exe = (Resolve-Path -LiteralPath $gameExePath).Path; " ^
  "if ([IO.Path]::GetFileName($exe) -ine 'GlobalAgenda.exe') { throw 'Path must point to GlobalAgenda.exe'; } " ^
  "$sourceBin = Split-Path -Parent $exe; " ^
  "if ([IO.Path]::GetFileName($sourceBin) -ine 'Binaries') { throw 'GlobalAgenda.exe must be inside the game Binaries folder'; } " ^
  "$sourceRoot = Split-Path -Parent $sourceBin; " ^
  "foreach ($name in @('Engine','TgGame')) { if (-not (Test-Path -LiteralPath (Join-Path $sourceRoot $name))) { throw ('Missing required game folder: ' + $name); } } " ^
  "$out = $env:OUT_DIR; $client = Join-Path $out 'client'; $runtimeBin = Join-Path $client 'Binaries'; " ^
  "if (Test-Path -LiteralPath $client) { " ^
  "  $item = Get-Item -LiteralPath $client -Force; " ^
  "  if ($item.LinkType) { cmd /c ('rmdir ""' + $client + '""') | Out-Null; if ($LASTEXITCODE -ne 0) { throw 'Failed to remove existing out\client junction'; } } " ^
  "  else { Remove-Item -LiteralPath $client -Recurse -Force; } " ^
  "} " ^
  "New-Item -ItemType Directory -Path $client -Force | Out-Null; " ^
  "Copy-Item -LiteralPath $sourceBin -Destination $runtimeBin -Recurse; " ^
  "foreach ($stale in @('dinput8.dll','version.dll','dinput8_orig.dll')) { $p = Join-Path $runtimeBin $stale; if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Force; } } " ^
  "$builtHook = Join-Path $out 'dinput8.dll'; if (Test-Path -LiteralPath $builtHook) { Copy-Item -LiteralPath $builtHook -Destination (Join-Path $runtimeBin 'dinput8.dll') -Force; } " ^
  "foreach ($name in @('Engine','TgGame','PreReqs')) { $target = Join-Path $sourceRoot $name; if (Test-Path -LiteralPath $target) { $link = Join-Path $client $name; New-Item -ItemType Junction -Path $link -Target $target | Out-Null; } }"
if errorlevel 1 (
    echo Failed to create local server runtime.
    pause
    exit /b 1
)
echo Created local server runtime:
echo %OUT_DIR%\client
exit /b 0

:check_msys
if exist "%MSYS_BASH%" (
    call :ensure_msys_packages
    exit /b %ERRORLEVEL%
)
call :install_msys
if errorlevel 1 exit /b 1
call :ensure_msys_packages
exit /b %ERRORLEVEL%

:install_msys
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
where winget >nul 2>nul
if errorlevel 1 (
    echo Missing MSYS2 build tools and winget is not available.
    echo Expected MSYS2 after install under:
    echo %MSYS_ROOT%
    pause
    exit /b 1
)
echo Installing MSYS2 using the default installer location:
echo %MSYS_ROOT%
winget install --id MSYS2.MSYS2 -e --accept-package-agreements --accept-source-agreements
if errorlevel 1 (
    echo MSYS2 install failed.
    pause
    exit /b 1
)
if not exist "%MSYS_BASH%" (
    echo MSYS2 installed, but bash was not found under:
    echo %MSYS_ROOT%
    pause
    exit /b 1
)
exit /b 0

:ensure_msys_packages
if exist "%MSYS_ROOT%\usr\bin\make.exe" if exist "%MSYS_ROOT%\mingw32\bin\i686-w64-mingw32-g++.exe" if exist "%MSYS_ROOT%\mingw64\bin\x86_64-w64-mingw32-g++.exe" exit /b 0
"%MSYS_BASH%" -lc "unset CONFIG; if [ ! -f /etc/pacman.d/gnupg/pubring.kbx ]; then pacman-key --init && pacman-key --populate msys2; fi; pacman -Sy --needed --noconfirm make mingw-w64-i686-gcc mingw-w64-x86_64-gcc"
if errorlevel 1 (
    echo MSYS2 package install/update failed.
    pause
    exit /b 1
)
exit /b 0

:msys_make
call :check_msys || exit /b 1
set "MAKE_TARGETS=%~1"
"%MSYS_BASH%" -lc "unset CONFIG; REPO_UNIX=$(cygpath -u '%REPO%'); export HOME=\"$REPO_UNIX/out/msys2-home\" TMPDIR=\"$REPO_UNIX/out/msys2-tmp\" TMP=\"$REPO_UNIX/out/msys2-tmp\" TEMP=\"$REPO_UNIX/out/msys2-tmp\"; mkdir -p \"$HOME\" \"$TMPDIR\"; export PATH=/mingw32/bin:/mingw64/bin:/usr/bin:$PATH; cd \"$REPO_UNIX\" && make -j4 %MAKE_TARGETS%"
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
)
echo Build completed.
pause
exit /b 0

:build_and_update
call :kill_stale_server_instances
if errorlevel 1 exit /b 1
call :msys_make "control-server-win all"
if errorlevel 1 exit /b 1
call :copy_server_dll
exit /b %errorlevel%

:copy_server_dll
powershell -NoProfile -ExecutionPolicy Bypass -Command "if ((Get-Item -LiteralPath (Join-Path $env:OUT_DIR 'client') -Force).LinkType) { exit 1 }"
if errorlevel 1 (
    echo Refusing to copy server hook because out\client is a junction.
    echo Keep out\client\Binaries local, and only junction content folders like Engine and TgGame.
    pause
    exit /b 1
)
if not exist "%SERVER_HOOK_DLL%" (
    echo Missing built server hook DLL: %SERVER_HOOK_DLL%
    echo Choose "Build all and update server hook DLL" first.
    pause
    exit /b 1
)
if not exist "%RUNTIME_BIN%\GlobalAgenda.exe" (
    echo Missing server runtime executable: %RUNTIME_BIN%\GlobalAgenda.exe
    pause
    exit /b 1
)
call :kill_stale_server_instances
if errorlevel 1 exit /b 1
copy /Y "%SERVER_HOOK_DLL%" "%RUNTIME_HOOK_DLL%" >nul
if errorlevel 1 (
    echo Failed to copy server hook DLL.
    pause
    exit /b 1
)
if exist "%STALE_RUNTIME_VERSION%" del /F /Q "%STALE_RUNTIME_VERSION%"
if exist "%STALE_RUNTIME_DINPUT8_ORIG%" del /F /Q "%STALE_RUNTIME_DINPUT8_ORIG%"
echo Copied server hook DLL to:
echo %RUNTIME_HOOK_DLL%
pause
exit /b 0

:run_server_hidden
set "GA_SHOW_GAME_CONSOLE=0"
call :run_server
set "RUN_RC=%ERRORLEVEL%"
set "GA_SHOW_GAME_CONSOLE="
exit /b %RUN_RC%

:run_server_visible
set "GA_SHOW_GAME_CONSOLE=1"
call :run_server
set "RUN_RC=%ERRORLEVEL%"
set "GA_SHOW_GAME_CONSOLE="
exit /b %RUN_RC%

:run_server
if not exist "%OUT_DIR%\control-server.exe" (
    echo Missing control server: %OUT_DIR%\control-server.exe
    echo Choose "Build all and update server hook DLL" first.
    pause
    exit /b 1
)
if not exist "%SERVER_CONFIG%" (
    call :ensure_config
    if errorlevel 1 exit /b 1
)
call :ensure_server_db
if errorlevel 1 exit /b 1
call :kill_stale_server_instances
if errorlevel 1 exit /b 1
cd /d "%REPO%"
echo Starting control server. Press Ctrl+C to stop.
if "%GA_SHOW_GAME_CONSOLE%"=="1" (
    echo Game instance log windows: visible
) else (
    echo Game instance log windows: hidden
)
echo.
"%OUT_DIR%\control-server.exe" --config "%SERVER_CONFIG%"
echo.
echo Control server exited.
pause
exit /b 0

:kill_stale_server_instances
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference = 'Stop'; " ^
  "if (-not (Test-Path -LiteralPath $env:RUNTIME_BIN)) { exit 0 }; " ^
  "$targetExe = [IO.Path]::GetFullPath((Join-Path $env:RUNTIME_BIN 'GlobalAgenda.exe')); " ^
  "$filter = 'Name = ''GlobalAgenda.exe'''; " ^
  "$find = { Get-CimInstance Win32_Process -Filter $filter -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -and ([IO.Path]::GetFullPath($_.ExecutablePath) -ieq $targetExe) } }; " ^
  "$procs = @(& $find); " ^
  "foreach ($p in $procs) { Write-Host ('Stopping stale server game instance PID ' + $p.ProcessId + ': ' + $p.ExecutablePath); Stop-Process -Id $p.ProcessId -Force -ErrorAction SilentlyContinue; } " ^
  "Start-Sleep -Milliseconds 300; " ^
  "$remaining = @(& $find); " ^
  "if ($remaining.Count -gt 0) { foreach ($p in $remaining) { Write-Host ('Still running server game instance PID ' + $p.ProcessId + ': ' + $p.ExecutablePath); }; exit 1 }; " ^
  "exit 0"
if errorlevel 1 (
    echo Failed to check or stop stale server game instances.
    pause
    exit /b 1
)
exit /b 0

:done
endlocal
