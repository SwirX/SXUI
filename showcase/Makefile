CC = gcc
CFLAGS = -Wall -Wextra -I.. -I../deps/sxlist
LDFLAGS = ../libsxui.a -lSDL2 -lSDL2_ttf -lm

SRC = main.c pages/dashboard.c pages/editor.c pages/canvas_lab.c pages/paint.c pages/files.c pages/stress.c pages/gradient_lab.c pages/input_lab.c
OBJ = $(SRC:.c=.o)
TARGET = sxui_showcase

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
