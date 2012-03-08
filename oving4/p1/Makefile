src=foo.s
program: $(src)
	as $(src) -o program.o --32
	gcc -m32 program.o -o program 
	rm -f program.o
clean:
	rm -f program program.o
