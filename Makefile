EXECUTABLE=wallpaper-shuffle.exe

CC=gcc -Os -flto -Wall -std=c11 -pedantic-errors -mrdrnd -mwindows
LDFLAGS=-lcomctl32 -lole32

src = $(wildcard *.c)
obj = $(src:.c=.o)

all: resource.o myprog

resource.o: resource.rc
	windres resource.rc -O coff -o resource.o

myprog: $(obj)
	$(CC) -o $(EXECUTABLE) $^ $(LDFLAGS) resource.o
	strip $(EXECUTABLE)

.PHONY: clean
clean:
	rm $(obj) $(EXECUTABLE) resource.o