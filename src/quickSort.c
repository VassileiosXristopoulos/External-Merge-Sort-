#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/quickSort.h"

#define CALL_OR_DIE(call)     \
  {                           \
    SR_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }



void FirstSort(int fileDesc,int bufferSize,int fieldNo){

  BF_Block *temp_block;
  BF_Block_Init(&temp_block);
  CALL_OR_DIE(BF_GetBlock(fileDesc,1,temp_block));
  char *temp_data;
  temp_data=BF_Block_GetData(temp_block);
  int blocks_num;
  CALL_OR_DIE(BF_GetBlockCounter(fileDesc,&blocks_num));//number of blocks in the file
  int numOfRecs=*(temp_data);//num of records in a full block(first one in this case)
  CALL_OR_DIE(BF_UnpinBlock(temp_block));
  int bc=1;//this will be our block counter
  const char *tempfile="TempFile";
  remove(tempfile);
  CALL_OR_DIE(SR_CreateFile(tempfile));
  int fDesc;
  CALL_OR_DIE(SR_OpenFile(tempfile,&fDesc));
  blocks_num--;
  int sizeofname,sizeofsurname,sizeofcity;
  while(blocks_num>0) {
    int pos=0,i,j;
    int main_counter;
    /*with this if-else statement we handle the occasion where we have
      blocks remaining unsorted after sorting bufferSize-tuples of blocks
      -----------where blocks_num<bufferSize and blocks_num!=0-----------*/
    if(blocks_num>=bufferSize){
      main_counter=bufferSize;
    }
    else{
      main_counter=blocks_num;
      if(blocks_num==0){
        break;
      }
    }
    int numOfIds=numOfRecs*main_counter;
    Record arr[numOfIds];
    int numArray[main_counter];
    for(i=0;i<main_counter;i++){
      BF_Block *block;
      BF_Block_Init(&block);
      CALL_OR_DIE(BF_GetBlock(fileDesc,bc,block));
      char *data;
      data=BF_Block_GetData(block);
      numArray[i]=*(data);
      sizeofname=*(data+1);
      sizeofsurname=*(data+2);
      sizeofcity=*(data+3);
      int offset=4;
      for(j=0;j<numOfRecs;j++){
        /*this check is for the case than last block of the file
          is not full,so it makes its id values that are not containing
          a record,equal to -1*/
        if(j!=0 && *((int *)(data+offset))==0){
          arr[pos].id= -1;
          strcpy(arr[pos].name,"");
          strcpy(arr[pos].surname,"");
          strcpy(arr[pos].city,"");
          offset+=sizeof(int)+sizeofname+sizeofsurname+sizeofcity;
          pos++;
          continue;
        }
        else{
          arr[pos].id= *((int *)(data+offset));
        }
        offset+= sizeof(int);
        strcpy(arr[pos].name,data+offset);
        offset+= sizeofname;
        strcpy(arr[pos].surname,data+offset);
        offset+= sizeofsurname;
        strcpy(arr[pos].city,data+offset);
        offset+= sizeofcity+1;
        pos++;
      }
      CALL_OR_DIE(BF_UnpinBlock(block));
      bc++;
    }
    quicksort(arr,0,numOfIds-1,fieldNo);
    //now we are going to copy the sorted elements in the correct block
    int pos1=0;
    for(i=0;i<main_counter;i++){
      BF_Block *block1;
      BF_Block_Init(&block1);
      CALL_OR_DIE(BF_AllocateBlock(fDesc,block1));
      char *data1;
      data1=BF_Block_GetData(block1);
      char *start=data1;
      memcpy(data1,&numArray[i],1);
      memcpy(data1+1,&sizeofname,1);
      memcpy(data1+2,&sizeofsurname,1);
      memcpy(data1+3,&sizeofcity,1);
      int offset=4;
      int temp=0;
      while(temp<numOfRecs){
        /*this check is for the case than last block of the file
          is not full,so it just doesn't pass some extra zeros to the tempfile*/
        if(arr[pos1].id==-1){
          pos1++;
        }
        else{
          memcpy(data1+offset,&arr[pos1].id,sizeof(int));
          offset+= sizeof(int);
          memcpy(data1+offset,arr[pos1].name,sizeofname);
          offset+= sizeofname;
          memcpy(data1+offset,arr[pos1].surname,sizeofsurname);
          offset+= sizeofsurname;
          memcpy(data1+offset,arr[pos1].city,sizeofcity);
          offset+= sizeofcity+1;
          pos1++;
          temp++;
        }
      }
      BF_Block_SetDirty(block1);
      CALL_OR_DIE(BF_UnpinBlock(block1));
    }
    blocks_num-=main_counter;
  }
  
  CALL_OR_DIE(BF_CloseFile(fDesc));
}

void quicksort(Record *arr,int first,int last,int fieldNo){
  int i, j, pivot;
  Record temp;

  if(first<last){
    pivot=first;
    i=first;
    j=last;
    while(i<j){
      while( (!(compare(arr,i,pivot,fieldNo)>0)) && (i<last) ){
        i++;
      }
      while(compare(arr,j,pivot,fieldNo)>0){
        j--;
      }
      if(i<j){
        temp.id=arr[i].id;
        memcpy(temp.name,arr[i].name,sizeof(temp.name));
        memcpy(temp.surname,arr[i].surname,sizeof(temp.surname));
        memcpy(temp.city,arr[i].city,sizeof(temp.city));

        arr[i].id=arr[j].id;
        memcpy(arr[i].name,arr[j].name,sizeof(temp.name));
        memcpy(arr[i].surname,arr[j].surname,sizeof(temp.surname));
        memcpy(arr[i].city,arr[j].city,sizeof(temp.city));

        arr[j].id=temp.id;
        memcpy(arr[j].name,temp.name,sizeof(temp.name));
        memcpy(arr[j].surname,temp.surname,sizeof(temp.surname));
        memcpy(arr[j].city,temp.city,sizeof(temp.city));
      }
    }
    temp.id=arr[pivot].id;
    memcpy(temp.name,arr[pivot].name,sizeof(temp.name));
    memcpy(temp.surname,arr[pivot].surname,sizeof(temp.surname));
    memcpy(temp.city,arr[pivot].city,sizeof(temp.city));

    arr[pivot].id=arr[j].id;
    memcpy(arr[pivot].name,arr[j].name,sizeof(temp.name));
    memcpy(arr[pivot].surname,arr[j].surname,sizeof(temp.surname));
    memcpy(arr[pivot].city,arr[j].city,sizeof(temp.city));

    arr[j].id=temp.id;
    memcpy(arr[j].name,temp.name,sizeof(temp.name));
    memcpy(arr[j].surname,temp.surname,sizeof(temp.surname));
    memcpy(arr[j].city,temp.city,sizeof(temp.city));

    quicksort(arr,first,j-1,fieldNo);
    quicksort(arr,j+1,last,fieldNo);
  }
}