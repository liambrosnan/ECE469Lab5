#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "queue.h"
#include "disk.h"
#include "dfs.h"
#include "synch.h"

#include "dfs_shared.h"

static dfs_inode inodes[DFS_INODE_NMAX_NUM]; // all inodes
static dfs_superblock sb; // superblock
static uint32 fbv[DFS_MAX_NUM_WORDS]; // Free block vector


static uint32 negativeone = 0xFFFFFFFF;
static inline uint32 invert(uint32 n) { return n ^ negativeone; }


int DFSisOpen = 0;


lock_t fbvLock;
lock_t inodesLock;


// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

// STUDENT: put your file system level functions below.
// Some skeletons are provided. You can implement additional functions.

///////////////////////////////////////////////////////////////////
// Non-inode functions first
///////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsModuleInit is called at boot time to initialize things and
// open the file system for use.
//-----------------------------------------------------------------

void DfsModuleInit() {
// You essentially set the file system as invalid and then open 
// using DfsOpenFileSystem().

    // Create Locks for Helper funcs - 
    fbvLock = LockCreate();
    inodesLock = LockCreate();

    DfsInvalidate();
    DfsOpenFileSystem();
}

//-----------------------------------------------------------------
// DfsInavlidate marks the current version of the filesystem in
// memory as invalid.  This is really only useful when formatting
// the disk, to prevent the current memory version from overwriting
// what you already have on the disk when the OS exits.
//-----------------------------------------------------------------

void DfsInvalidate() {
// This is just a one-line function which sets the valid bit of the 
// superblock to 0.
    sb.valid = 0;
}

//-------------------------------------------------------------------
// DfsOpenFileSystem loads the file system metadata from the disk
// into memory.  Returns DFS_SUCCESS on success, and DFS_FAIL on 
// failure.
//-------------------------------------------------------------------

int DfsOpenFileSystem() {
//Basic steps:
// Check that filesystem is not already open

    if(DFSisOpen){
        return DFS_FAIL;
        DFSisOpen = 1;      // flag that its open
    }

// Read superblock from disk.  Note this is using the disk read rather 
// than the DFS read function because the DFS read requires a valid 
// filesystem in memory already, and the filesystem cannot be valid 
// until we read the superblock. Also, we don't know the block size 
// until we read the superblock, either.
    
    disk_block buffer;

    if(DfsReadBlock(1,&buffer) != DISK_BLOCKSIZE){
        Printf("Error in DfsReadBlock\n");
        return DFS_FAIL;
    }

// Copy the data from the block we just read into the superblock in memory
    
    bcopy(buffer.data,(char *)(&sb),sizeof(dfs_superblock));

// All other blocks are sized by virtual block size:
// Read inodes
// Read free block vector
// Change superblock to be invalid, write back to disk, then change 
// it back to be valid in memory
    char * hold;
    int i;
    hold = (char *) inodes;
    int bytesRead;
    
    for(i = dfsBNUM(sb.inodeBlockStart); i < dfsBNUM(sb.fbvBlockStart); i++){
        // bytesRead = DfsReadBlock(1,&buffer);
        bytesRead = DiskReadBlock(1,&buffer);      // DISK, not DFS ReadBlock
        if(bytesRead != DISK_BLOCKSIZE){
            Printf("Error in DfsReadBlock\n");
            return DFS_FAIL;
        }

        bcopy(buffer.data,hold,DISK_BLOCKSIZE);
        hold += DISK_BLOCKSIZE;
    }

    hold = (char *) fbv;

    for(i = dfsBNUM(sb.fbvBlockStart); i < dfsBNUM(sb.dataBlockStart); i++){
        // Printf("here");
        // bytesRead = DfsReadBlock(1,&buffer);
        bytesRead = DiskReadBlock(1,&buffer);       
        if(bytesRead != DISK_BLOCKSIZE){
            Printf("Error in DfsReadBlock\n");
            return DFS_FAIL;
        }

        bcopy(buffer.data,hold,DISK_BLOCKSIZE);
        hold += DISK_BLOCKSIZE;
    }

    DfsInvalidate();
    bzero(buffer.data,DISK_BLOCKSIZE);
    bcopy((char *)(&sb),buffer.data,sizeof(dfs_superblock));

    int bytesWritten;
    bytesWritten = DiskWriteBlock(1,&buffer);

    if(bytesWritten != DISK_BLOCKSIZE){
        Printf("Error in DfsReadBlock\n");
        return DFS_FAIL;
    }



    Printf("DFSOpenFileSystem worked"\n);
    sb.valid = 1;
    return DFS_SUCCESS;
}


//-------------------------------------------------------------------
// DfsCloseFileSystem writes the current memory version of the
// filesystem metadata to the disk, and invalidates the memory's 
// version.
//-------------------------------------------------------------------

int DfsCloseFileSystem() {
    int i;
    char * inodesPtr;
    char * fbvPtr;
    disk_block buffer;
    uint32 inodeStart;
    uint32 fbvStart;
    uint32 dataStart;

    if(DFSisOpen == 0){
        Printf("Already Closed\n");
        return DFS_SUCCESS;
    }

    DFSisOpen = 0;  // close

    inodesPtr = (char *) inodes;
    inodeStart = dfsBNUM(sb.inodeBlockStart);
    fbvStart = dfsBNUM(sb.fbvBlockStart);

    for(i = inodeStart; i < fbvStart; i++){
        // Printf("here\n");
        bcopy(inodesPtr,buffer.data,DISK_BLOCKSIZE);
        if(DiskWriteBlock(i,&buffer) != DISK_BLOCKSIZE){
            Printf("Write to Inodes failed\n");
            return DFS_FAIL;
        }

        inodesPtr += DISK_BLOCKSIZE;
    }

    fbvPtr = (char *)fbv;
    dataStart = dfsBNUM(sb.dataBlockStart);

    for(i = fbvStart; i < dataStart; i++){
        // Printf("here\n");
        bcopy(fbvPtr,buffer.data,DISK_BLOCKSIZE);
        if(DiskWriteBlock(i,&buffer) != DISK_BLOCKSIZE){
            // Printf("here\n");
            Printf("Write to FBV failed\n");
            return DFS_FAIL;
        }
        fbvPtr += DISK_BLOCKSIZE;
    }
    sb.valid = 1;

    // DfsInvalidate();
    bzero(buffer.data,DISK_BLOCKSIZE);
    bcopy((char *)(&sb),buffer.data,sizeof(dfs_superblock));
    
    int bytesWritten;
    bytesWritten = DiskWriteBlock(1,&buffer);

    if(bytesWritten != DISK_BLOCKSIZE){
        Printf("Error in DfsReadBlock\n");
        return DFS_FAIL;
    }

    Printf("DFS Closed successfully\n");
    return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsAllocateBlock allocates a DFS block for use. Remember to use 
// locks where necessary.
//-----------------------------------------------------------------

uint32 DfsAllocateBlock() {
// Check that file system has been validly loaded into memory
// Find the first free block using the free block vector (FBV), mark it in use
// Return handle to block
    int i;
    int j;
    int packet;
    int position;
    int returnHandle;

    if(sb.valid != 1){
        Printf("Filesystem not validly loaded (DfsAllocateBlock)\n");
        return DFS_FAIL;
    }

    while(LockHandleAcquire(fbvLock) != SYNC_SUCCESS);

    // Find Packet
    int foundPacket = 0;
    for(i = 0;(i <= DFS_MAX_NUM_WORDS) || (foundPacket == 0);i++){
        packet = i;
        for(j = 0; j <= 31; j++){
            if((fbv[packet] << j) >> (31 - j) == 0){
                position = j;
                foundPacket = 1;
            }
        }
    }

    fbv[packet] = fbv[packet] | (1 << (31 - position));

    while(LockHandleRelease(fbvLock) != SYNC_SUCCESS);

    returnHandle = position + (32 * packet);
}


//-----------------------------------------------------------------
// DfsFreeBlock deallocates a DFS block.
//-----------------------------------------------------------------

int DfsFreeBlock(uint32 blocknum) {    
    if(DFSisOpen != 1){
        Printf("Filesystem not open (FreeBlock)\n");
        return DFS_FAIL;
    }

    while(LockHandleAcquire(fbvLock) != SYNC_SUCCESS); 

    int packet = blocknum >> 5;         // adjust for sizing
    int position = blocknum & 0x1F;     // bitwise calc

    fbv[packet] &= invert(1<<(31 - position));
    
    while(LockHandleRelease(fbvLock) != SYNC_SUCCESS);  
    
    Printf("Block Successfully Freed\n");
    return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsReadBlock reads an allocated DFS block from the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to read from it.  Returns DFS_FAIL
// on failure, and the number of bytes read on success.  
//-----------------------------------------------------------------

int DfsReadBlock(uint32 blocknum, dfs_block *b) {
    int i;
    uint32 physicalBN;
    uint32 ratioBN;
    disk_block buffer;
    char * dataPtr;
    int bytesRead;

    // check if allocated
    int packet = blocknum >> 5;         // adjust for sizing
    int position = blocknum & 0x1F;     // bitwise calc
    if((fbv[packet] & (0x80000000 >> position)) == 0){
        // Printf("not allocated\n");
        return DFS_FAIL;
    }

    physicalBN = dfsBNUM(blocknum);
    ratioBN = dfsRatio();
    dataPtr = b->data;
    bytesRead = 0;

    for(i = physicalBN; i < (physicalBN + ratioBN); i++){
        DiskReadBlock(i,&buffer);
        bcopy(buffer.data,dataPtr,DISK_BLOCKSIZE);

        bytesRead += DISK_BLOCKSIZE;
        dataPtr += DISK_BLOCKSIZE;
    }

    return bytesRead;

}


//-----------------------------------------------------------------
// DfsWriteBlock writes to an allocated DFS block on the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to write to it.  Returns DFS_FAIL
// on failure, and the number of bytes written on success.  
//-----------------------------------------------------------------

int DfsWriteBlock(uint32 blocknum, dfs_block *b){
    int i;
    uint32 physicalBN;
    uint32 ratioBN;
    disk_block buffer;
    char * dataPtr;
    int bytesWritten;

    // check if allocated
    int packet = blocknum >> 5;         // adjust for sizing
    int position = blocknum & 0x1F;     // bitwise calc
    if((fbv[packet] & (0x80000000 >> position)) == 0){
        // Printf("not allocated\n");
        return DFS_FAIL;
    }

    physicalBN = dfsBNUM(blocknum);
    ratioBN = dfsRatio();
    dataPtr = b->data;
    bytesWritten = 0;

    for(i = physicalBN; i < (physicalBN + ratioBN); i++){
        DiskWriteBlock(i,&buffer);
        bcopy(buffer.data,dataPtr,DISK_BLOCKSIZE);

        bytesWritten += DISK_BLOCKSIZE;
        dataPtr += DISK_BLOCKSIZE;
    }

    return bytesWritten;

}

////////////////////////////////////////////////////////////////////////////////
// Inode-based functions
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsInodeFilenameExists looks through all the inuse inodes for 
// the given filename. If the filename is found, return the handle 
// of the inode. If it is not found, return DFS_FAIL.
//-----------------------------------------------------------------

uint32 DfsInodeFilenameExists(char *filename) {
    int i;

    if(DFSisOpen != 1){
        Printf("Filesystem not open (FreeBlock)\n");
        return DFS_FAIL;
    }

    for(i = 0; i < DFS_INODE_NMAX_NUM; i++){
        if(inodes[i].in_use == 1){
            if(dstrncmp(filename,inodes[i].filename,dstrlen(filename)) == 0){
                return i; // inode in use
            }
        }
    }

    // no inodes found in use
    Printf("No Inodes Found in use\n");
    return DFS_FAIL;
}


//-----------------------------------------------------------------
// DfsInodeOpen: search the list of all inuse inodes for the 
// specified filename. If the filename exists, return the handle 
// of the inode. If it does not, allocate a new inode for this 
// filename and return its handle. Return DFS_FAIL on failure. 
// Remember to use locks whenever you allocate a new inode.
//-----------------------------------------------------------------

uint32 DfsInodeOpen(char *filename) {
    uint32 handle;
    int i;


    handle = DfsInodeFilenameExists(filename);
    if(handle != DFS_FAIL){
        return handle;
    }
    else{
        // find inode for filename
        int flag = 0;
        for(i = 0; (i < DFS_INODE_NMAX_NUM) && (flag = 0);i++){
            //Printf("here\n");
            if(inodes[i].in_use != 1){
                handle = i;
                flag = 1;
            }
        }

        while(LockHandleAcquire(inodesLock) != SYNC_SUCCESS);

        inodes[handle].file_size = 0;
        inodes[handle].in_use = 1;

        dstrncpy(inodes[handle].filename,filename,dstrlen(filename));

        while(LockHandleRelease(inodesLock) != SYNC_SUCCESS);

        Printf("Inode is Open\n");
        return handle;
    }
    
}
//-----------------------------------------------------------------
// DfsInodeDelete de-allocates any data blocks used by this inode, 
// including the indirect addressing block if necessary, then mark 
// the inode as no longer in use. Use locks when modifying the 
// "inuse" flag in an inode.Return DFS_FAIL on failure, and 
// DFS_SUCCESS on success.
//-----------------------------------------------------------------

int DfsInodeDelete(uint32 handle) {
    int i;

    while(LockHandleAcquire(inodesLock) != SYNC_SUCCESS);

    inodes[handle].file_size = 0;
    inodes[handle].in_use = 0;
    // inodes[handle].filename = '\0';

    for(i = 0; i < DFS_INODE_BLOCKTABLE_SIZE; i++){
        inodes[handle].blockTable[i] = -1;
    }

    if(inodes[handle].BTindex != -1){
        DfsFreeBlock(inodes[handle].BTindex);
        inodes[handle].BTindex = -1;
    }

    if(inodes[handle].BTindex2 != -1){
        inodes[handle].BTindex2 = -1;
    }

    while(LockHandleRelease(inodesLock) != SYNC_SUCCESS);

    Printf("Inode Deleted\n");
    return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsInodeReadBytes reads num_bytes from the file represented by 
// the inode handle, starting at virtual byte start_byte, copying 
// the data to the address pointed to by mem. Return DFS_FAIL on 
// failure, and the number of bytes read on success.
//-----------------------------------------------------------------

int DfsInodeReadBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {
    dfs_block buffer;
    int * bufPtr;
    int * memPtr;
    int bytesRead;
    int BN;
    int dfsBN;
    int readFromFunc;
    int position;
    int posOffset, bytesOffset;

    
    if(inodes[handle].in_use != 1){
        Printf("Filename does not exist\n");
        return DFS_FAIL;
    }

    memPtr = mem;
    bytesRead = 0;
    BN = start_byte / sb.blocksize;
    readFromFunc = DfsReadBlock(inodes[handle].BTindex,&buffer);
    position = start_byte % sb.blocksize;
    

    while(bytesRead < num_bytes){
        if(BN < DFS_INODE_BLOCKTABLE_SIZE){
            dfsBN = inodes[handle].blockTable[BN];
        }
        if(inodes[handle].BTindex = -1){
            inodes[handle].BTindex = DfsAllocateBlock();
        }        
        if(readFromFunc != sb.blocksize){
            return DFS_FAIL;
        }
        else{
            bufPtr = (int *) buffer.data;
            dfsBN = bufPtr[BN - DFS_INODE_BLOCKTABLE_SIZE];
        }
        if(DfsReadBlock(dfsBN,&buffer) != sb.blocksize){
            Printf("DFS Read failed (DfsInodeReadBytes)\n");
            return DFS_FAIL;
        }

        posOffset = sb.blocksize - position;
        bytesOffset = num_bytes - bytesRead;

        // Offsets off, return Fail
        if((bytesOffset) > (posOffset)){
            bcopy(buffer.data + position, memPtr,posOffset);
            position = 0;
            memPtr += posOffset;
            bytesRead += posOffset;
            BN++;
            return DFS_FAIL;
        }
        // Bytes read correctly
        else{
            bcopy(buffer.data + position,memPtr,bytesOffset);
            bytesRead += posOffset;
            Printf("Bytes read from inode correctly\n");
            return  bytesRead;
        }
    }
}


//-----------------------------------------------------------------
// DfsInodeWriteBytes writes num_bytes from the memory pointed to 
// by mem to the file represented by the inode handle, starting at 
// virtual byte start_byte. Note that if you are only writing part 
// of a given file system block, you'll need to read that block 
// from the disk first. Return DFS_FAIL on failure and the number 
// of bytes written on success.
//-----------------------------------------------------------------

int DfsInodeWriteBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {
    dfs_block buffer;
    int * bufPtr;
    char * memPtr;
    int bytesWritten;
    int bytesRead;
    int BN;
    int dfsBN;
    int writeFromFunc;
    int position;
    int posOffset, bytesOffset;

    if(inodes[handle].in_use != 1){
        Printf("Filename does not exist\n");
        return DFS_FAIL;
    }

    memPtr = 0;
    bufPtr = NULL;
    bytesRead = 0;
    bytesWritten = 0;
    BN = start_byte / sb.blocksize;
    writeFromFunc = DfsReadBlock(inodes[handle].BTindex,&buffer);
    position = start_byte % sb.blocksize;
    posOffset = sb.blocksize - position;
    bytesOffset = num_bytes - bytesWritten;

    while(bytesWritten < num_bytes){
        if(BN < DFS_INODE_BLOCKTABLE_SIZE){
            dfsBN = inodes[handle].blockTable[BN];
            if(dfsBN == -1){
                dfsBN = DfsInodeAllocateVirtualBlock(handle,BN);
                if(dfsBN == DFS_FAIL){
                    Printf("Not allocated (DfsInodeWriteBytes) 1\n");
                    return DFS_FAIL;
                }
            }
        else{
            if(memPtr == NULL){
                if(inodes[handle].BTindex == -1){
                    dfsBN = DfsInodeAllocateVirtualBlock(handle,BN);
                }
                bytesRead = DfsReadBlock(inodes[handle].BTindex,&buffer);

                if(bytesRead != sb.blocksize){
                    return DFS_FAIL;
                }
                bufPtr = (int *)buffer.data;
            }
            dfsBN = bufPtr[BN - DFS_INODE_BLOCKTABLE_SIZE];
            if(dfsBN == -1){
                dfsBN = DfsInodeAllocateVirtualBlock(handle,BN);
                if(dfsBN == DFS_FAIL){
                    Printf("Not allocated (DfsInodeWriteBytes) 2\n");
                    return DFS_FAIL;
                }
            }
        }
        if(bytesOffset > posOffset){
            bcopy(memPtr,buffer.data + position,posOffset);
            if(DfsWriteBlock(dfsBN,&buffer) != sb.blocksize){
                Printf("Not written (DfsInodeWriteBytes)1\n");
                return DFS_FAIL;
            }
            position = 0;
            memPtr += posOffset;
            bytesWritten += posOffset;
            BN++;
        }
        else{
            bcopy(memPtr,buffer.data + position,posOffset);
            if(DfsWriteBlock(dfsBN,&buffer) != sb.blocksize){
                Printf("Not written (DfsInodeWriteBytes) 2\n");
                return DFS_FAIL;
            }
            bytesWritten += bytesOffset;
            if(start_byte + num_bytes > inodes[handle].file_size){
                inodes[handle].file_size = start_byte +num_bytes;
            }
            Printf("Bytes written from inode correctly\n");
            return bytesWritten;
        }
    }
    return DFS_FAIL;
}
}


//-----------------------------------------------------------------
// DfsInodeFilesize simply returns the size of an inode's file. 
// This is defined as the maximum virtual byte number that has 
// been written to the inode thus far. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeFilesize(uint32 handle) {
    if(inodes[handle].in_use != 1){
        Printf("Filename does not exist\n");
        return DFS_FAIL;
    }
    return inodes[handle].file_size
}


//-----------------------------------------------------------------
// DfsInodeAllocateVirtualBlock allocates a new filesystem block 
// for the given inode, storing its blocknumber at index 
// virtual_blocknumber in the translation table. If the 
// virtual_blocknumber resides in the indirect address space, and 
// there is not an allocated indirect addressing table, allocate it. 
// Return DFS_FAIL on failure, and the newly allocated file system 
// block number on success.
//-----------------------------------------------------------------

uint32 DfsInodeAllocateVirtualBlock(uint32 handle, uint32 virtual_blocknum) {
    dfs_block buffer;
    int * bufPtr;
    int dfsBN;

    if(inodes[handle].in_use != 1){
        Printf("Filename does not exist\n");
        return DFS_FAIL;
    }
    

    if(virtual_blocknum < 0 || virtual_blocknum >= (DFS_INODE_BLOCKTABLE_SIZE + (sb.blocksize / 4))){
        return DFS_FAIL;
    }
    else if(virtual_blocknum < 10){
        if(inodes[handle].blockTable[virtual_blocknum] != -1){
            return DFS_FAIL;
        }
        dfsBN = DfsAllocateBlock();
        inodes[handle].blockTable[virtual_blocknum] = dfsBN;
    }
    else{
        if(inodes[handle].BTindex == -1){
            inodes[handle].BTindex = DfsAllocateBlock();
        }
        if(DfsReadBlock(inodes[handle].BTindex,&buffer) != sb.blocksize){
            return DFS_FAIL;
        }
        dfsBN = DfsAllocateBlock();
        bufPtr = (int *)buffer.data;
        bufPtr[virtual_blocknum - DFS_INODE_BLOCKTABLE_SIZE] = dfsBN;

        if(DfsWriteBlock(inodes[handle].BTindex,&buffer) != sb.blocksize){
            return DFS_FAIL;
        }


    }
    return dfsBN;
}



//-----------------------------------------------------------------
// DfsInodeTranslateVirtualToFilesys translates the 
// virtual_blocknum to the corresponding file system block using 
// the inode identified by handle. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeTranslateVirtualToFilesys(uint32 handle, uint32 virtual_blocknum) {
    dfs_block buffer;
    int * bufPtr;
    int dfsBN;

    if(inodes[handle].in_use != 1){
        Printf("Filename does not exist\n");
        return DFS_FAIL;
    }
    
    if(virtual_blocknum < 10){
        if(inodes[handle].blockTable[virtual_blocknum] != -1){
            return DFS_FAIL;
        }
    }
    else{
        if(inodes[handle].BTindex == -1){
            return DFS_FAIL;
        }
        if(DfsReadBlock(inodes[handle].BTindex,&buffer) != sb.blocksize){
            return DFS_FAIL;
        }
        bufPtr = (int*) buffer.data;
        dfsBN = bufPtr[virtual_blocknum - DFS_INODE_BLOCKTABLE_SIZE];
    }
    return dfsBN;
}

uint32 dfsRatio(){
    uint32 back = sb.blocksize / DISK_BLOCKSIZE;
    return back;
}

uint32 dfsBNUM(uint32 n){
    uint32 back = n * dfsRatio();
    return back;
}
