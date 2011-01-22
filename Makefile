gncci.so: gncci.c
	gcc -shared -ldl -fPIC gncci.c -o gncci.so
clean:
	rm -f gncci.so
