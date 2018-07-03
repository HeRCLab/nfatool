CC = g++
CFLAGS = -I/usr/include/libxml2 -fopenmp -g -DDEBUG
LIBS = -lxml2 -lpcre
MODULES = allocate_memory.o scc.o visualization.o allocate_stes.o parse_anml.o partition.o list.o  do_next.o do_gates.o

all: nfatool 

allocate_memory.o: allocate_memory.cpp allocate_memory.h nfatool.h
	$(CC) $(CFLAGS) -c $<

scc.o: scc.cpp scc.h nfatool.h
	$(CC) $(CFLAGS) -c $<

visualization.o: visualization.cpp visualization.h nfatool.h
	$(CC) $(CFLAGS) -c $<

allocate_stes.o: allocate_stes.cpp allocate_stes.h nfatool.h
	$(CC) $(CFLAGS) -c $<

parse_anml.o: parse_anml.cpp parse_anml.h nfatool.h
	$(CC) $(CFLAGS) -c $<

partition.o: partition.cpp partition.h nfatool.h
	$(CC) $(CFLAGS) -c $<


list.o:  list.cpp nfatool.h
	$(CC) $(CFLAGS) -c $<

testing.o: testing.cpp testing.h nfatoo.h 
	$(CC) $(CFLAGS) -c $< 

do_next.o: do_next.cpp do_next.h nfatool.h 
	$(CC) $(CFLAGS) -c $<

do_gates.o: do_gates.cpp do_gates.h nfatool.h 
	$(CC) $(CFLAGS) -c $<

nfatool: main.cpp nfatool.h $(MODULES)
	$(CC) $(CFLAGS) $< $(LIBS) $(MODULES) -o nfatool $(LIBS)

clean:
	rm *.o
