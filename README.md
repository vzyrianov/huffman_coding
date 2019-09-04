# huffman_coding

A file compressor / decompressor that implements the huffman algorithm. Works on any file type. On text it generally decreases file size by 50%. Other file type's compression ratios depend on how random the data in the file is.  

## Handing binary

All binary strings are stored in structs containing a 64 bit integer that holds the value of the binary string and an integer that specifies the length of the binary string. 

Binary strings are written to files in 64 bit chunks. A buffer is used to insure this. 

## Compressed file format

The compressed version of the text is stored in a single file. 

The first part of the file is the huffman tree encoded in a flat format. The `\a` character specifies moving back up a node. If a node's value is the `\a` character, it is escaped by putting a `\` in front of it. 

The rest of the file is the text compressed in binary. The end of file is marked by a null terminator character `\0` so that binary digits beyond it are not read. If the compressed text contains a `\0` character, it is escaped with a `\` character.  

