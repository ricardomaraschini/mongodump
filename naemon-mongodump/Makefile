MONGOLIBS=$(shell pkg-config --cflags --libs libmongoc-1.0)
JSONLIBS=-ljansson

all:
	gcc -D_GNU_SOURCE -Wall -fPIC $(JSONLIBS) -shared -o mongodump.o src/utils.c src/mongodump.c $(MONGOLIBS)
