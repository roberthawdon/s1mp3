@echo off

echo clean...
if exist giveio.h    del giveio.h   /F /Q >NUL
if exist giveio.bin  del giveio.bin /F /Q >NUL
if exist giveio.obj  del giveio.obj /F /Q >NUL
if exist giveio.ihx  del giveio.ihx /F /Q >NUL

echo assemble...
..\..\bin\as-z80 -lo giveio.obj giveio.asm
if not %ERRORLEVEL% == 0 goto error
if not exist giveio.obj goto error

echo link...
..\..\bin\link-z80 -n -- -i giveio.ihx giveio.obj
if not %ERRORLEVEL% == 0 goto error
if not exist giveio.ihx goto error

echo create binary...
..\..\bin\ihx2bin giveio.bin giveio.ihx
if not %ERRORLEVEL% == 0 goto error
if not exist giveio.bin goto error

echo create c header file...
..\..\bin\bin2c giveio.bin giveio >giveio.h
if not %ERRORLEVEL% == 0 goto error
if not exist giveio.h goto error

goto exit


:error
pause


:exit
