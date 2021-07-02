@echo off

set application_name=app
set build_options= -DBUILD_WIN32=1
set compile_flags= /nologo /Zi /FC /I ..\source\
set common_link_flags= opengl32.lib /incremental:no /debug
set platform_link_flags= gdi32.lib user32.lib winmm.lib dxguid.lib Dinput8.lib %common_link_flags%

if not exist build mkdir build
pushd build
start /b /wait "" "cl.exe"  %build_options% %compile_flags% ../source/win32/win32_main.c /link %platform_link_flags% /out:platform_windows.exe
start /b /wait "" "cl.exe"  %build_options% %compile_flags% ../source/app.c /LD /link %common_link_flags% /out:%application_name%.dll
popd