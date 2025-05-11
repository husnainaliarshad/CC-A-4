
CC = gcc
CFLAGS = -Wall -g
LEX = flex
YACC = bison

all: json2relcsv

json2relcsv: lex.yy.c parser.tab.c ast.c csv_generator.c
	$(CC) $(CFLAGS) -o json2relcsv lex.yy.c parser.tab.c ast.c csv_generator.c -lfl

lex.yy.c: scanner.l parser.tab.h
	$(LEX) scanner.l

parser.tab.c parser.tab.h: parser.y
	$(YACC) -d parser.y

clean:
	rm -f json2relcsv lex.yy.c parser.tab.c parser.tab.h *.csv
