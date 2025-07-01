#----------------------------------------------------------------------------
# Makefile – Assemble payload ASM & compile static injector (Linux + mingw‑w64)
#----------------------------------------------------------------------------
NASM      = nasm
CC        = x86_64-w64-mingw32-g++
CFLAGS    = -O2 -static -s
LDFLAGS   =

all: injector.exe

payload.bin: payload.asm
	$(NASM) -f bin -o $@ $<

injector.exe: injector.cpp payload.bin
	$(CC) $(CFLAGS) -o $@ injector.cpp $(LDFLAGS)

clean:
	rm -f injector.exe payload.bin
