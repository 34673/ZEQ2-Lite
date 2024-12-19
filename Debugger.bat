@echo off
cd %~dp0Tools/MinGW-w64/bin
cd %~dp0Tools/w64devkit-1.18.0/bin
gdb %~dp0/../Build/ZEQ2-Lite.x86.exe