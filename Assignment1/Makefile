.phony all:
all: pman

pman: main.c linked_list.c
	gcc -Wall main.c linked_list.c -o pman
	gcc -Wall test.c -o test
	gcc -Wall inf.c -o inf

.PHONY clean:
clean:
	-rm -rf *.o *.exe
