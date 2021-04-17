CC = gcc
CFLAGS = -g -Wall
OBJS = main.o parse.o scan.o analyze.o codegen.o util.o symtab.o 

cm: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)
main.o: globals.h parse.h util.h symtab.h
analyze.o: globals.h parse.h symtab.h analyze.h util.h
scan.o: scan.c
parse.o: parse.c
symtab.o: globals.h parse.h symtab.h util.h
util.o: globals.h parse.h symtab.h util.h
codegen.o: globals.h parse.h symtab.h codegen.h util.h
scan.c: scan.l globals.h parse.h scan.h util.h
	flex -o scan.c scan.l
parse.c: parse.y globals.h parse.h scan.h symtab.h util.h analyze.h
	bison -o parse.c parse.y -d

.PHONY: test clean
test:
	./tryall
clean:
	-rm *.o *.s cm out
