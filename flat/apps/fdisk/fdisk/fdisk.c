#include "usertraps.h"
#include "misc.h"

#include "fdisk.h"

dfs_superblock sb;
dfs_inode inodes[DFS_INODE_MAX_NUM];
uint32 fbv[DFS_FBV_MAX_NUM_WORDS];

int diskblocksize = 0; // These are global in order to speed things up
int disksize = 0;      // (i.e. fewer traps to OS to get the same number)

int FdiskWriteBlock(uint32 blocknum, dfs_block *b); //You can use your own function. This function
//calls disk_write_block() to write physical blocks to disk

void main (int argc, char *argv[])
{
    // STUDENT: put your code here. Follow the guidelines below. They are just the main steps.
    // You need to think of the finer details. You can use bzero() to zero out bytes in memory
    
    char dBlock_buf[disk_blocksize()];
    char * hold;
    
    //Initializations and argc check
    
    if(argc != 1){
        printf("ERR -> No expected inputs");
        Exit();                 // make sure this exists
    }
    
    disksize = disk_size();
    diskblocksize = disk_blocksize();
    
    // Need to invalidate filesystem before writing to it to make sure that the OS
    // doesn't wipe out what we do here with the old version in memory
    // You can use dfs_invalidate(); but it will be implemented in Problem 2. You can just do
    // sb.valid = 0
    
    sb.valid = 0;
    sb.blocksize = DFS_BLOCKSIZE;
    sb.blocks = disksize / sb.blocksize;
    
    sb.num_inodes = FDISK_NUM_INODES;                  // double check these match
    
    sb.inodeBlockStart = FDISK_INODE_BLOCK_START;
    
    sb.fbvBlockStart = FDISK_BLOCK_START;
    
    sb.dataBlockStart = FDISK_BLOCK_START + (((sb.blocks + 31) / 32 * 4) + (sb.blocksize-1)) / sb.blocksize
    
    // ----------- ADD print statements for checks/output ------------
    
    
    
    
    
    // Make sure the disk exists before doing anything else
    
    if(disk_create() == DISK_FAIL){
        printf("disk doesn't exist\n");
    }
    
    
    // Write all inodes as not in use and empty (all zeros)
    int i,j;
    for(i = 0; i < sb.blocks; i++){
        inodes[i].in_use = 0;
        inodes[i].file_size = 0;
        inodes[i].filename[0] = '\0';
        for(j = 0; j < DFS_INODE_BLOCKTABLE_SIZE; j++){
            inodes[i].blockTable[j] = -1;
        }
        inodes[i].BTindex = -1;
        inodes[i].BTindex2 = -1;
    }
    
    for(i = sb.inodeBlockStart; i < sb.fbvBlockStart; i++){
        FdiskWriteBlock(i,&hold);
    }
    // Next, setup free block vector (fbv) and write free block vector to the disk
    
    fbv[0] = 0xFFFFFF00;
    for(i = 1; i < DFS_MAX_NUM_WORDS; i++){
        fbv[i] = 0;
    }
    hold = (char *)fbv;
    
    for(i = sb.fbvBlockStart; i < sb.dataBlockStart; i++){
        FdiskWriteBlock(i,&hold);
    }
    // Finally, setup superblock as valid filesystem and write superblock and boot record to disk:
    
    sb.valid = 1;
    bcopy((char*)&sb,dBlock_buf,sizeof(dfs_superblock));
    disk_write_block(FDISK_BOOT_FILESYSTEM_BLOCKNUM+1,dBlock_buf);
    
    // boot record is all zeros in the first physical block, and superblock structure goes into the second physical block
    Printf("fdisk (%d): Formatted DFS disk for %d bytes.\n", getpid(), disksize);
}

int FdiskWriteBlock(uint32 blocknum, dfs_block *b) {
    // STUDENT: put your code here
    uint32 writes = sb.blocksize / diskblocksize;
    uint32 disk_blocknum;
    
    for(int i=0; i < writes; i++){
        // Calc Physical Block Number:
        physical_blocknum = i + (blocknum * writes);
        
        if(disk_write_block(physical_blocknum,b) == DISK_FAIL){
            printf("Error writing physical blocknum");
            return DISK_FAIL;
        }
        b = b + disk_blocksize;
    }
    return DISK_SUCCESS;
}


























