
INSTALL_DIR = /usr/local
LIB_DEST = $(INSTALL_DIR)/lib
BIN_DEST = $(INSTALL_DIR)/bin
INC_DEST = $(INSTALL_DIR)/include
PKG_DEST = $(LIB_DEST)/pkgconfig

LN = ln
AR = ar
CC = gcc
RANLIB = ranlib

BIN = gtk3curve-sample gtk3gammacurve-sample gtk3ruler-sample
LN_SHARED_LIB = libgtk3curve-1.0.so
LN_0_SHARED_LIB = $(LN_SHARED_LIB).0
SHARED_LIB = $(LN_0_SHARED_LIB).1.0
STATIC_LIB = libgtk3curve.a
LIBS = -lm
INCLUDES = -I.

GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LDFLAGS = `pkg-config --libs gtk+-3.0`

CFLAGS = -g -DDEBUG -fPIC $(INCLUDES) $(GTK_CFLAGS)

.SUFFIXES: .c

.c.o:
	$(CC) $(CFLAGS) -c $<

.c :
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

LIB_SRC = gtk3curve.c gtk3gamma.c Gtk3CurveResource.c gtk3ruler.c
LIB_OBJ = $(addsuffix .o, $(basename $(LIB_SRC)))
SRC = sample.c $(LIB_SRC)
APP_OBJ = $(addsuffix .o, $(basename $(SRC)))

all: $(BIN) lib_static lib_shared

$(BIN): $(APP_OBJ)
	$(CC) -o $@ $(APP_OBJ) $(GTK_LDFLAGS) $(LIBS)

lib_static: $(LIB_OBJ)
	$(AR) rc $(STATIC_LIB) $(LIB_OBJ)
	$(RANLIB) $(STATIC_LIB)

lib_shared: $(LIB_OBJ)
	$(CC) -shared -o $(SHARED_LIB) $(LIB_OBJ) $(GTK_LDFLAGS) $(LIBS)

clean:
	rm -f $(LIB_OBJ) $(APP_OBJ) $(BIN) *~ *.a *.so* *.la

install: lib_static lib_shared
	install -m 644 -D $(STATIC_LIB) $(LIB_DEST)/$(STATIC_LIB)
	install -m 755 -D $(SHARED_LIB) $(LIB_DEST)/$(SHARED_LIB)
	$(LN) -sf $(SHARED_LIB) $(LIB_DEST)/$(LN_SHARED_LIB)
	$(LN) -sf $(SHARED_LIB) $(LIB_DEST)/$(LN_0_SHARED_LIB)
	install -m 755 -D libgtk3curve.la $(LIB_DEST)/libgtk3curve.la
	install -m 644 -D gtk3curve.pc $(PKG_DEST)/gtk3curve.pc
	install -m 644 -D gtk3curve.h $(INC_DEST)/gtk3curve.h
	install -m 644 -D gtk3gamma.h $(INC_DEST)/gtk3gammacurve.h
	install -m 644 -D gtk3ruler.h $(INC_DEST)/gtk3ruler.h

uninstall:
	rm $(LIB_DEST)/$(STATIC_LIB)
	rm $(LIB_DEST)/$(SHARED_LIB)
	rm $(LIB_DEST)/$(LN_SHARED_LIB)
	rm $(LIB_DEST)/$(LN_0_SHARED_LIB)
	rm $(LIB_DEST)/libgtk3curve.la
	rm $(PKG_DEST)/gtk3curve.pc $(PKG_DEST)
	rm $(INC_DEST)/gtk3curve.h $(INC_DEST)
	rm $(INC_DEST)/gtk3gammacurve.h $(INC_DEST)
	rm $(INC_DEST)/gtk3ruler.h $(INC_DEST)
