set INPUT_DIR=../GpApp
set OUTPUT_DIR=obj
set FLAGS=-s USE_SDL=2 -flto -I../GpCommon/ -I../PortabilityLayer/ -I../Common/ -s ASYNCIFY -O3 -DGP_DEBUG_CONFIG=0

emcc -c %INPUT_DIR%/GpApp_Combined.cpp -o %OUTPUT_DIR%/GpApp_Combined.o %FLAGS%

pause
