CC = cc
SAN_FLAGS= #-fsanitize=address -fsanitize-recover=address
CFLAGS=-Ilib -Ilib/raylib/src -Og -fno-omit-frame-pointer -g $(SAN_FLAGS)
LDFLAGS=-Llib/raylib/src $(SAN_FLAGS)

HEADERS =
OBJECTS = main.o

LIBS=-lraylib -ldl -lm -lpthread

# this is needed because emsdk_env.sh doesn't work with a standard
# posix shell
SHELL=bash

EMSDK_PATH ?= ~/source/emsdk

WEB_BUILD_DIR ?= temp-web
WEB_OUTPUT_DIR ?= web

all: life

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

life: $(OBJECTS) raylib
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@

raylib: lib/raylib/src
	make -C lib/raylib/src

web: $(WEB_OUTPUT_DIR)/life.html

$(WEB_OUTPUT_DIR)/life.html: main.c shell.html
	mkdir -p $(WEB_BUILD_DIR)
	. $(EMSDK_PATH)/emsdk_env.sh && $(MAKE) -C $(WEB_BUILD_DIR) -f ../lib/Makefile.raylib.web RAYLIB_SRC_PATH=../lib/raylib/src
	mkdir -p $(WEB_OUTPUT_DIR)
	. $(EMSDK_PATH)/emsdk_env.sh && emcc -o $(WEB_OUTPUT_DIR)/life.html main.c -Os -Wall -lraylib -I. -Ilib -Ilib/raylib/src -L$(WEB_BUILD_DIR) -s USE_GLFW=3 -DPLATFORM_WEB -s EXPORTED_RUNTIME_METHODS=[FS,ccall] --shell-file shell.html

clean:
	-rm -f $(OBJECTS)
	-rm -f life
	-rm -rf $(WEB_BUILD_DIR)
	-rm -rf $(WEB_OUTPUT_DIR)

distclean: clean
	-make -C lib/raylib/src

.PHONY: clean all