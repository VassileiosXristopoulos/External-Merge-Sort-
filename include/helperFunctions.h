#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H
#include "Record.h"
#include "sort_file.h"
int compare(Record *,int ,int ,int );
void merge(int *,int ,int ,int,int,int,int );
char* pop(Record*,char* ,int *,int ,int *);
int key_compare(char *,char *,int);
int findVictim(char**,int ,int,int*,int, int*,int* );
int GetOffset(int );
int resume(int *,int ,int );
void flush(int,int,int,int);
int compare(Record *,int ,int ,int );
#endif