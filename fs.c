// ============================================================================
// fs.c - user FileSytem API
// ============================================================================

#include "bfs.h"
#include "fs.h"

// ============================================================================
// Close the file currently open on file descriptor 'fd'.
// ============================================================================
i32 fsClose(i32 fd) { 
  i32 inum = bfsFdToInum(fd);
  bfsDerefOFT(inum);
  return 0; 
}



// ============================================================================
// Create the file called 'fname'.  Overwrite, if it already exsists.
// On success, return its file descriptor.  On failure, EFNF
// ============================================================================
i32 fsCreate(str fname) {
  i32 inum = bfsCreateFile(fname);
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Format the BFS disk by initializing the SuperBlock, Inodes, Directory and 
// Freelist.  On succes, return 0.  On failure, abort
// ============================================================================
i32 fsFormat() {
  FILE* fp = fopen(BFSDISK, "w+b");
  if (fp == NULL) FATAL(EDISKCREATE);

  i32 ret = bfsInitSuper(fp);               // initialize Super block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitInodes(fp);                  // initialize Inodes block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitDir(fp);                     // initialize Dir block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitFreeList();                  // initialize Freelist
  if (ret != 0) { fclose(fp); FATAL(ret); }

  fclose(fp);
  return 0;
}


// ============================================================================
// Mount the BFS disk.  It must already exist
// ============================================================================
i32 fsMount() {
  FILE* fp = fopen(BFSDISK, "rb");
  if (fp == NULL) FATAL(ENODISK);           // BFSDISK not found
  fclose(fp);
  return 0;
}



// ============================================================================
// Open the existing file called 'fname'.  On success, return its file 
// descriptor.  On failure, return EFNF
// ============================================================================
i32 fsOpen(str fname) {
  i32 inum = bfsLookupFile(fname);        // lookup 'fname' in Directory
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Read 'numb' bytes of data from the cursor in the file currently fsOpen'd on
// File Descriptor 'fd' into 'buf'.  On success, return actual number of bytes
// read (may be less than 'numb' if we hit EOF).  On failure, abort
// ============================================================================
i32 fsRead(i32 fd, i32 numb, void* buf) {

    // It stores the whole data, and "buf" will copy it at the end.
    i8 wholeTemp[2000];    

    // Get the inum
    i32 inum = bfsFdToInum(fd);

    // By using a curser, get the first fbn to read
    i32 fbn = fsTell(fd) / 512;
    
    // If it requires only one block, just 
    if(numb <= 512)
    {
        // Read the one block
        bfsRead(inum, fbn, wholeTemp);

        // Copy the buffer in certain number of bytes (numb)
        memcpy(buf, wholeTemp, numb);
    }
    // If it requires more than on blocks to read
    else
    {
        // It is a buffer whcih holds exactly 512 bytes from one DBN
        i8 blockTemp[512];

        // Make sure how many blocks are needed to be read
        int fbnNum = numb / 512;
        
        // It tracks the index of "wholeTemp"
        int wholeIndex = 0;

        // It tracks the index of "blockTemp"
        int blockIndex = 0;

        // It is the stopper when the curser reached to 512th index
        int maxIndex;

        // It loops the number of FBN to read each 512 input.
        for(int i=0; i<fbnNum; i++)
        {
            // Reads the 512 bytes and store in "blockTemp"
            bfsRead(inum, fbn + i, blockTemp);

            // Reset the max of maxIndex. It simply adds 512 every time
            maxIndex = wholeIndex + 512;

            // Reset the index of "blockTemp" to 0 every time
            blockIndex = 0;

            while(wholeIndex < maxIndex)
            {
                // Copy the data
                wholeTemp[wholeIndex] = blockTemp[blockIndex];
                // Increment two indexes
                wholeIndex++;
                blockIndex++;
            }
        }  

        // If there is a FBN that needs to read partial input
        if(numb % 512 != 0)
        {
          // This is the last fbn to read.       
          bfsRead(inum, fbn + fbnNum, blockTemp);  
                          
          // Reset the index of "blockTemp" to 0
          blockIndex = 0;

          // Below while loop reads the remainder of the data in the "blocktemp"
          while(wholeIndex < numb)
          {
              // Copy the data
              wholeTemp[wholeIndex] = blockTemp[blockIndex];
              // Increment two indexes
              wholeIndex++;
              blockIndex++;
          }        
        }

        // At the last, it copies the whole data in "wholeTemp" to "buf"
        memcpy(buf, wholeTemp, numb);

        // Count the number of the indexes that allocated but not written
        int notWritten = 0;
        // Read the very last FBN
        bfsRead(inum, fbn + fbnNum - 1, blockTemp);  
        // Reset to 0
        blockIndex = 0;
        // Count the number of empty indexes
        while(blockIndex < 512)
        {
          if(blockTemp[blockIndex] == 0)
          {
            notWritten++;
          }                    
          blockIndex++;
        }
        // Deduct the amount of empty indexes.
        numb-=notWritten;
    }
    // Set the curser to right position
    fsSeek(fd, numb, SEEK_CUR);
    return numb;
}


// ============================================================================
// Move the cursor for the file currently open on File Descriptor 'fd' to the
// byte-offset 'offset'.  'whence' can be any of:
//
//  SEEK_SET : set cursor to 'offset'
//  SEEK_CUR : add 'offset' to the current cursor
//  SEEK_END : add 'offset' to the size of the file
//
// On success, return 0.  On failure, abort
// ============================================================================
i32 fsSeek(i32 fd, i32 offset, i32 whence) {

  if (offset < 0) FATAL(EBADCURS);
 
  i32 inum = bfsFdToInum(fd);
  i32 ofte = bfsFindOFTE(inum);
  
  switch(whence) {
    case SEEK_SET:
      g_oft[ofte].curs = offset;
      break;
    case SEEK_CUR:
      g_oft[ofte].curs += offset;
      break;
    case SEEK_END: {
        i32 end = fsSize(fd);
        g_oft[ofte].curs = end + offset;
        break;
      }
    default:
        FATAL(EBADWHENCE);
  }
  return 0;
}



// ============================================================================
// Return the cursor position for the file open on File Descriptor 'fd'
// ============================================================================
i32 fsTell(i32 fd) {
  return bfsTell(fd);
}



// ============================================================================
// Retrieve the current file size in bytes.  This depends on the highest offset
// written to the file, or the highest offset set with the fsSeek function.  On
// success, return the file size.  On failure, abort
// ============================================================================
i32 fsSize(i32 fd) {
  i32 inum = bfsFdToInum(fd);
  return bfsGetSize(inum);
}



// ============================================================================
// Write 'numb' bytes of data from 'buf' into the file currently fsOpen'd on
// filedescriptor 'fd'.  The write starts at the current file offset for the
// destination file.  On success, return 0.  On failure, abort
// ============================================================================
i32 fsWrite(i32 fd, i32 numb, void* buf)
{
    // Copy the "buf" input
    i8 copyBuf[2000];
    memcpy(copyBuf, buf, 2000);    

    // Get the inum
    i32 inum = bfsFdToInum(fd);

    // By using a curser, get the first fbn to read
    i32 fbn = fsTell(fd) / 512;

    // Get DBN
    i32 dbn = bfsFbnToDbn(inum, fbn);

    // Store the data in the certain DNB
    i8 temp[512] = {0};

    if(dbn < 0)
    {
      // If the DBN is free, allocate block
      bfsAllocBlock(inum, fbn);
      dbn = bfsFbnToDbn(inum, fbn);
      memset(temp, 0, 512);
    }
    else
    {
      // If the blokc is already allocated, read the block
      bfsRead(inum, fbn, temp);
    }
    
    // Set index to the starting curser at the DBN
    int tempIndex = fsTell(fd) % 512;

    // Set index to track the copied buf
    int copyIndex = 0;

    // It is the index that cureser needs to stop
    int endIndex;

    // If it requires only one block, just 
    if(numb <= 512)
    {        
        endIndex = tempIndex + numb;               
        while(tempIndex < endIndex)
        {
            temp[tempIndex] = copyBuf[copyIndex];
            tempIndex++;
            copyIndex++;
        } 

        // write on the disk
        bioWrite(dbn, &temp);
    }
    else
    {      
      // Fill up the rest of the inputs in the first fbn
      while(tempIndex < 512)
      {
        temp[tempIndex] = copyBuf[copyIndex];
        tempIndex++;
        copyIndex++;
      }
      bioWrite(dbn, temp);
      
      // Track how many number of blocks need to be stored
      int numbLeft = numb - copyIndex;      
      
      
      // Loop until the last FBN to write
      while(numbLeft > 512)
      {
        tempIndex = 0;
        while(tempIndex < 512)
        {
          temp[tempIndex] = copyBuf[copyIndex];
          tempIndex++;
          copyIndex++;
        }
        numbLeft-=512;
        fbn++;
        dbn = bfsFbnToDbn(inum, fbn);
        bioWrite(dbn, temp);
      }      
      

      // Last FBN to write the input on disk
      dbn = bfsFbnToDbn(inum, fbn+1);

      
      if(dbn < 0)
      {
        // If the DBN is free, allocate block
        bfsAllocBlock(inum, fbn+1);
        dbn = bfsFbnToDbn(inum, fbn+1);
        memset(temp, 0, 512);
      }
      else
      {
        // Read the block
        bfsRead(inum, fbn+1, temp);
      }

      tempIndex = 0;

      // Write the last FBN
      while(numbLeft > 0)
      {
        temp[tempIndex] = copyBuf[copyIndex];
        copyIndex++;
        tempIndex++;
        numbLeft--;
      }      
      bioWrite(dbn, temp);
    }    

    // Move a curser to the right place
    fsSeek(fd, numb, SEEK_CUR);    
    return 0;
    // It is used to check the input in the buffer (for implementation tool)
    //debDumpDbn(dbn, 1);
    
   
    FATAL(ENYI);                                  // Not Yet Implemented!
    return 0;
}
