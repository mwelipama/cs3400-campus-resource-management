@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
set "LOG=%~dp0build_log.txt"
echo CS3400 GUI build log  %DATE% %TIME% > "%LOG%"
echo [..] Searching for Qt (qmake.exe). The full-disk fallback can take a few minutes...
echo Searching for qmake.exe...>> "%LOG%"

REM ================= 1. locate qmake.exe =================
set "QMAKE="
for %%P in ("C:\Qt" "D:\Qt" "C:\Qt6" "C:\dev\Qt" "D:\dev\Qt" "C:\Program Files\Qt" "C:\Program Files (x86)\Qt" "%USERPROFILE%\Qt" "%LOCALAPPDATA%\Qt" "%USERPROFILE%\OneDrive\Desktop\MECP 2026" "%USERPROFILE%\Desktop") do (
  if not defined QMAKE if exist "%%~P" (
    for /f "delims=" %%Q in ('dir /s /b "%%~P\qmake.exe" 2^>nul') do if not defined QMAKE set "QMAKE=%%Q"
  )
)
REM definitive fallback: scan the whole C: then D: (slow)
if not defined QMAKE (
  echo [..] candidate dirs had no qmake; scanning all of C:\ ...>> "%LOG%"
  for /f "delims=" %%Q in ('where /r C:\ qmake.exe 2^>nul') do if not defined QMAKE set "QMAKE=%%Q"
)
if not defined QMAKE (
  for /f "delims=" %%Q in ('where /r D:\ qmake.exe 2^>nul') do if not defined QMAKE set "QMAKE=%%Q"
)

if not defined QMAKE (
  echo [X] qmake.exe not found anywhere on C: or D:. Qt does not appear to be installed as a buildable kit.>> "%LOG%"
  echo [X] qmake.exe NOT FOUND - Qt may not be fully installed. See build_log.txt
  type "%LOG%" & pause & exit /b 1
)
for %%I in ("%QMAKE%") do set "QTBIN=%%~dpI"
set "QTBIN=%QTBIN:~0,-1%"
echo [OK] qmake : %QMAKE%>> "%LOG%"
echo [OK] QTBIN : %QTBIN%>> "%LOG%"

REM ================= 2. locate mingw32-make.exe =================
set "MAKE="
for %%P in ("C:\Qt\Tools" "D:\Qt\Tools" "C:\dev\Qt\Tools" "%USERPROFILE%\Qt\Tools" "C:\Program Files\Qt\Tools" "C:\Program Files (x86)\Qt\Tools" "%USERPROFILE%\OneDrive\Desktop\MECP 2026\mingw64" "%USERPROFILE%\OneDrive\Desktop\MECP 2026") do (
  if not defined MAKE if exist "%%~P" (
    for /f "delims=" %%Q in ('dir /s /b "%%~P\mingw32-make.exe" 2^>nul') do if not defined MAKE set "MAKE=%%Q"
  )
)
if not defined MAKE where mingw32-make.exe >nul 2>&1 && for /f "delims=" %%p in ('where mingw32-make.exe') do if not defined MAKE set "MAKE=%%p"
REM also accept plain mingw make (standalone mingw64 ships 'mingw32-make.exe' in its bin)
if not defined MAKE (
  for /f "delims=" %%Q in ('where /r "%USERPROFILE%\OneDrive\Desktop\MECP 2026\mingw64" mingw32-make.exe 2^>nul') do if not defined MAKE set "MAKE=%%Q"
)

echo %QTBIN% | find /i "mingw" >nul
if errorlevel 1 echo [!] Kit at %QTBIN% looks like MSVC, not MinGW. This script targets the MinGW kit.>> "%LOG%"
if not defined MAKE (
  echo [X] mingw32-make.exe not found. The Qt kit may be MSVC, or MinGW is elsewhere.>> "%LOG%"
  echo [X] mingw32-make NOT FOUND - see build_log.txt
  type "%LOG%" & pause & exit /b 1
)
for %%I in ("%MAKE%") do set "MINGWBIN=%%~dpI"
set "MINGWBIN=%MINGWBIN:~0,-1%"
echo [OK] make  : %MAKE%>> "%LOG%"
set "PATH=%QTBIN%;%MINGWBIN%;%PATH%"

REM ================= 3. build =================
if not exist build mkdir build
cd build
echo [..] qmake...>> "%LOG%"
"%QMAKE%" ..\campus_gui.pro >> "%LOG%" 2>&1 || (echo [X] qmake failed>> "%LOG%" & type "%LOG%" & pause & exit /b 1)
echo [..] compiling...>> "%LOG%"
"%MAKE%" -j4 >> "%LOG%" 2>&1 || (echo [X] compile failed>> "%LOG%" & type "%LOG%" & pause & exit /b 1)
cd ..

set "EXE="
if exist "build\release\CampusResourceManager.exe" set "EXE=build\release\CampusResourceManager.exe"
if exist "build\CampusResourceManager.exe" set "EXE=build\CampusResourceManager.exe"
if "%EXE%"=="" (echo [X] exe not produced>> "%LOG%" & type "%LOG%" & pause & exit /b 1)
echo [OK] built: %EXE%>> "%LOG%"

REM ================= 4. portable package =================
set "REL=release\CS3400_Campus_Resource_Manager"
if exist "%REL%" rmdir /s /q "%REL%"
mkdir "%REL%"
copy /y "%EXE%" "%REL%\CampusResourceManager.exe" >nul
if not exist "%REL%\data" mkdir "%REL%\data"
copy /y "..\data\*.txt" "%REL%\data\" >nul
if exist "README.txt" copy /y "README.txt" "%REL%\README.txt" >nul
echo [..] windeployqt...>> "%LOG%"
"%QTBIN%\windeployqt.exe" --release --no-translations --no-compiler-runtime "%REL%\CampusResourceManager.exe" >> "%LOG%" 2>&1
for %%F in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do if exist "%MINGWBIN%\%%F" copy /y "%MINGWBIN%\%%F" "%REL%\" >nul

echo [DONE] Portable folder: %CD%\%REL%>> "%LOG%"
echo ================================================================
echo  DONE. Portable folder: %CD%\%REL%
echo ================================================================
type "%LOG%"
pause
