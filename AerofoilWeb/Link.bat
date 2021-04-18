set INPUT_DIR=.
set OUTPUT_DIR=bin
set FLAGS=-flto -O3 -s USE_SDL=2 -s USE_ZLIB=1 -s ASYNCIFY -s ASYNCIFY_IGNORE_INDIRECT -s INITIAL_MEMORY=33554432 -s ASYNCIFY_ADVISE

emcc obj/AerofoilWeb_Combined.o obj/AerofoilWeb_Resources.o obj/GpShell_Combined.o obj/AerofoilSDL_Combined.o obj/GpApp_Combined.o obj/PortabilityLayer_Combined.o obj/MacRomanConversion.o -o %OUTPUT_DIR%/aerofoil.html %FLAGS%
