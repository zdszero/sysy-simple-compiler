CC = gcc
CCFLAGS = 
OBJS = main.o parse.o scan.o symtab.o util.o codegen.o

cm: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CCFLAGS)
main.o: globals.h util.h
scan.o: scan.c
parse.o: parse.c
symtab.o: globals.h symtab.h
util.o: globals.h symtab.h util.h
codegen.o: globals.h symtab.h codegen.h
scan.c: scan.l
	flex -o scan.c scan.l
parse.c: parse.y globals.h scan.h symtab.h util.h
	bison -o parse.c parse.y -d

.PHONY: test clean
test:
	./tryall
clean:
	-rm *.o *.s cm out
