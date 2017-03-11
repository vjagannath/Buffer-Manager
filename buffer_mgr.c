#include <stdlib.h>
#include <sys/stat.h>
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "replacementStrategies.h"
#include "BufferPoolDataStructure.h"
#include <string.h>

RC initBufferPool(BM_BufferPool*const bm, const char*const pageFileName,
	const int numPages, ReplacementStrategy strategy,
	void* stratData)
{
	/*
	* Check if the page file exists.
	* If yes, then initialize a new buffer pool for the given page file
	*/

	struct stat buffer;
	if (stat(pageFileName, &buffer) != 0)
	{
		return RC_FILE_NOT_FOUND;
	}

	bm->pageFile = (char *)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	bm->mgmtData = (BM_BufferPoolData *)calloc(1, sizeof(BM_BufferPoolData));

	BM_BufferPoolData* bufferPoolData = (BM_BufferPoolData *)bm->mgmtData;
	bufferPoolData->pageNumberList = (int *)malloc(numPages * sizeof(int));
	bufferPoolData->fixCounts = (int *)calloc(numPages, sizeof(int));
	bufferPoolData->clockFlag = (int *)calloc(numPages * PAGE_SIZE, sizeof(int));
	bufferPoolData->numberOfAvailablePageFrames = numPages;
	bufferPoolData->poolData = (char *)calloc(numPages * PAGE_SIZE, sizeof(char));
	bufferPoolData->dirtyFlags = (bool *)malloc(numPages * sizeof(bool));
	bufferPoolData->head = NULL;
	bufferPoolData->curToBeReplaced = NULL;
	bufferPoolData->tail = NULL;
	bufferPoolData->numberOfReadOperations = 0;
	bufferPoolData->numberOfWriteOperations = 0;

	int index = 0;
	while (index < numPages)
	{
		bufferPoolData->pageNumberList[index] = NO_PAGE;
		bufferPoolData->dirtyFlags[index] = false;
		index++;
	}

	return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool*const bm)
{
	/*
	* Check if all the pages in the pool have fix count = 0 (no client has pinned any of the pages in the buffer pool)
	* If any of the pages in the buffer pool is still pinned by a client(fixCount > 0), do not perform shutdown
	* If no page is pinned by any of the client (all pages have fixCount = 0), check if any of the pages is marked dirty and write them to the disk
	* release all buffer pool resources
	*/

	BM_BufferPoolData* bufferPoolData = (BM_BufferPoolData *)bm->mgmtData;
	int index = 0;
	while (index < bm->numPages)
	{
		if (bufferPoolData->fixCounts[index] > 0)
		{
			return RC_FAIL_SHUTDOWN_POOL;
		}
		index++;
	}
	forceFlushPool(bm);

	free(bufferPoolData->pageNumberList);
	free(bufferPoolData->fixCounts);
	free(bufferPoolData->clockFlag);
	free(bufferPoolData->poolData);
	free(bufferPoolData->dirtyFlags);

	bufferPoolData->pageNumberList = NULL;
	bufferPoolData->fixCounts = NULL;
	bufferPoolData->clockFlag = NULL;
	bufferPoolData->poolData = NULL;
	bufferPoolData->dirtyFlags = NULL;

	bufferPoolData->head = NULL;
	bufferPoolData->curToBeReplaced = NULL;
	bufferPoolData->tail = NULL;

	bufferPoolData->numberOfReadOperations = 0;
	bufferPoolData->numberOfWriteOperations = 0;

	free(bm->mgmtData);
	bm->mgmtData = NULL;

	return RC_OK;
}

RC forceFlushPool(BM_BufferPool*const bm)
{
	/*
	* Open the page file
	* Check if the buffer pool associated with the page file contains any page frames which are marked dirty
	* If any, write the pages to the disk and update the dirty flag associated with the page
	* Close the page file
	*/

	SM_FileHandle fileHandle;
	int status = openPageFile(bm->pageFile, &fileHandle);
	if (status != RC_OK)
	{
		return RC_FILE_NOT_FOUND;
	}

	BM_BufferPoolData* bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	PageNumber pageNumber = 0;
	while (pageNumber < bm->numPages)
	{
		if (bufferPoolData->dirtyFlags[pageNumber] == true && bufferPoolData->fixCounts[pageNumber] == 0)
		{
			SM_PageHandle pageHandle = bufferPoolData->poolData + pageNumber*PAGE_SIZE * sizeof(char);
			int actualPageIndex = bufferPoolData->pageNumberList[pageNumber];
			writeBlock(actualPageIndex, &fileHandle, pageHandle);
			bufferPoolData->dirtyFlags[pageNumber] = false;
			bufferPoolData->numberOfWriteOperations++;
		}
		pageNumber++;
	}

	closePageFile(&fileHandle);
	return RC_OK;
}

RC markDirty(BM_BufferPool*const bm, BM_PageHandle*const page)
{
	PageNumber actualPageNumber = page->pageNum;
	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	BM_TraverseList *currentPageInBufferPool = bufferPoolData->head;
	PageNumber bufferPoolPageNumber = NO_PAGE;

	int count = 0;
	int filledSlots = bm->numPages - bufferPoolData->numberOfAvailablePageFrames;

	while (currentPageInBufferPool != NULL && count < filledSlots)
	{
		if (currentPageInBufferPool->pageIndex == actualPageNumber)
		{
			bufferPoolPageNumber = currentPageInBufferPool->poolIndex;
			break;
		}
		currentPageInBufferPool = currentPageInBufferPool->next;
		count++;
	}

	if (bufferPoolPageNumber == NO_PAGE)
	{
		return RC_PAGE_NOT_FOUND_IN_CACHE;
	}

	bufferPoolData->dirtyFlags[bufferPoolPageNumber] = true;
	return RC_OK;
}

RC unpinPage(BM_BufferPool*const bm, BM_PageHandle*const page)
{
	PageNumber actualPageNumber = page->pageNum;
	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	BM_TraverseList *currentPageInBufferPool = bufferPoolData->head;
	PageNumber bufferPoolPageNumber = NO_PAGE;

	int count = 0;
	int filledSlots = bm->numPages - bufferPoolData->numberOfAvailablePageFrames;

	while (currentPageInBufferPool != NULL  && count < filledSlots)
	{
		if (currentPageInBufferPool->pageIndex == actualPageNumber)
		{
			bufferPoolPageNumber = currentPageInBufferPool->poolIndex;
			break;
		}
		currentPageInBufferPool = currentPageInBufferPool->next;
		count++;
	}

	if (bufferPoolPageNumber == NO_PAGE)
	{
		return RC_PAGE_NOT_FOUND_IN_CACHE;
	}
	bufferPoolData->fixCounts[bufferPoolPageNumber]--;

	return RC_OK;
}

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	PageNumber actualPageNumber = page->pageNum;
	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	BM_TraverseList *currentPageInBufferPool = bufferPoolData->head;
	PageNumber bufferPoolPageNumber = NO_PAGE;

	int count = 0;
	int filledSlots = bm->numPages - bufferPoolData->numberOfAvailablePageFrames;

	while (currentPageInBufferPool != NULL  && count < filledSlots)
	{
		if (currentPageInBufferPool->pageIndex == actualPageNumber)
		{
			bufferPoolPageNumber = currentPageInBufferPool->poolIndex;
			break;
		}
		currentPageInBufferPool = currentPageInBufferPool->next;
		count++;
	}

	if (bufferPoolPageNumber == NO_PAGE)
	{
		return RC_PAGE_NOT_FOUND_IN_CACHE;
	}

	SM_FileHandle fileHandle;
	int status = openPageFile(bm->pageFile, &fileHandle);
	if (status != RC_OK)
	{
		return RC_FILE_NOT_FOUND;
	}
	//SM_PageHandle pageHandle = bufferPoolData->poolData + bufferPoolPageNumber*PAGE_SIZE * sizeof(char);
	SM_PageHandle pageHandle = page->data;
	writeBlock(actualPageNumber, &fileHandle, pageHandle);
	closePageFile(&fileHandle);
	bufferPoolData->dirtyFlags[bufferPoolPageNumber] = false;
	bufferPoolData->numberOfWriteOperations++;

	return RC_OK;
}

PageNumber *getFrameContents(BM_BufferPool *const bm)
{
	/*
	* Get the buffer pool data from the buffer pool
	* return the page number list in the pool data
	*/

	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	return (PageNumber*)bufferPoolData->pageNumberList;
}

bool *getDirtyFlags(BM_BufferPool *const bm)
{
	/*
	* Get the buffer pool data from the buffer pool
	* return the dirty flags list in the pool data
	*/

	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	return bufferPoolData->dirtyFlags;
}

int *getFixCounts(BM_BufferPool *const bm)
{
	/*
	* Get the buffer pool data from the buffer pool
	* return the fix counts list in the pool data
	*/

	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	return bufferPoolData->fixCounts;
}

int getNumReadIO(BM_BufferPool *const bm)
{
	/*
	* Get the buffer pool data from the buffer pool
	* return the numberOfReadOperations from the pool data
	*/

	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	return bufferPoolData->numberOfReadOperations;
}

int getNumWriteIO(BM_BufferPool *const bm)
{
	/*
	* Get the buffer pool data from the buffer pool
	* return the numberOfWriteOperations from the pool data
	*/

	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData*)bm->mgmtData;
	return bufferPoolData->numberOfWriteOperations;
}

RC pinPage(BM_BufferPool*const bm, BM_PageHandle*const page,
	const PageNumber pageNum)
{
	/*
	* Check if the page file contains the requested page number.
	* If the page requested to be pinned is not present (last pageindex in the page file is less than the page number asked for tobe pinned), then
	* either throw an error or increase the capacity of the file to ensure requested page is pinned successfully
	*/

	SM_FileHandle fh;
	openPageFile(bm->pageFile, &fh);

	// since the page number starts from 0, last index of the page in the page file is (total number of pages - 1)
	if (fh.totalNumPages - 1 < pageNum)
	{
		ensureCapacity(pageNum + 1, &fh);
	}

	page->pageNum = pageNum;
	BM_BufferPoolData *bufferPoolData = (BM_BufferPoolData *)bm->mgmtData;
	BM_TraverseList* temp = bufferPoolData->head;
	bool requestedPageIsPresentIncache = false;
	PageNumber bufferPoolPageIndex = NO_PAGE;
	while (temp != NULL)
	{
		if (temp->pageIndex == pageNum)
		{
			requestedPageIsPresentIncache = true;
			bufferPoolPageIndex = temp->poolIndex;
			break;
		}
		temp = temp->next;
	}

	// Check if the page is already in the cache
	if (requestedPageIsPresentIncache == true)
	{
		if (bm->strategy == RS_LRU)
		{
			// move the page to the end of the queue
			pinPageInCacheForLRU(bm, page, pageNum);
		}

		page->data = bufferPoolData->poolData + bufferPoolPageIndex * PAGE_SIZE * sizeof(char);
		bufferPoolData->fixCounts[bufferPoolPageIndex]++;
		bufferPoolData->clockFlag[bufferPoolPageIndex] = 1;
		return RC_OK;
	}

	if (bufferPoolData->numberOfAvailablePageFrames > 0)
	{
		BM_TraverseList* handle = ((BM_TraverseList *)malloc(sizeof(BM_TraverseList)));

		bufferPoolPageIndex = bm->numPages - bufferPoolData->numberOfAvailablePageFrames;
		handle->poolIndex = bufferPoolPageIndex;
		handle->pageIndex = pageNum;
		handle->next = NULL;

		if (bufferPoolData->head == NULL)
		{
			bufferPoolData->head = handle;
			bufferPoolData->tail = handle;
			handle->prev = NULL;
			handle->next = NULL;
		}
		else
		{
			bufferPoolData->tail->next = handle;
			handle->prev = bufferPoolData->tail;
			bufferPoolData->tail = handle;
		}

		page->data = bufferPoolData->poolData + bufferPoolPageIndex * PAGE_SIZE * sizeof(char);
		bufferPoolData->pageNumberList[bufferPoolPageIndex] = pageNum;
		bufferPoolData->numberOfAvailablePageFrames--;
		bufferPoolData->fixCounts[bufferPoolPageIndex]++;
	}
	else
	{
		if (bm->strategy == RS_FIFO || bm->strategy == RS_LRU)
		{
			bufferPoolPageIndex = pinPageForFIFOLRU(bm, page, pageNum, &fh);	
		}
		else if (bm->strategy == RS_CLOCK)
		{
			bufferPoolPageIndex = pinPageForCLOCK(bm, page, pageNum);
		}

		bufferPoolData->fixCounts[bufferPoolPageIndex]++;
		page->data = bufferPoolData->poolData + bufferPoolPageIndex * PAGE_SIZE * sizeof(char);
	}

	readBlock(page->pageNum, &fh, page->data);
	bufferPoolData->numberOfReadOperations++;

	closePageFile(&fh);
	return RC_OK;
}

