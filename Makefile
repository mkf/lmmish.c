.PHONY: clean

clean:
	rm lmmish

default: lmmish

lmmish: lmmish.c
	gcc -Wall -o lmmish lmmish.c
