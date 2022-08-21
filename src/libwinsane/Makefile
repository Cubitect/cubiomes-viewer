.POSIX:
#CROSS   = x86_64-w64-mingw32-
LD      = $(CROSS)ld
CC      = $(CROSS)gcc
WINDRES = $(CROSS)windres
CFLAGS  = -Os -g -Wall -Wextra

libwinsane.o: init.o manifest.o
	$(LD) -relocatable -o $@ init.o manifest.o

manifest.o: manifest.rc utf8.xml
	$(WINDRES) -o $@ manifest.rc

clean:
	rm -f init.o manifest.o libwinsane.o
