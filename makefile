CC = gcc
CFLAGS = -Wall -g

all: compiler

compiler: driver.o lexer.o parser.o utils.o
	$(CC) $(CFLAGS) -o compiler driver.o lexer.o parser.o utils.o

driver.o: driver.c lexer.h parser.h constants.h
	$(CC) $(CFLAGS) -c driver.c

lexer.o: lexer.c lexer.h lexerDef.h constants.h utils.h
	$(CC) $(CFLAGS) -c lexer.c

parser.o: parser.c parser.h parserDef.h lexer.h constants.h utils.h
	$(CC) $(CFLAGS) -c parser.c

utils.o: utils.c utils.h constants.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o compiler