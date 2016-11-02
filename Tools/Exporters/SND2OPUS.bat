@echo OFF

reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set OS=32BIT || set OS=64BIT

if %OS%==32BIT start FFmpeg/ffmpeg_x86.exe -i %~n1%~x1 %~n1.opus
if %OS%==64BIT start FFmpeg/ffmpeg_x64.exe -i %~n1%~x1 %~n1.opus
