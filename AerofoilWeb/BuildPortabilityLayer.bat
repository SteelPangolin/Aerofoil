set INPUT_DIR=../PortabilityLayer
set OUTPUT_DIR=obj
set FLAGS=-s USE_ZLIB=1 -flto -I../GpCommon/ -I../Common/ -I../PortabilityLayer/ -I../rapidjson/include/ -I../MacRomanConversion/ -I../stb/ -s ASYNCIFY -O3 -DGP_DEBUG_CONFIG=0 -Wno-tautological-constant-out-of-range-compare

emcc -c %INPUT_DIR%/PortabilityLayer_Combined.cpp -o %OUTPUT_DIR%/PortabilityLayer_Combined.o %FLAGS%

pause
