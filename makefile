.PHONY : clean
luke.o :  luke.cpp
	gcc -c *.cpp -I ../ZLTCPTransfer/src
clean :
	-rm *.o
