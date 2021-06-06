# final_os_project

This document describes how to implement a Unix-like fileSystem for a project in an Operating Class. I call this filesystem “BFS”.

The filesystem comprises 3 layers.  From top to bottom, these are:
•	fs – user-level filesystem, with functions like fsOpen, fsRead, fsSeek, fsClose.
•	bfs – functions internal to BFS such as bfsFindFreeBlock, bfsInitFreeList.
•	bio - lowest level block IO functions: bioRead and bioWrite.
It is easiest to describe these 3 layers in the order, bio, fs and bfs, as follows:



bio (Block Input and Output): 
A raw, unformatted BFS disk consists of a sequence of 100 blocks, each 512 bytes in size.  
The BFS disk is implemented on the host filesystem as a file called BFSDISK.  
Blocks are numbered from 0 up to 99.  Block 0 starts at byte offset 0 in BFSDISK.  
The next block starts at byte offset 512, and so on.  We can read an entire block from disk into memory.  
And we can write an entire block from memory onto a disk block.  
It’s perfectly legal to update an existing disk block by over-writing its contents.  

fs (user-level File System):
An interface of raw, fixed-size blocks is too primitive for regular use.  
BFS presents a more user-friendly interface: one that lets us create files, 
where “file” is a collection of zero or more bytes on disk, reached by a user-supplied name.  
The bytes that comprise the file may be scattered over the disk’s blocks, 
rather than having to occupy a contiguous sequence of block numbers.  
Files can be grown automatically as a user writes more and more data into that file

bfs (Block File System): 
BFS uses certain blocks – which we will call metablocks – at the start of the BFS disk, 
to remember where all its files are stored, and to keep track of all free blocks.  
These metablocks are not visible to the programmer via the fs interface.



Purposer for this project:
Pass all the 6 tests in the test5.c by implementing fsWrite() and fsRead() in fs.c. 
All the files except those two functions are all provided in prior. 
