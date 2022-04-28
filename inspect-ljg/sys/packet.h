#pragma once


#pragma pack(1)

typedef struct _DNS_REQUEST {
	unsigned short id;
	unsigned short flags;
	unsigned short questions;
	unsigned short answerRRS;
	unsigned short authorityRRS;
	unsigned short additionalRRS;

	unsigned char queries[0];

}DNS_REQUEST;


typedef struct {
	unsigned short type;
	unsigned short cls;
}DNS_REQUEST_TYPECLASS;


typedef struct {
	unsigned short name;
	unsigned short type;
	unsigned short cls;
	unsigned int time2live;
	unsigned short datelen;

}DNS_ANSWER_HEAD;


typedef struct {
	DNS_ANSWER_HEAD	hdr;
	unsigned int address;
}DNS_ANSWER_ADDRESS;

#pragma pack()