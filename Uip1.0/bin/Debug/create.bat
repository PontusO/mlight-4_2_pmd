@echo off
rem
rem Split and create new hex files
rem
@echo Converting files.....
srec_cat bin\debug\uip1.hex -intel -crop 0x0000 0xffff -o bin\debug\uip1_b01.hex -intel > build.log
srec_cat bin\debug\uip1.hex -intel -crop 0x28000 0x2ffff -o bin\debug\uip1_b2.hex -intel >> build.log
srec_cat bin\debug\uip1.hex -intel -crop 0x38000 0x3ffff -o bin\debug\uip1_b3.hex -intel >> build.log

rem
rem Flash the device
rem
@echo *** Flashing device ***
@echo Erasing device...
FlashUtilCL FLASHEraseUSB "" 0

@echo Flashing Common area (0x0000 - 0xFFFF)
FlashUtilCL DownloadUSB bin\debug\uip1_b01.hex "" 0 0  >> build.log

@echo Flashing Bank 2 (0x28000 - 0x2FFFF)
FlashUtilCL DownloadUSB -B2 bin\debug\uip1_b2.hex "" 0 0  >> build.log

@echo Flashing Bank 3 (0x38000 - 0x3FFFF)
FlashUtilCL DownloadUSB -B3R bin\debug\uip1_b3.hex "" 0 0  >> build.log

@echo All banks are flashed, No Errors !
