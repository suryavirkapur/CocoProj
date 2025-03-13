CC = gcc
CFLAGS = -Wall -g

all: compiler

compiler: driver.o lexer.o parser.o
	$(CC) $(CFLAGS) -o compiler driver.o lexer.o parser.o

driver.o: driver.c lexer.h parser.h
	$(CC) $(CFLAGS) -c driver.c

lexer.o: lexer.c lexer.h lexerDef.h
	$(CC) $(CFLAGS) -c lexer.c

parser.o: parser.c parser.h parserDef.h lexer.h
	$(CC) $(CFLAGS) -c parser.c

clean:
	rm -f *.o compiler