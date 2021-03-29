CC=gcc
CFLAGS=-Wall -s
SOURCES=src/util.c \
        src/fs/misc.c \
				src/aes.c \
				src/aes_xts.c \
				src/kgen.c \
				src/device.c \
				src/fs/ufs.c \
				src/fs/fat.c \
				src/main.c
EXECUTABLE=ps3_hdd_reader
all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(EXECUTABLE)
clean:
	rm -rf $(EXECUTABLE)
