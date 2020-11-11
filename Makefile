AR = ar
ARFLAGS = rcs
CC = gcc
CFLAGS = -Izlib -W -Wno-implicit-fallthrough -O2 -fPIC
INC = libpng/libpng.so -lz -lm
LIBS = -Llibpng
MAIN_FLAGS = -w -g -Ilibpng -Izlib
PROGRAM = glitch_png

LIBPNG_DIR = libpng
OBJ_DIR = obj

LIBPNG_DEPS := libpng/png.o libpng/pngerror.o libpng/pngget.o
LIBPNG_DEPS += libpng/pngmem.o libpng/pngpread.o libpng/pngread.o
LIBPNG_DEPS += libpng/pngrio.o libpng/pngrtran.o libpng/pngrutil.o
LIBPNG_DEPS += libpng/pngset.o libpng/pngtrans.o libpng/pngwio.o
LIBPNG_DEPS += libpng/pngwrite.o libpng/pngwtran.o libpng/pngwutil.o


all: pre_build main

pre_build:
	test -d bin || mkdir -p bin
	test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)

main: $(LIBPNG_DIR)/libpng.so $(OBJ_DIR)/main.o
	$(CC) $(LIBS) -o $(PROGRAM) $(OBJ_DIR)/main.o $(INC)

$(OBJ_DIR)/main.o: main.c
	$(CC) $(MAIN_FLAGS) -c $< -o $@

$(LIBPNG_DIR)/libpng.so: $(LIBPNG_DEPS)
	$(CC) -lz -lm -shared -o $@ -fPIC $^

clean:
	rm -f $(PROGRAM)
	rm -rf bin
	rm -rf $(OBJ_DIR)
	rm -f $(LIBPNG_DIR)/*.o
	rm -f $(LIBPNG_DIR)/*.so


