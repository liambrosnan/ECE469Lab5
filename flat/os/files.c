#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

// STUDENT: put your file-level functions here

uint32 FileOpen(char *filename, char *mode){
    int inode_name;
    
    
    inode_name = DfsInodeFilename
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
