@echo off
cl /Zi main.cpp /link /SUBSYSTEM:console || exit /b
del main.obj || exit /b
main 8 input.txt
