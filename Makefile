CC = g++
CFLAGS = -I/usr/include/libxml2 -fopenmp -g -DDEBUG
LIBS = -lxml2 -lpcre
MODULES = allocate_memory.o scc.o visualization.o allocate_stes.o parse_anml.o partition.o list.o do_next.o do_gates.o

all: nfatool

nfatool: main.cpp nfatool.h $(MODULES)
	$(CC) $(CFLAGS) $< $(LIBS) $(MODULES) -o nfatool $(LIBS)

.PHONY: clean

%.o: %.cpp %.h nfatool.h
	$(CC) $(CFLAGS) -c $(LIBS) $< -o $@
	
clean:
	-rm *.o
