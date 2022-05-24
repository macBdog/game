CC=gcc
CFLAGS=-c -Wall -I.
INC=-Icore -Iengine -Iexternal/SDL2-2.0.7/include -Iexternal/glew-2.1.0 -Iexternal/irrKlang-1.5.0 -Iexternal/lua-5.2.3 -Iexternal/remotery/lib -Iexternal/zlib-1.2.8/include

default: game

game: game.cpp
	$(CC) $(CFLAGS) $(INC) -c game.cpp

.PHONY: clean

clean: rm -r obj/*.o
