#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "replacementStrategies.h"
#include "BufferPoolDataStructure.h"


PageNumber pinPageForFIFOLRU(BM_BufferPool*const bm, BM_PageHandle*const page, const PageNumber pageNum, SM_FileHandle *fileHandle)
{	
	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	BM_TraverseList *temp = bufferPoolData->head;
	PageNumber bufferPoolPageIndex = NO_PAGE;

	while (temp != NULL)
	{
		if (bufferPoolData->fixCounts[temp->poolIndex] == 0)
		{
			break;
		}
		temp = temp->next;
	}
	if (temp != NULL)
	{
		if (temp != bufferPoolData->tail)
		{
			bufferPoolData->tail->next = temp;

			temp->next->prev = temp->prev;
			if (temp == bufferPoolData->head)
			{
				bufferPoolData->head = bufferPoolData->head->next;
			}
			else
			{
				temp->prev->next = temp->next;
			}
			temp->prev = bufferPoolData->tail;
			bufferPoolData->tail = bufferPoolData->tail->next;
			bufferPoolData->tail->next = NULL;
		}

		if (bufferPoolData->dirtyFlags[bufferPoolData->tail->poolIndex] == true)
		{
			char* memory = bufferPoolData->poolData + bufferPoolData->tail->poolIndex * PAGE_SIZE * sizeof(char);
			int old_pageNum = bufferPoolData->tail->pageIndex;
			writeBlock(old_pageNum, fileHandle, memory);
			bufferPoolData->dirtyFlags[bufferPoolData->tail->poolIndex] = false;
			bufferPoolData->numberOfWriteOperations++;
		}
		bufferPoolData->tail->pageIndex = pageNum;
		bufferPoolData->pageNumberList[bufferPoolData->tail->poolIndex] = pageNum;
		bufferPoolPageIndex = bufferPoolData->tail->poolIndex;
	}

	return bufferPoolPageIndex;
}

void pinPageInCacheForLRU(BM_BufferPool*const bm, BM_PageHandle*const page, const PageNumber pageNum)
{	
	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	BM_TraverseList* current = bufferPoolData->head;
	BM_TraverseList* tail = bufferPoolData->tail;
	while (current != NULL)
	{
		if (current->pageIndex == pageNum)
		{
			break;
		}

		current = current->next;
	}

	if (current != tail)
	{
		bufferPoolData->tail->next = current;
		current->next->prev = current->prev;

		if (current == bufferPoolData->head)
		{
			bufferPoolData->head = bufferPoolData->head->next;
		}
		else
		{
			current->prev->next = current->next;
		}
		current->prev = bufferPoolData->tail;
		bufferPoolData->tail = bufferPoolData->tail->next;
		bufferPoolData->tail->next = NULL;
	}
}

PageNumber pinPageForCLOCK(BM_BufferPool*const bm, BM_PageHandle*const page, const PageNumber pageNum)
{
	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	BM_TraverseList *temp = NULL;
	PageNumber bufferPoolPageIndex = NO_PAGE;

	if (bufferPoolData->curToBeReplaced == NULL)
	{
		bufferPoolData->curToBeReplaced = bufferPoolData->head;
	}
	while (bufferPoolData->clockFlag[bufferPoolData->curToBeReplaced->poolIndex] == 1)
	{
		bufferPoolData->clockFlag[bufferPoolData->curToBeReplaced->poolIndex] = 0;
		if (bufferPoolData->curToBeReplaced == bufferPoolData->tail)
		{
			bufferPoolData->curToBeReplaced = bufferPoolData->head;
		}
		else
		{
			bufferPoolData->curToBeReplaced = bufferPoolData->curToBeReplaced->next;
		}
	}
	bufferPoolData->curToBeReplaced->pageIndex = pageNum;
	bufferPoolData->pageNumberList[bufferPoolData->curToBeReplaced->poolIndex] = pageNum;
	bufferPoolData->clockFlag[bufferPoolData->curToBeReplaced->poolIndex] = 1;
	temp = bufferPoolData->curToBeReplaced;

	if (bufferPoolData->curToBeReplaced == bufferPoolData->tail)
	{
		bufferPoolData->curToBeReplaced = bufferPoolData->head;
	}
	else
	{
		bufferPoolData->curToBeReplaced = bufferPoolData->curToBeReplaced->next;
	}
	bufferPoolPageIndex = temp->poolIndex;

	return bufferPoolPageIndex;
}