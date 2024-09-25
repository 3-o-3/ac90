all:
	cc -m32 -o pdld.exe -lm -Ibytearray -Iftebc -Ihashtab \
bytearray/bytearray.c \
ftebc/febc.c \
ftebc/tebc.c \
hashtab/hashtab.c \
aout.c \
aout_bytearray.c \
atari.c \
atari_bytearray.c \
cms.c \
coff.c \
coff_bytearray.c \
elf.c \
elf_bytearray.c \
error.c \
hunk.c \
int64sup.c \
ld.c \
libld.c \
link.c \
lx.c \
lx_bytearray.c \
mainframe.c \
map.c \
read.c \
sections.c \
symbols.c \
vse.c \
xmalloc.c
	cp pdld.exe ../../../bin/

clean:
	rm -f *.exe

