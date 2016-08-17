
BIN = gtk3curve-sample
LIBS = -lm
INCLUDES = -I.

GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LDFLAGS = `pkg-config --libs gtk+-3.0`

CC = gcc
CFLAGS = -g -DDEBUG $(INCLUDES) $(GTK_CFLAGS)

.SUFFIXES: .c

.c.o:
	$(CC) $(CFLAGS) -c $<

.c :
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

SRC = sample.c gtk3curve.c
OBJ = $(addsuffix .o, $(basename $(SRC)))

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(GTK_LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJ) $(BIN)
