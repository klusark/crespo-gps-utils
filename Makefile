all: gps-test-crespo

gps-test-crespo: bcm4751_test.c
	$(CC) -o $@ $^
clean:
	rm -f gps-test-crespo
