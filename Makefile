.PHONY: default clean

default: lmmish

clean:
	rm lmmish

lmmish: lmmish.c
	gcc -Wall -o lmmish lmmish.c
