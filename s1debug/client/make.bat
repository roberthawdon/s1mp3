@echo off

echo clean...
del test.bin /F /Q >NUL
del test.obj /F /Q >NUL
del test.ihx /F /Q >NUL

echo assemble...
as-z80 -o test.obj test.asm
if not %ERRORLEVEL% == 0 goto error
if not exist test.obj goto error

echo link...
link-z80 -n -- -i test.ihx test.obj
if not %ERRORLEVEL% == 0 goto error
if not exist test.ihx goto error

echo create binary...
..\..\bin\ihx2bin test.bin test.ihx
if not %ERRORLEVEL% == 0 goto error
if not exist test.bin goto error

echo upload binary...
..\..\bin\s1giveio.exe < s1giveio.inp

goto exit


:error
pause


:exit

echo clean...
del test.obj /F /Q >NUL
del test.ihx /F /Q >NUL
