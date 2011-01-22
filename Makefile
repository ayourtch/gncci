gncci.so: gncci.c
	gcc -shared -ldl -fPIC -I/usr/include/lua5.1 -llua5.1 gncci.c -o gncci.so 
clean:
	rm -f gncci.so
