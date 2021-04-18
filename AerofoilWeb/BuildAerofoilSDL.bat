set INPUT_DIR=../AerofoilSDL
set OUTPUT_DIR=obj
set FLAGS=-s USE_SDL=2 -flto -I../GpCommon/ -I../PortabilityLayer/ -I../Common/ -s ASYNCIFY -O3 -DGP_DEBUG_CONFIG=0

emcc -c %INPUT_DIR%/AerofoilSDL_Combined.cpp -o %OUTPUT_DIR%/AerofoilSDL_Combined.o %FLAGS%

pause
