#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mergeSort.h"

#define CALL_OR_DIE(call)     \
  {                           \
    SR_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }



int mergeSort(int bufferSize,int fieldNo,int groups,int groupSize,int total_blocks,int sizeofrecord){
	int fDesc;
	remove("TempFile2");
	CALL_OR_DIE(SR_CreateFile("TempFile2"));
	CALL_OR_DIE(SR_OpenFile("TempFile2",&fDesc));
	int loops=1 + ((groups - 1) / (bufferSize-1)); //times I get pairs and merge them in one step
	int index_iterator=1;
	for(int i=0;i<loops;i++){
		int temp=index_iterator;
		int length=0;
		while(1){ //get number of groups to operate to, might be less than bufferSize-1
			if((temp+groupSize>(total_blocks-1))||(length>=bufferSize-1)){
				break;
			}
			else{
				length++;
				temp+=groupSize;
			}
		}
		if((length<=1)&&(index_iterator+groupSize>(total_blocks-1))){ //append to file
			if(index_iterator<total_blocks){
				int fileD;
				CALL_OR_DIE(SR_OpenFile("TempFile",&fileD));
				flush(index_iterator,total_blocks-index_iterator,fDesc,fileD);
				CALL_OR_DIE(SR_CloseFile(fileD));
			}
		}
		else{
			if((temp+groupSize>(total_blocks-1))&&(temp<total_blocks)) length++;
			int indexes[length];
			for(int k=0;k<length;k++){
				indexes[k]=index_iterator;
				index_iterator+=groupSize;
			}
			merge(indexes,length,groupSize,sizeofrecord,fieldNo,fDesc,total_blocks);
		}
	}
	
	groupSize=(bufferSize-1)*groupSize;
	groups = 1 +((total_blocks - 1)/groupSize);
	CALL_OR_DIE(SR_CloseFile(fDesc)); //close TempFile2
	remove("TempFile");
	rename("TempFile2","TempFile");
	
	if(groups==1){
		return 1;
	}
	else{
		
		mergeSort(bufferSize,fieldNo,groups,groupSize,total_blocks,sizeofrecord);
		return 1;
	}
}