all: assignment2 assignment2_2
assignment2:
	gcc -o test_assign2_1 test_assign2_1.c buffer_mgr.c buffer_mgr_stat.c storage_mgr.c dberror.c replacementStrategies.c -I.
assignment2_2:
	 gcc -o test_assign2_2  test_assign2_2.c buffer_mgr.c storage_mgr.c buffer_mgr_stat.c dberror.c replacementStrategies.c -I.
clean:
	$(RM) test_assign2_1 test_assign2_2 testbuffer.bin

