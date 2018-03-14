CC = g++
CFLAGS = -I/usr/include/libxml2 -fopenmp -g
LIBS = -lxml2 -lpcre

nfatool: ANMLFarm_final.cpp
	$(CC) $(CFLAGS) $< $(LIBS) -o nfatool

all: nfatool

clean:
	rm *.o
