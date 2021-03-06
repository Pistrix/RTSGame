APPNAME=run

TARGET_OS ?= OS_WINDOWS

ifeq ($(TARGET_OS), OS_WINDOWS)

	LDFLAGS:=-static -L C:/raylib/src -lm -lraylib  -pthread -lopengl32 -lgdi32 -lwinmm -lstdc++

	CFLAGS:= -g -Wfatal-errors -pedantic -Wall -Wextra
	CFLAGS+= -std=c++17 -I ./include -I C:/raylib/src

	SRC:=$(wildcard src/*.cpp)
	OBJ:=$(SRC:src/%.cpp=obj/%.o)
	INC:=$(wildcard include/*.h)
endif
ifeq ($(TARGET_OS),OS_LINUX)

	LDFLAGS:=-L ~/raylib/src -lm -lraylib -lX11 -ldl -pthread -lstdc++fs

	CFLAGS:= -g -Wfatal-errors -pedantic -Wall -Wextra -std=c++17
	CFLAGS+= -I ./include -I ~/raylib/src

	SRC:=$(wildcard src/*.cpp src/*/*.cpp)
	OBJ:=$(SRC:src/%.cpp=obj/%.o)
	INC:=$(wildcard include/*.h)

endif

CC=g++

$(APPNAME): $(OBJ)
	$(CC) $(OBJ) -o $(APPNAME) $(LDFLAGS)

$(OBJ): obj/%.o : src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: debug release
debug: CFLAGS+= -g
release: CFLAGS+= -O3

debug release: clean $(APPNAME)

.PHONY:	clean
clean:
	rm obj\* -f
	rm $(APPNAME) -f

style: $(SRC) $(INC)
	astyle -A10 -s4 -S -p -xg -j -z2 -n src/* include/*
