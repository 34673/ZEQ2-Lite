@echo OFF

reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set OS=32BIT || set OS=64BIT

if %OS%==32BIT start FFmpeg/FFmpeg32 -i %~n1%~x1 -ab 256k -ar 22050 -r 30 -s 512x512 intro.roq
if %OS%==64BIT start FFmpeg/FFmpeg64 -i %~n1%~x1 -ab 256k -ar 22050 -r 30 -s 512x512 intro.roq