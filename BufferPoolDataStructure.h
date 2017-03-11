#ifndef BUFFERPOOL_DATASTRUCTURE_H
#define BUFFERPOOL_DATASTRUCTURE_H

// includes
#include "dt.h"

// defines
#define RC_PAGE_NOT_FOUND_IN_CACHE 400
#define RC_FAIL_SHUTDOWN_POOL 401
#define RC_FAIL_FORCE_PAGE_DUETO_PIN_EXIT 402
#define RC_ALL_PAGE_RESOURCE_OCCUPIED 403

// typedefs
typedef struct BM_TraverseList
{
	int poolIndex;
	int pageIndex;
	struct BM_TraverseList* next;
	struct BM_TraverseList* prev;
} BM_TraverseList;

typedef struct BM_BufferPoolData
{
	int* pageNumberList;
	int* fixCounts;
	int* clockFlag;
	int numberOfAvailablePageFrames;
	char* poolData;
	bool* dirtyFlags;
	int numberOfReadOperations;
	int numberOfWriteOperations;
	BM_TraverseList *head, *curToBeReplaced, *tail;
} BM_BufferPoolData;

#endif