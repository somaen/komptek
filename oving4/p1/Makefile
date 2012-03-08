src=foo.s
program: $(src)
	as $(src) -g -o program.o --32
	gcc -g -m32 program.o -o program 
	rm -f program.o
clean:
	rm -f program program.o
