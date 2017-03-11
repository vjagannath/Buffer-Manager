# Buffer-Manager
Implementation of a buffer manager managing a cache of pages in memory including reading/flushing to disk and page replacement by FIFO and LRU algorithms

The buffer manager manages a fixed number of pages in memory that represent pages from a page file managed by the storage manager.
It manages a buffer of blocks in memory including reading/flushing to disk and block replacement.
The FIFO,LRU, and Clock replacement strategies have been implemented for the buffer manager.

How To Compile and Run(Makefile operations)
****************************************************
==> Enter 'make' command in the terminal (To build and create test_assign2_1 and test_assign2_2 binary files).
==> Enter './test_assign2_1' command in the terminal (To run the test_assign2_1.c test case file).
==> Enter './test_assign2_2' command in the terminal (To run the test_assign2_2.c test case file).

___________________________________________________________________________________________
Please Note: Additional test cases for the CLOCK replacement strategy has been added to the test_assign2_2.c file.
___________________________________________________________________________________________

Implementation Details
**************************

1.initBufferPool :
---------------------
==>Creates and initializes the buffer pool for the given page file.
==>The pool is used to cache pages from the page file.


2.shutdownBufferPool : 
-----------------------------
==> It is mainly used to destroy the buffer pool.
==> It frees up all the resources associated with the buffer pool.
==> It should also free the memory allocated for page frames.
==> The dirty pages should be written back to disk before destroying the pool.
==> If any page is pinned, it should not be shutdown.



3.forceFlushPool :
----------------------
==>It is mainly used for writting all the dirty unpinned pages with a fix count of 0, from the buffer pool to the disk.

4.pinPage:
-------------
==>The pages are mapped from disk to main memory using this method.
==>In an event of full buffer pool, replacement strategies like FIFO, LRU and CLOCK are used to replace the page.



5.unpinPage:
--------------- 
==>The page with the given page number is unpinned from the buffer pool.



6.markDirty:
---------------
==>It is used to mark the pages as dirty. 

7.forcePage:
-----------------
==>Write the current content of the page to the page file on disk.


8.getFrameContents:
---------------------------
==>It returns an array of page numbers in the buffer pool.

9.getDirtyFlags:
--------------------
==>It returns an array of bools associated with each of the page in the buffer pool representing if the page is marked dirty.

10.getFixCounts:
---------------------
==>It returns an array depicting the fix counts associated with the pages present in the buffer pool.

11.getNumReadIO:
-----------------------
==>It returns the number of pages that have been read from disk since a buffer pool has been initialized.

12.getNumWriteIO:
------------------------
==>It returns the number of pages that have been written to the disk since a buffer pool has been initialized.


Data Structures
******************


1.BM_BufferPoolData:
-------------------------
==> Data structure to main buffer pool contents


2.BM_TraverseList :
-------------------------
==> Data structure used to maintain page identifications present in the buffer pool
