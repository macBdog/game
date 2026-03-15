CC=gcc
CFLAGS=-c -Wall -I.
INC=-Icore -Iengine -Ithird_party/SDL2-2.0.7/include -Ithird_party/glew-2.1.0 -Ithird_party/irrKlang-1.5.0 -Ithird_party/lua-5.2.3 -Ithird_party/remotery/lib -Ithird_party/zlib-1.2.8/include

default: game

game: game.cpp
	$(CC) $(CFLAGS) $(INC) -c game.cpp

.PHONY: clean

clean: rm -r obj/*.o
