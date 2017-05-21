@echo OFF
Setlocal enabledelayedexpansion
set find=.wav
for /r %%f in (*.wav) do start OpusTools/Binaries/opusenc.exe %%f %%f.opus
echo Making sure all the files get exported before renaming...
timeout /nobreak 10
for /r %%r in (*.opus) do (
	set file=%%~nxr
	ren %%r !file:%find%=!
)
echo msgbox "Your exported files should be saved in %~dp0." > %tmp%\tmp.vbs
wscript %tmp%\tmp.vbs
del %tmp%\tmp.vbs