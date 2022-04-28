#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H

#include "packet_pool.h"

void ProcessPacket(packet_t* packet);

BOOL ParseHttpPacket(const UINT8* data,
	SIZE_T len,
	const UINT8** method,
	SIZE_T* methodlen,
	const UINT8** path,
	SIZE_T* pathlen,
	const UINT8** host,
	SIZE_T* hostlen);

#endif /* PACKET_PROCESSOR_H */
