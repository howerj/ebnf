CFLAGS=-Wall -Wextra -pedantic -std=c99 -fwrapv

.PHONY: all clean

all: vm

doc: doc.tgz
	tar zxf $<

clean:
	git clean -dfx

