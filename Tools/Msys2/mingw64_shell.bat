:
@echo off

rem ember value of GOTO: is used to know recursion has happened.
if "%1" == "GOTO:" goto %2

if NOT "x%WD%" == "x" set WD=

rem ember command.com only uses the first eight characters of the label.
goto _WindowsNT

start /min %COMSPEC% /e:4096 /c %0 GOTO: _Resume %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
goto EOF

rem ember that we execute here if we recursed.
:_Resume
for %%F in (1 2 3) do shift
if NOT EXIST %WD%msys-2.0.dll set WD=.\usr\bin\

rem ember that we get here even in command.com.
:_WindowsNT

if NOT EXIST %WD%msys-2.0.dll set WD=%~dp0usr\bin\

set MSYSTEM=MINGW64
rem To activate windows native symlinks uncomment next line
rem set MSYS=winsymlinks:nativestrict
rem Set debugging program for errors
rem set MSYS=error_start:%WD%../../mingw64/bin/qtcreator.exe^|-debug^|^<process-id^>
set MSYSCON=mintty.exe
if "x%1" == "x-consolez" set MSYSCON=console.exe
if "x%1" == "x-mintty" set MSYSCON=mintty.exe

if "x%MSYSCON%" == "xmintty.exe" goto startmintty
if "x%MSYSCON%" == "xconsole.exe" goto startconsolez

:startmintty
if NOT EXIST %WD%mintty.exe goto startsh
start %WD%mintty -i /msys2.ico /usr/bin/bash --login %*
exit

:startconsolez
cd %WD%..\lib\ConsoleZ
start console -t "MinGW" -r "%*"
exit

:copyprofile
copy %~dp0\..\Msys2\home\ZEQ2-Lite\.profile_default %~dp0\..\Msys2\home\ZEQ2-Lite\.profile
echo Created local .profile successfully.
goto startsh

:startsh
if NOT EXIST %WD%sh.exe goto notfound
set PATH=%~dp0\..\bin;%PATH%
if NOT EXIST %~dp0\..\Msys2\home\ZEQ2-Lite\.profile goto copyprofile
start %WD%sh --login -i %*
exit

:EOF
