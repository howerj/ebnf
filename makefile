CFLAGS=-Wall -Wextra -pedantic -std=c99 -fwrapv

.PHONY: all clean

all: vm

doc: doc.tgz
	tar zcf $<

clean:
	rm -fv *.a *.o vm

