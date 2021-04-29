#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

// I think this is included in dfs.h, but its constants weren't being reconized
#include "dfs_shared.h"

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

// STUDENT: put your file-level functions here

static file_descriptor files[DFS_INODE_NMAX_NUM];

uint32 FileOpen(char *filename, char *mode){
    int i;
    int inode_name;
    int successFlag;
    int handle;
    int currPid;
    
    successFlag = 0;
    inode_name = DfsInodeFilenameExists(filename);
    if(inode_name != DFS_FAIL){
        for(i=0; i < DFS_INODE_NMAX_NUM;i++){
            if(files[i].inode == inode_name){
                successFlag = 1;
                handle = i;
            }
        }
        currPid = GetCurrentPid();
        if(successFlag == 0){
            return FILE_FAIL;
        }
        if(currPid != files[handle].file_pid){
            return FILE_FAIL;
        }

    }
    
}

int FileClose(int handle){

}

int FileRead(int handle, void *mem, int num_bytes){
    int bytesRead;
}

int FileWrite(int handle, void *mem, int num_bytes){

}

int FileSeek(int handle, int num_bytes, int from_where){

}

int FileDelete(char *filename){

}
