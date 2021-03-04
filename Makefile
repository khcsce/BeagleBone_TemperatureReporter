# NAME: Khoa Quach
# EMAIL: khoaquachschool@gmail.com
# ID: 105123806
CC = gcc
CFLAGS = -Wall -Wextra
.SILENT:

default:
	$(CC) $(CFLAGS) -o lab4b -lm -lmraa lab4b.c
clean:
	rm -f lab4b *.tar.gz *.o *~ log.txt input.txt

check: default
	rm -f input.txt
	touch input.txt
	chmod +x input.txt
	echo "SCALE=C" >> input.txt; \
	echo "STOP" >> input.txt; \
	echo "SCALE=F" >>input.txt; \
	echo "PERIOD=5" >> input.txt; \
	echo "START" >> input.txt; \
	echo "OFF" >> input.txt; \
	(cat input.txt | ./lab4b --log=log.txt && echo "Passed YOUR check") || (echo "Failed YOUR check" && exit 1); \

dist: 
	tar -czvf lab4b-105123806.tar.gz lab4b.c Makefile README