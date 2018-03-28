CC = g++
CFLAGS = -I/usr/include/libxml2 -fopenmp -g -DDEBUG
LIBS = -lxml2 -lpcre
MODULES = allocate_memory.o scc.o visualization.o allocate_stes.o parse_anml.o partition.o strong_components_analysis.o

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

strong_components_analysis.o: strong_components_analysis.cpp strong_components_analysis.h nfatool.h
	$(CC) $(CFLAGS) -c $<

nfatool: main.cpp nfatool.h allocate_memory.o scc.o visualization.o allocate_stes.o parse_anml.o partition.o strong_components_analysis.o
	$(CC) $(CFLAGS) $< $(LIBS) $(MODULES) -o nfatool $(LIBS)

clean:
	rm *.o
