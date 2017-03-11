#ifndef REPLACEMENT_STRATEGIES_H
#define REPLACEMENT_STRATEGIES_H

PageNumber pinPageForFIFOLRU(BM_BufferPool*const bm, BM_PageHandle*const page, const PageNumber pageNum, SM_FileHandle *fileHandle);
void pinPageInCacheForLRU(BM_BufferPool*const bm, BM_PageHandle*const page, const PageNumber pageNum);
PageNumber pinPageForCLOCK(BM_BufferPool*const bm, BM_PageHandle*const page, const PageNumber pageNum);

#endif
