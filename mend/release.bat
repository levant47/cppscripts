@echo off
cl /Os /GS- main.cpp /link /SUBSYSTEM:console /NODEFAULTLIB /out:mend.exe /ENTRY:main kernel32.lib || exit /b
del main.obj || exit /b
