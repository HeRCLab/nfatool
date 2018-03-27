CC = g++
CFLAGS = -I/usr/include/libxml2 -fopenmp -g
LIBS = -lxml2 -lpcre
MODULES = allocate_memory.o scc.o visualization.o allocate_stes.o parse_anml.o partition.o

all: nfatool

allocate_memory.o: allocate_memory.cpp allocate_memory.h
	$(CC) $(CFLAGS) -c $<

scc.o: scc.cpp scc.h
	$(CC) $(CFLAGS) -c $<

visualization.o: visualization.cpp visualization.h
	$(CC) $(CFLAGS) -c $<

allocate_stes.o: allocate_stes.cpp allocate_stes.h
	$(CC) $(CFLAGS) -c $<

parse_anml.o: parse_anml.cpp parse_anml.h
	$(CC) $(CFLAGS) -c $<

partition.o: partition.cpp partition.h
	$(CC) $(CFLAGS) -c $<

nfatool: main.cpp nfatool.h allocate_memory.o scc.o visualization.o allocate_stes.o parse_anml.o partition.o 
	$(CC) $(CFLAGS) $< $(LIBS) $(MODULES) -o nfatool $(LIBS)

clean:
	rm *.o
