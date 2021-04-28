#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "disk.h"
#include "dfs.h"

void RunOSTests() {

  uint32 handle;
  uint32 filesize;
  uint32 updatedFilesize;

  char write[4] = "test";
  char read_buffer[4];

  // STUDENT: run any os-level tests here

// Open Inode
  printf("Beginning Tests:\n\n");
  printf("DfsInodeOpen:\n");

  handle = DfsInodeOpen("new_inode");
  filesize = DfsInodeFilesize(handle);
  
  printf("File Handle: %d\n",handle);
  printf("File Size: %d\n",filesize);

// Write Bytes
  printf("\nWriting Bytes...\n\n");
  printf("DfsInodeWriteBytes:\n");

  DfsInodeWriteBytes(handle,&write,20,4);
  updatedFilesize = DfsInodeFilesize(handle);

  printf("Expected Filesize: 24\n");
  printf("Updated Filesize: %d\n",updatedFilesize);

// Read Bytes
  printf("\nReading Bytes...\n\n");
  printf("DfsInodeReadBytes:\n");

  DfsInodeReadBytes(handle,&read_buffer,20,4);

  printf("Read from file: (expecting 'test'\n");
  int i;
  for(i = 0; i < 4;i++){
    printf("%c",read_buffer[i]);
  }
  printf("\n");

  // Delete
  printf("\nDelete Inode\n\n");
  DfsInodeDelete(handle);
  printf("Inode deleted\n");

  if(DfsInodeFilenameExists("test") == DFS_FAIL){
    printf("File successfully deleted\n");
  }
  else{
    printf("File not successfully deleted\n");
  }











}

