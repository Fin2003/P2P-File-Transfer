CC=gcc
CFLAGS=-Wall -Wvla -std=c2x -g -fsanitize=address
LDFLAGS=-lm -lpthread -fsanitize=address
INCLUDE=-Iinclude

.PHONY: all valgrind

all: pkgchk.o pkgmain btide p1tests p2tests clean
# Required for Part 1 - Make sure it outputs a .o file

pkgchk.o: src/chk/pkgchk.c src/tree/merkletree.c src/crypt/sha256.c
	$(CC) -c $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS)

pkgmain: src/pkgmain.c src/chk/pkgchk.c src/tree/merkletree.c src/crypt/sha256.c
	$(CC) $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o $@

pkgchecker: src/pkgmain.c src/chk/pkgchk.c src/tree/merkletree.c src/crypt/sha256.c
	$(CC) $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o $@

# Required for Part 2 - Make sure it outputs `btide` file

btide: src/btide.c src/chk/pkgchk.c src/tree/merkletree.c src/crypt/sha256.c #package
	$(CC) src/btide.c src/chk/pkgchk.c src/tree/merkletree.c src/crypt/sha256.c $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o $@

p1tests:
	bash p1test.sh

p2tests:
	bash p2test.sh

clean:
	rm -f pkgchk.o pkgmain btide merkletree.o sha256.o package 
	rm -rf tests

# Valgrind target for btide and its dependency package
valgrind-btide: btide
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./btide

# Optionally, if you want to run Valgrind on package as well
valgrind-package: package
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./package