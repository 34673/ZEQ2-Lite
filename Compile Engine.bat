@echo off
set oldSystemPath=%PATH%
set compilerPath=%~dp0Tools/MinGW-w64/bin
set PATH=%compilerPath%;%PATH%
%compilerPath%/make install -j 12 COMPILE_PLATFORM=mingw32 BUILD_GAME_SO=1 BUILD_GAME_QVM=0
pause
set PATH=%oldSystemPath%