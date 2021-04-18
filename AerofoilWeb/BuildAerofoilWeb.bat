set INPUT_DIR=.
set OUTPUT_DIR=obj
set FLAGS=-s USE_SDL=2 -flto -I../GpCommon/ -I../PortabilityLayer/ -I../Common/ -I../GpShell/ -s ASYNCIFY -O3 -DGP_DEBUG_CONFIG=0

emcc -c %INPUT_DIR%/AerofoilWeb_Combined.cpp -o %OUTPUT_DIR%/AerofoilWeb_Combined.o %FLAGS%

pause
