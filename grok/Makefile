all: json2relcsv

json2relcsv: scanner.o parser.o ast.o main.o
	gcc -o json2relcsv scanner.o parser.o ast.o main.o -lfl

scanner.o: scanner.c parser.tab.h
	gcc -c scanner.c

parser.o: parser.c ast.h
	gcc -c parser.c

ast.o: ast.c ast.h
	gcc -c ast.c

main.o: main.c ast.h
	gcc -c main.c

scanner.c: scanner.l
	flex -o scanner.c scanner.l

parser.c parser.tab.h: parser.y
	bison -d parser.y

clean:
	rm -f *.o scanner.c parser.c parser.tab.h json2relcsv