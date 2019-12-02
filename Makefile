CC=gcc

gf_mds: src/main.c src/dataset.c src/common.c
	$(CC) -o gf_mds src/main.c src/dataset.c src/common.c -O2 -lm

clean:
	rm -f src/*.o
	rm -f gf_mds


