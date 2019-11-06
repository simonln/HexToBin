CC = gcc

HexToBin : main.c
	$(CC) $^ -o $@

clean : 
	rm  HexToBin

