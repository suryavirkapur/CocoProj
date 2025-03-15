CC = gcc
CFLAGS = -Wall -g

stage1exe: driver.o lexer.o dataStructures.o grammar.o firstFollow.o parse.o utils.o
	$(CC) $(CFLAGS) -o stage1exe driver.o lexer.o dataStructures.o grammar.o firstFollow.o parse.o utils.o

driver.o: driver.c parser.h lexer.h constants.h
	$(CC) $(CFLAGS) -c driver.c

lexer.o: lexer.c lexer.h lexerDef.h constants.h utils.h
	$(CC) $(CFLAGS) -c lexer.c

dataStructures.o: dataStructures.c dataStructures.h grammar.h constants.h
	$(CC) $(CFLAGS) -c dataStructures.c

grammar.o: grammar.c grammar.h dataStructures.h constants.h
	$(CC) $(CFLAGS) -c grammar.c

firstFollow.o: firstFollow.c firstFollow.h grammar.h dataStructures.h constants.h
	$(CC) $(CFLAGS) -c firstFollow.c

parse.o: parse.c parse.h firstFollow.h grammar.h dataStructures.h lexer.h constants.h
	$(CC) $(CFLAGS) -c parse.c

utils.o: utils.c utils.h constants.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o stage1exe

cleanall:
	rm -f *.o stage1exe
	rm -f *.txt