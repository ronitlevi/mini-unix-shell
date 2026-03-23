all: myshell

myshell: myshell.o lineParser.o
	gcc -Wall -g -o myshell myshell.o lineParser.o

myshell.o: myshell.c lineParser.h
	gcc -Wall -g -c myshell.c

lineParser.o: lineParser.c lineParser.h
	gcc -Wall -g -c lineParser.c

mypipeline.o: mypipeline.c
	gcc -Wall -g -c mypipeline.c

looper.o: looper.c
	gcc -Wall -g -c looper.c

clean:
	rm -f myshell myshell.o lineParser lineParser.o looper looper.o mypipeline mypipeline.o
