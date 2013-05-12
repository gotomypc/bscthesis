CC=clang
LIB_OBJ=libnatpeer.o
BIN_OBJ=natpeer.o
BIN=natpeer
CFLAGS=-Wall -Werror -pedantic -std=c99 -g -fPIC
LDFLAGS=-ljansson -lpthread
STATIC_LIB=libnatpeer.a
SHARED_LIB=libnatpeer.so

all: $(BIN) $(SHARED_LIB)

$(BIN_OBJ): natpeer.c natpeer.h
	$(CC) -c natpeer.c $(CFLAGS)

$(BIN): $(STATIC_LIB) $(BIN_OBJ)
	$(CC) -o $(BIN) $(BIN_OBJ) $(STATIC_LIB) $(LDFLAGS)

$(LIB_OBJ): libnatpeer.c libnatpeer.h
	$(CC) -c libnatpeer.c $(CFLAGS)

$(STATIC_LIB): $(LIB_OBJ)
	ar cr $(STATIC_LIB) $(LIB_OBJ)

$(SHARED_LIB): $(LIB_OBJ)
	$(CC) -o $(SHARED_LIB) -shared $(LIB_OBJ)

clean:
	rm -f $(BIN) *.a *.so *.o
	rm -rf obj/ libs/

android:
	ndk-build
