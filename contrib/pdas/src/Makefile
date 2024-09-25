
all:
	cc i386_gen.c -o a.exe
	cp i386_opc.tbl i386_t.h
	cc -E i386_t.h -o i386_opc.i 
	./a.exe i386_opc.i i386_tbl.h
	cc -o pdas.exe -Ihashtab \
hashtab/hashtab.c \
a_out.c \
as.c \
bytearray.c \
cfi.c \
coff.c \
cond.c \
cstr.c \
elf.c \
error.c \
expr.c \
frags.c \
i386_as.c \
int64sup.c \
libas.c \
listing.c \
load_line.c \
process.c \
sections.c \
symbols.c \
write.c


clean:
	rm -f *.exe *.i i386_t.h


