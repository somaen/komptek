VSLC=../bin/vslc
ASFLAGS=-32
LDFLAGS=-m32
SOURCES=$(shell ls *.vsl)
ASSEMBLY=$(subst .vsl,.s,${SOURCES})
TARGETS=$(subst .vsl,,${SOURCES})
all: ${TARGETS}
asm: ${ASSEMBLY}
test: all
	@echo "\n-- Testing 'Hello, world':"
	./hello
	@echo "\n-- Testing single features:"
	@echo "\tTree reduction"
	./simplify
	@echo "\tUnary minus operator"
	./uminus
	@echo "\tFunction calls"
	./funcall
	@echo "\n-- Testing string escape codes:"
	./escapecodes
clean:
	@for FILE in ${ASSEMBLY}; do\
		if [ -e $$FILE ]; then \
			echo "Removing $$FILE" && rm $$FILE;\
		fi;\
	done
purge: clean
	@for FILE in ${TARGETS}; do\
		if [ -e $$FILE ]; then \
			echo "Removing $$FILE" && rm $$FILE;\
		fi;\
	done
%.s: %.vsl
	${VSLC} ${VSLFLAGS} -f $*.vsl -o $*.s
