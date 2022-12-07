
.phony all:
all: sfs

sfs:
	gcc -Wall diskinfo.c -o diskinfo
	gcc -Wall disklist.c -o disklist
	gcc -Wall diskget.c -o diskget
	gcc -Wall diskput.c -o diskput

.PHONY clean:
clean:
	-rm -rf *.o *.exe