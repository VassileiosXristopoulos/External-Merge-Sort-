#ifndef RECORD_H
#define RECORD_H

typedef struct Record Record;
struct Record {
	int id;
	char name[15];
	char surname[20];
	char city[20];
};

#endif