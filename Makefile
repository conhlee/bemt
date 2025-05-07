PKGCONFIG_ZSTD ?= 0

CC = gcc
CFLAGS += $(shell pkg-config --cflags zlib opus) -Wall -Wpedantic -O3 -Wno-zero-length-array
LDFLAGS += $(shell pkg-config --libs zlib opus)
TARGET = bemt
SOURCES = \
	cons/buffer.c cons/comp.c cons/error.c cons/file.c cons/linklist.c cons/list.c cons/ptrie.c \
	tex/bcn.c tex/tegraSwizzle.c \
	stb/stb_image_write_impl.c \
	process/nnBin.c process/beaProcess.c process/bntxProcess.c \
	main.c
HEADERS = \
	cons/cons.h cons/buffer.h cons/comp.h cons/error.h cons/file.h cons/linklist.h cons/list.h \
	cons/macro.h cons/ptrie.h cons/type.h \
	tex/bcn.h tex/tegraSwizzle.h \
	stb/stb_image_write.h \
	process/nnBin.h process/beaProcess.h

# lua stuff
CFLAGS += -Ilua/lib/src

SOURCES += $(wildcard lua/lib/src/*.c)
SOURCES := $(filter-out lua/lib/src/luac.c lua/lib/src/lua.c, $(SOURCES))

HEADERS += $(wildcard lua/lib/src/*.h)

OBJECTS = $(SOURCES:.c=.o)

ifneq ($(PKGCONFIG_ZSTD),0)
CFLAGS += $(shell pkg-config --cflags zstd)
LDFLAGS += $(shell pkg-config --libs zstd)
else
LDFLAGS += -lzstd
endif

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
