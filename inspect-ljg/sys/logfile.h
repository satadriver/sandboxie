#ifndef LOGFILE_H
#define LOGFILE_H

#pragma warning(push)
#pragma warning(disable:4201) /* Unnamed struct/union. */

#include <fwpsk.h>

#pragma warning(pop)

NTSTATUS OpenLogFile(SIZE_T log_buffer_size);
void CloseLogFile();

BOOL Log(LARGE_INTEGER* system_time, const char* format, ...);
BOOL FlushLog();

#define LOG_BUFFER_SIZE			(8 * 1024)
#define MIN_LOG_BUFFER_SIZE		(4 * 1024)
#define MIN_REMAINING			512


#define LOG_FILE				L"inspect.log"

#define WRITE_TO_FILE			1

#if WRITE_TO_FILE
typedef struct {
	HANDLE hFile;

	char* buf;
	SIZE_T bufsize;
	SIZE_T used;
} logfile_t;


#endif /* WRITE_TO_FILE */

#endif /* LOGFILE_H */
