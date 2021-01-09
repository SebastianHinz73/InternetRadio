
@echo off
SET COMPORT=COM3


:loop
cls
echo.
echo Please select one of the following options:
echo. 
echo  1 - Change com port
echo  2 - Get board info on %COMPORT%
echo  3 - Flash Lyrat WebRadio firmware %COMPORT%
echo  4 - Delete NVS (intialize radio data) on %COMPORT%
echo  5 - Serial console on %COMPORT%
echo. 
echo  x - Exit
echo.
set /p SELECTED=Your choice: 

if "%SELECTED%" == "x" goto :eof
if "%SELECTED%" == "1" goto :SetComPort
if "%SELECTED%" == "2" goto :BoardInfo
if "%SELECTED%" == "3" goto :Lyrat
if "%SELECTED%" == "4" goto :Erase
if "%SELECTED%" == "5" goto :Serial
goto :errorInput 


:SetComPort
echo  1 - Set com port1
echo  2 - Set com port2
echo  3 - Set com port3
echo  4 - Set com port4
echo  5 - Set com port5
echo  6 - Set com port6
echo  7 - Set com port7
echo  8 - Set com port9
echo  9 - Set com port9
echo  10 - Set com port10
echo  x - Exit
echo.
set /p SELECTED=Your choice: 

if "%SELECTED%" == "x" goto :loop
if "%SELECTED%" == "1" (
set COMPORT=COM1
goto :loop
)
if "%SELECTED%" == "2" (
set COMPORT=COM2
goto :loop
)
if "%SELECTED%" == "3" (
set COMPORT=COM3
goto :loop
)
if "%SELECTED%" == "4" (
set COMPORT=COM4
goto :loop
)
if "%SELECTED%" == "5" (
set COMPORT=COM5
goto :loop
)
if "%SELECTED%" == "6" (
set COMPORT=COM6
goto :loop
)
if "%SELECTED%" == "7" (
set COMPORT=COM7
goto :loop
)
if "%SELECTED%" == "8" (
set COMPORT=COM8
goto :loop
)
if "%SELECTED%" == "9" (
set COMPORT=COM9
goto :loop
)
if "%SELECTED%" == "10" (
set COMPORT=COM10
goto :loop
)
goto :errorInput 

pause
goto :loop

:BoardInfo
@echo on
esptool.exe --chip esp32 --port %COMPORT% chip_id
@echo off
echo.
pause
goto :loop

:Lyrat
@echo on
esptool.exe -p %COMPORT% -b 460800 --before default_reset --after hard_reset --chip esp32 write_flash --flash_mode dio --flash_freq 40m --flash_size 2MB 0x8000 Lyrat/partition-table.bin 0x1000 Lyrat/bootloader.bin 0x10000 Lyrat/LyratRadio.bin
@echo off
echo.
pause
goto :loop

:Erase
@echo on
esptool.exe --chip esp32 --port %COMPORT% --baud 115200 --before default_reset --after hard_reset erase_region 0x9000 0x6000
@echo off
echo.
pause
goto :loop

:Serial
@echo on
Putty\putty.exe -serial %COMPORT% -sercfg 115200,8,n,1,N
@echo off
echo.
pause
goto :loop

:errorInput
echo.
echo Illegal input! Please try again!
echo.
pause
goto :loop

