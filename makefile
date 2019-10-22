main: main.o functions.o
	gcc -g main.o functions.o -o main

main.o: main.c
	gcc -c -g main.c

functions.o: functions.c myLib.h
	gcc -c -g functions.c

clean:
	rm -rf *.o main
