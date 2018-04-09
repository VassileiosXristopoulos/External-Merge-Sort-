#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "sort_file.h"

#define CALL_OR_DIE(call)     \
  {                           \
    SR_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

SR_ErrorCode SR_Init() {
  // Your code goes here

  return SR_OK;
}

SR_ErrorCode SR_CreateFile(const char *fileName) {
  // Your code goes here
  CALL_OR_DIE(BF_CreateFile(fileName));
  BF_Block *block;
  BF_Block_Init(&block);
  int fDesc;
  CALL_OR_DIE(BF_OpenFile(fileName,&fDesc));
  CALL_OR_DIE(BF_AllocateBlock(fDesc,block));
  char *info="SORT_FILE";
  char *data=BF_Block_GetData(block);
  memcpy(data,info,10);

  BF_Block_SetDirty(block);
  CALL_OR_DIE(BF_UnpinBlock(block));
  CALL_OR_DIE(BF_CloseFile(fDesc));

  return SR_OK;
}

SR_ErrorCode SR_OpenFile(const char *fileName, int *fileDesc) {
  // Your code goes here
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_OR_DIE(BF_OpenFile(fileName,fileDesc));
  CALL_OR_DIE(BF_GetBlock(*fileDesc,0,block));
  char *data="SORT_FILE";
  char *ptr=BF_Block_GetData(block);
  if(strcmp(ptr,data)!=0){
    return SR_ERROR;
  }

  CALL_OR_DIE(BF_UnpinBlock(block));
  return SR_OK;
}

SR_ErrorCode SR_CloseFile(int fileDesc) {
  // Your code goes here
  CALL_OR_DIE(BF_CloseFile(fileDesc));
  
  return SR_OK;
}

SR_ErrorCode SR_InsertEntry(int fileDesc,	Record record) {
  // Your code goes here
  int blocks_num;
  CALL_OR_DIE(BF_GetBlockCounter(fileDesc,&blocks_num));
  BF_Block *block;
  char * data;
  int flag;
  BF_Block_Init(&block);
  
  if(blocks_num>1){
    CALL_OR_DIE(BF_GetBlock(fileDesc,blocks_num-1,block));
    data=BF_Block_GetData(block);
    flag=(BF_BLOCK_SIZE-(*data)*sizeof(record))>=sizeof(record);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  int count=0;
  if(blocks_num==1/*infoBlock*/ || flag==0 /*no space in the block*/){
    //create a new block
    int recordCounter=1;
    CALL_OR_DIE(BF_AllocateBlock(fileDesc,block));
    data=BF_Block_GetData(block);
    memcpy(data,&recordCounter,1);
    
    int temp;
    temp=sizeof(record.name);
    memcpy(data+1,&(temp),1);
    temp=sizeof(record.surname);
    memcpy(data+2,&(temp),1);
    temp=sizeof(record.city);
    memcpy(data+3,&(temp),1);

    int offset=4;
    memcpy(data+offset,&(record.id),sizeof(record.id));
    offset+=sizeof(record.id);
    memcpy(data+offset,record.name,sizeof(record.name));
    offset+=sizeof(record.name);
    memcpy(data+offset,record.surname,sizeof(record.surname));
    offset+=sizeof(record.surname);
    memcpy(data+offset,record.city,sizeof(record.city));

    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));

  }
  else{
    //write on the already existing block
    CALL_OR_DIE(BF_GetBlock(fileDesc,blocks_num-1,block));
    data=BF_Block_GetData(block);
    char *ptr=data;
    data+=(*data)*sizeof(record)+4;
    int recordCounter=(*ptr);
    recordCounter++;
    memcpy(ptr,&recordCounter,1);

    int offset=0;
    memcpy(data+offset,&(record.id),sizeof(record.id));
    offset+=sizeof(record.id);
    memcpy(data+offset,record.name,sizeof(record.name));
    offset+=sizeof(record.name);
    memcpy(data+offset,record.surname,sizeof(record.surname));
    offset+=sizeof(record.surname);
    memcpy(data+offset,record.city,sizeof(record.city));

    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  return SR_OK;
}

SR_ErrorCode SR_SortedFile(
  const char* input_filename,
  const char* output_filename,
  int fieldNo,
  int bufferSize
) {
  if((bufferSize<3)||(bufferSize>BF_BUFFER_SIZE)) return SR_ERROR;
  remove(output_filename);
  int fileDesc,temp,blocks;
  CALL_OR_DIE(SR_OpenFile(input_filename,&fileDesc));
  CALL_OR_DIE(BF_GetBlockCounter(fileDesc,&blocks));
  /*----------get size of record----------------------- */
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_OR_DIE(BF_GetBlock(fileDesc,1,block));
  char *data;
  data=BF_Block_GetData(block);
  int sizeofrecord=sizeof(int)+*(data+1)+*(data+2)+*(data+3)+1;
  CALL_OR_DIE(BF_UnpinBlock(block));
  /*----------------------------------------------------*/
  int groupSize=bufferSize;
  int groups=1 + ((blocks - 1) / bufferSize);
  /*----------------------------------------------------------------------*/
  FirstSort(fileDesc,bufferSize,fieldNo);

  mergeSort(bufferSize,fieldNo,groups,groupSize,blocks,sizeofrecord);
  /*--------------put TempFile to output_filename-------------------------*/
  remove(output_filename);
  rename("TempFile",output_filename);
  CALL_OR_DIE(SR_CloseFile(fileDesc));
  return SR_OK;
}


SR_ErrorCode SR_PrintAllEntries(int fileDesc) {
  int blocks_num,bc;
  CALL_OR_DIE(BF_GetBlockCounter(fileDesc,&blocks_num));
  BF_Block *block;
  BF_Block_Init(&block);
  for(bc=1;bc<blocks_num;bc++){//iterating through the blocks
    CALL_OR_DIE(BF_GetBlock(fileDesc,bc,block));
    char *data;
    data=BF_Block_GetData(block);
    int offset=4;
    for(int i=0;i<(*data);i++){//iterating through the records
      printf("%d ", *((int *)(data+offset))); //id
      offset+= sizeof(int);
      printf("%s ", data+offset); //name
      offset+= *(data+1);
      printf("%s ", data+offset ); //surname
      offset+= *(data+2);
      printf("%s\n", data+offset ); //city
      offset+= *(data+3)+1;
    }
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  
  return SR_OK;
}






