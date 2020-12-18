CC = g++
CFLAGS = -I/usr/include/libxml2 -fopenmp -fPIC
LIBS = -lxml2 -lpcre
OBJS = obj/allocate_memory.o obj/edge.o obj/scc.o obj/visualization.o obj/allocate_stes.o obj/parse_anml.o obj/partition.o obj/list.o obj/do_next.o obj/do_gates.o
DEBUG_OBJS = $(shell echo "$(OBJS)" | tr ' ' '\n' | sed 's/^obj/obj\/dbg/g' | tr '\n' ' ')
RELEASE_OBJS = $(shell echo "$(OBJS)" | tr ' ' '\n' | sed 's/^obj/obj\/rel/g' | tr '\n' ' ')

all: debug

release: src/main.cpp src/nfatool.h $(RELEASE_OBJS)
	$(CC) -O3 $(CFLAGS) $< $(LIBS) $(RELEASE_OBJS) -o nfatool $(LIBS)

debug: src/main.cpp src/nfatool.h $(DEBUG_OBJS)
	$(CC) -g -DDEBUG $(CFLAGS) $< $(LIBS) $(DEBUG_OBJS) -o nfatool $(LIBS)

.PHONY: clean

obj/dbg/%.o: src/%.cpp src/%.h src/nfatool.h
	$(CC) -g -DDEBUG $(CFLAGS) -c $(LIBS) $< -o $@

obj/rel/%.o: src/%.cpp src/%.h src/nfatool.h
	$(CC) -O3 $(CFLAGS) -c $(LIBS) $< -o $@

clean:
	-rm obj/dbg/*.o obj/rel/*.o nfatool
