#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#pragma warning(push)
#pragma warning(disable:4201) /* Unnamed struct/union. */

#include <fwpsk.h>

#pragma warning(pop)

#include "packet_pool.h"

BOOL InitWorkerThread(unsigned max_packets);
void FreeWorkerThread();

NTSTATUS StartWorkerThread();
void StopWorkerThread();

static void ThreadProc(void* context);

BOOL GivePacketToWorkerThread(packet_t* packet);

#define FLUSH_LOGS_EVERY_MS 1000

#pragma pack(1)

typedef struct {
	packet_t** packets;
	unsigned max_packets;
	unsigned count;

	void* thread;
	BOOL running;

	KSPIN_LOCK spin_lock;
	KSEMAPHORE semaphore;
} worker_thread_t;
#pragma pack()

#endif /* WORKER_THREAD_H */
