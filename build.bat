@echo off
setlocal
cd /D "%~dp0"

rem ######################## COMPILER AND LINKER FLAGS #############################################

set opt_define= -D_CRT_SECURE_NO_WARNINGS
set opt_common= -Zi -W4 -nologo

set opt_warning= -Wdouble-promotion -Wconversion
set opt_warning= -Wno-unused-value %opt_warning%
set opt_warning= -Wno-unused-function %opt_warning%
set opt_warning= -Wno-unused-variable %opt_warning%
set opt_warning= -Wno-unused-parameter %opt_warning%
set opt_warning= -Wno-unused-but-set-variable %opt_warning%

set opt_all= %opt_define% %opt_common% %opt_warning%

set opt_debug=     -Od -MTd -RTCcsu
set opt_release=   -O2 -GS-
set links_debug=   -opt:ref
set links_release= -opt:ref -subsystem:windows -fixed
set libs_debug=    user32.lib gdi32.lib winmm.lib
set libs_release=  user32.lib gdi32.lib winmm.lib kernel32.lib libvcruntime.lib ucrt.lib

rem ######################## PARSING SCRIPT ARGUMENTS ##############################################

for %%a in (%*) do set "%%a=1"

if not "%release%"=="1" set debug=1

if "%debug%"=="1" (
	set opt=     -DDISABLE_CRT=0 %opt_all% %opt_debug%
	set linkopt= -link %links_debug% %libs_debug%
	echo [debug]
) else if "%release%"=="1" (
	set opt=     -DDISABLE_CRT=1 %opt_all% %opt_release%
	set linkopt= -link %links_release% %libs_release%
	echo [release]
)

rem ######################## COMPILATION ###########################################################

if not exist build mkdir build
pushd build

del *.pdb
call clang-cl %opt% ..\src\windows.c %linkopt% -out:app.exe

popd
