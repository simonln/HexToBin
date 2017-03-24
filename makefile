CC=gcc

test : 
	$(CC) main.c -o test

clean : 
	rm *.o test.exe