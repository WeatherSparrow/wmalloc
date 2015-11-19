WMOBJS = wmalloc.cpp
CC = g++

wmtest: wmtest.cpp $(WMOBJS)
	$(CC) -Wall -O2 -std=c++11 -o wmtest wmtest.cpp $(WMOBJS) -lpthread
wmdebug: wmtest.cpp $(WMOBJS)
	$(CC) -g -O0 -std=c++11 -o wmdebug wmtest.cpp $(WMOBJS) -lpthread
