#ifndef __DFS_SHARED__
#define __DFS_SHARED__

typedef struct dfs_superblock {
    // STUDENT: put superblock internals here
    int valid;
    int blocksize;
    int blocks;
    int num_inodes;
    int inodeBlockStart;
    int fbvBlockStart;
    int dataBlockStart
} dfs_superblock;

#define DFS_BLOCKSIZE 1024  // Must be an integer multiple of the disk blocksize

typedef struct dfs_block {
    char data[DFS_BLOCKSIZE];
} dfs_block;


// Size declarations for Inode
#define DFS_INODE_MAX_FILENAME_LENGTH 72
#define DFS_INODE_BLOCKTABLE_SIZE 10


typedef struct dfs_inode {
    // STUDENT: put inode structure internals here
    // IMPORTANT: sizeof(dfs_inode) MUST return 128 in order to fit in enough
    // inodes in the filesystem (and to make your life easier).  To do this,
    // adjust the maximumm length of the filename until the size of the overall inode
    // is 128 bytes.
    
    int in_use;
    int file_size;
    char filename[DFS_INODE_MAX_FILENAME_LENGTH];
    int blockTable[DFS_INODE_BLOCKTABLE_SIZE];
    int BTindex;
    int BTindex2;
} dfs_inode;

#define DFS_MAX_FILESYSTEM_SIZE 0x1000000  // 16MB

#define DFS_FAIL -1
#define DFS_SUCCESS 1

#define DFS_MAX_NUM_BLOCKS (DFS_MAX_FILESYSTEM_SIZE / DFS_BLOCKSIZE)
#define DFS_MAX_NUM_WORDS (DFS_BLOCKSIZE * 8) / 4
#define DFS_INODE_NMAX_NUM 128
#define DFS_SB_PBLOCK 1



#endif
