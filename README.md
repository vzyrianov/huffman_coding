# huffman_coding

A text file compressor / decompressor that implements the huffman algorithm. 

## Handing binary

All binary strings are stored in structs containing a 64 bit integer that holds the value of the binary string and an integer that specifies the length of the binary string. 

Binary strings are written to files in 64 bit chunks. A buffer is used to insure this. 

## Compressed file format

The compressed version of the text is stored in a single file. 

The first part of the file is the huffman tree encoded in a flat format. The `\a` character specifies moving back up a node.

The rest of the file is the text compressed in binary. The end of file is marked by a null terminator character `\0` so that binary digits beyond it are not read.

## Limitations

Because the special characters `\a` and `\0` are used, only files which do not contain them can be compressed. Namely, text files. To support compression of other files, the code would have to be modified to escape the special characters in a similar way to how C++ does it. 
