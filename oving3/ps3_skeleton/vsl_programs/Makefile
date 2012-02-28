VSLC=../bin/vslc
ASFLAGS=-32
LDFLAGS=-m32
SOURCES=$(shell ls *.vsl)
ASSEMBLY=$(subst .vsl,.s,${SOURCES})
TREES=$(subst .vsl,.tree,${SOURCES})
DIFFS=$(subst .vsl,.diff,${SOURCES})
TARGETS=${ASSEMBLY}

SHELL=/bin/bash

all: $(TARGETS) 

trees: $(TREES)

test: $(DIFFS)

clean:
	@for FILE in ${ASSEMBLY}; do\
		if [ -e $$FILE ]; then \
			echo "Removing $$FILE" && rm $$FILE;\
		fi;\
	done
	rm -f *.diff
	
purge: clean
	@for FILE in ${TARGETS}; do\
		if [ -e $$FILE ]; then \
			echo "Removing $$FILE" && rm $$FILE;\
		fi;\
	done

%.diff: %.vsl $(VSLC)
	@echo "Testing $<..." 
	$(VSLC) $(VSLFLAGS) -f $< 2>&1 |diff - $(*F).tree > $@||true
	@echo "Found `cat $@|wc -l` differences. diff output saved to $@."

%.tree:%.vsl
	${VSLC} ${VSLFLAGS} -f $< 2> $@

%.s: %.vsl
	${VSLC} ${VSLFLAGS} -f $*.vsl -o $*.s
