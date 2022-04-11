split.c is the source file for this assinemnt

make, make all or make split builds the file 

make clean : clears split.o and split
 
./split <delimiter> <file> <file> ....

above is the format to execute the code

split.c uses read() and write() functioins to read and write to files or stdin stdout.

A buffer (static) of 100 is used to read, then a loop goes through the buffer to check for delimeter,

if found a swap is made, swap(delimeter, '\n').

then the same is written to stdout


