#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helperFunctions.h"

#define CALL_OR_DIE(call)     \
  {                           \
    SR_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }



void flush(int index,int blocks_num,int fileDesc,int fDesc){
	
	char *data;
	BF_Block *block;
	BF_Block_Init(&block);
	for(int i=0;i<blocks_num;i++){
		
		BF_GetBlock(fDesc,index,block);
		data=BF_Block_GetData(block);
		Record record;
		int offset=4;
		for(int j=0;j<*(data);j++){
			record.id=*((int *)(data+offset));
			offset+=sizeof(int);
			strcpy(record.name, data+offset);
			offset+=*(data+1);
			strcpy(record.surname, data+offset);
			offset+=*(data+2);
			strcpy(record.city, data+offset);
			offset+=*(data+3)+1;
			CALL_OR_DIE(SR_InsertEntry(fileDesc,record));

		}
		CALL_OR_DIE(BF_UnpinBlock(block));
		index++;
	}
}
void merge(int *indexes,int length,int groupSize,int sizeofrecord,int fieldNo,int fDesc,int total_blocks){
	int fileDesc;
	Record record;
	CALL_OR_DIE(SR_OpenFile("TempFile",&fileDesc));
	BF_Block *block_array[length];
 	char *data_array[length]; //pointer at the current record 
 	int head[length]; //how many records I have written from a block
 	int blocks_move[length]; //how many times a group's pointer changed block
 	int numOfRecs[length];
	for(int k=0;k<length;k++){
		BF_Block_Init(&block_array[k]);
		CALL_OR_DIE(BF_GetBlock(fileDesc,indexes[k],block_array[k]));
		char* d = BF_Block_GetData(block_array[k]);
		numOfRecs[k]=*d;
		data_array[k]=d+4;
		head[k]=0;
		blocks_move[k]=0;
	}
	while(resume(blocks_move,length,groupSize)==1){
		int position=findVictim(data_array,length,fieldNo,blocks_move,groupSize,numOfRecs,head);
		/*----------position represents the group-----------------------------*/
	
		if(pop(&record,data_array[position],head,position,numOfRecs)!=NULL){
			char*t=data_array[position]+sizeofrecord;
			data_array[position]=t;
			head[position]+=1;
			CALL_OR_DIE(SR_InsertEntry(fDesc,record));
			if(head[position]>=numOfRecs[position]) {
				CALL_OR_DIE(BF_UnpinBlock(block_array[position]));
				indexes[position]+=1;
				CALL_OR_DIE(BF_GetBlock(fileDesc,indexes[position],block_array[position]));
				char* d=BF_Block_GetData(block_array[position]);
				numOfRecs[position]=*d;
				data_array[position]= d+4;
				head[position]=0;
				blocks_move[position]+=1;
			}
		}
		else{//pointer reached end of block
			if((blocks_move[position]+1<groupSize)&&(indexes[position]+1<total_blocks)){
				CALL_OR_DIE(BF_UnpinBlock(block_array[position]));
				indexes[position]+=1;
				CALL_OR_DIE(BF_GetBlock(fileDesc,indexes[position],block_array[position]));
				char* d=BF_Block_GetData(block_array[position]);
				numOfRecs[position]=*d;
				data_array[position]= d+4;
				head[position]=0;
				blocks_move[position]+=1;
			}
			else{
				blocks_move[position]=groupSize+1; //ELIMINATE
			}
		}
	}
	int pos;
	for(int k=0;k<length;k++) {
		if(blocks_move[k]<groupSize){
			pos=k;
		}
	}
	char *data;
	BF_Block *block;
	BF_Block_Init(&block);
	for(int k=0;k<groupSize-blocks_move[pos];k++){ // appending the remaining data, continuing from the record we stoped at a block
		if(indexes[pos]+k<total_blocks){
			BF_GetBlock(fileDesc,indexes[pos]+k,block);
		}
		else{
			break;
		}
		data=BF_Block_GetData(block);
		if(k==0){
			int offset=head[pos]*sizeofrecord+4;
			while(head[pos]<(*data)){
				record.id=*((int *)(data+offset));
				offset+=sizeof(int);
				strcpy(record.name, data+offset);
				offset+=GetOffset(2);
				strcpy(record.surname, data+offset);
				offset+=GetOffset(3);
				strcpy(record.city, data+offset);
				offset+=GetOffset(4)+1;
				CALL_OR_DIE(SR_InsertEntry(fDesc,record));
				head[pos]+=1;
			}
		}
		else{
				int offset=4;
				for(int i=0;i<*(data);i++){
					record.id=*((int *)(data+offset));
					offset+=sizeof(int);
					strcpy(record.name, data+offset);
					offset+=*(data+1);
					strcpy(record.surname, data+offset);
					offset+=*(data+2);
					strcpy(record.city, data+offset);
					offset+=*(data+3)+1;
					CALL_OR_DIE(SR_InsertEntry(fDesc,record));
				}
		}
		CALL_OR_DIE(BF_UnpinBlock(block));
	}
	for(int i=0;i<length;i++){
		BF_UnpinBlock(block_array[i]);
	}
	CALL_OR_DIE(SR_CloseFile(fileDesc));
 }
int resume(int *blocks_move,int length,int groupSize){
	int count=0;
	for(int k=0;k<length;k++){
		if(blocks_move[k]>=groupSize) count++; //inactive groups
	}
	if(length-count>1) return 1;
	else return 0;
}
int findVictim(char**data_array,int length,int fieldNo,int* blocks_move,int groupSize,int *numOfRecs,int* head){
	char min[GetOffset(fieldNo+1)+1];
	int _ret;
	int count=0;
	int sum=0;
	int i=1;
	int copy=fieldNo;
	while(copy>0){
	 sum+=GetOffset(i++);
	 copy--;
	} //sum is the offset for the field of the struct we compare
	while(count<length){
		if((blocks_move[count]<groupSize)&&(head[count]<numOfRecs[count])){ //search for victim ONLY the active blocks
			memcpy(min,data_array[count]+sum,GetOffset(fieldNo+1));
			_ret=count;
			break;
		}
		count++;
	}
	for(int k=0;k<length;k++){
		char field[GetOffset(fieldNo+1)+1];
		if((blocks_move[k]<groupSize)&&(head[k]<numOfRecs[k])){
			memcpy(field,data_array[k]+sum,GetOffset(fieldNo+1));
			if(key_compare(field,min,fieldNo)<0){
				memcpy(min,field,GetOffset(fieldNo+1));
				_ret=k;
			}
		}
	}
	return _ret;
}
int GetOffset(int fieldNo){
	int fileDesc;
	CALL_OR_DIE(SR_OpenFile("unsorted_data.db", &fileDesc));
	BF_Block *block;
  	BF_Block_Init(&block);
	CALL_OR_DIE(BF_GetBlock(fileDesc,1,block));
	char *data;
	data=BF_Block_GetData(block);
	int offset;
	if(fieldNo==0) offset=0;
	else if(fieldNo==1) offset=sizeof(int);
	else if(fieldNo==2) offset=*(data+1);
	else if(fieldNo==3) offset=*(data+2);
	else if(fieldNo==4) offset=*(data+3);
	CALL_OR_DIE(BF_UnpinBlock(block));
	CALL_OR_DIE(SR_CloseFile(fileDesc));
	return offset;
}

char * pop(Record* record,char* data,int *head,int position,int *numOfRecs){
	if(head[position]<numOfRecs[position]){
		int offset=GetOffset(0);
		record->id=*((int*)(data+offset));
		offset+=GetOffset(1);
		strcpy(record->name, data+offset);
		offset+=GetOffset(2);
		strcpy(record->surname, data+offset );
		offset+=GetOffset(3);
		strcpy(record->city, data+offset);
		return "1";
	}
	else{
		record=NULL;
		return NULL;
	}
}

int compare(Record *arr,int position1,int position2,int fieldNo){
	if(fieldNo==0){
		return arr[position1].id - arr[position2].id;
	}
	else if(fieldNo==1){
		return strcmp((char*)(arr[position1].name),(char*)(arr[position2].name));
	}
	else if(fieldNo==2){
		return strcmp((char*)(arr[position1].surname),(char*)(arr[position2].surname));
	}
	else if(fieldNo==3){
		return strcmp((char*)(arr[position2].city),(char*)(arr[position2].city));
	}
}
int key_compare(char *key1,char *key2,int fieldNo){
	if(fieldNo==0){
		return *((int*)(key1)) - *((int*)(key2));
	}
	else if(fieldNo==1){
		return strcmp((char*)(key1),(char*)(key2));
	}
	else if(fieldNo==2){
		return strcmp((char*)(key1),(char*)(key2));
	}
	else if(fieldNo==3){
		return strcmp((char*)(key1),(char*)(key2));
	}
}