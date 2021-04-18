set INPUT_DIR=../MacRomanConversion
set OUTPUT_DIR=obj
set FLAGS=-flto -I../MacRomanConversion/ -s ASYNCIFY -O3

emcc -c %INPUT_DIR%/MacRomanConversion.cpp -o %OUTPUT_DIR%/MacRomanConversion.o %FLAGS%

pause
