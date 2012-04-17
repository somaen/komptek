VSLC=../bin/vslc
ASFLAGS=-32
LDFLAGS=-m32
SOURCES=$(shell ls *.vsl)
ASSEMBLY=$(subst .vsl,.s,${SOURCES})
TARGETS=$(subst .vsl,,${SOURCES})
all: ${TARGETS}
asm: ${ASSEMBLY}
test: all
	for i in $(TARGETS); do\
		echo "-- Testing $$i...";\
		./$$i;\
	done
clean:
	@for FILE in ${ASSEMBLY} $(TARGETS); do\
		if [ -e $$FILE ]; then \
			echo "Removing $$FILE" && rm $$FILE;\
		fi;\
	done

%.s: %.vsl
	${VSLC} ${VSLFLAGS} -f $*.vsl -o $*.s

$(TARGETS): $(ASSEMBLY)
	gcc -m32 $@.s -o $@ 
