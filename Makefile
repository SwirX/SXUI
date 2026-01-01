CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -Ideps/sxlist
LDFLAGS = -lSDL2 -lSDL2_ttf -lm

# Dependencies
LIST_SRC = deps/sxlist/dynamic_list.c

# Library
LIB_NAME = libsxui.a
LIB_OBJ = sxui.o dynamic_list.o

.PHONY: all clean lib example showcase

all: showcase example

# Compile dependency
dynamic_list.o: $(LIST_SRC)
	$(CC) $(CFLAGS) -c $(LIST_SRC) -o dynamic_list.o

# Compile library core
sxui.o: sxui.c sxui.h
	$(CC) $(CFLAGS) -c sxui.c -o sxui.o

# Create static library
$(LIB_NAME): $(LIB_OBJ)
	ar rcs $(LIB_NAME) $(LIB_OBJ)

# Build Showcase binary
showcase_bin: showcase.c $(LIB_NAME)
	$(CC) $(CFLAGS) showcase.c $(LIB_NAME) -o showcase_bin $(LDFLAGS)

# Build Example binary
example: example.c $(LIB_NAME)
	$(CC) $(CFLAGS) example.c $(LIB_NAME) -o example $(LDFLAGS)

lib: $(LIB_NAME)

clean:
	rm -f *.o $(LIB_NAME) showcase_bin example