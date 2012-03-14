VSLC=../bin/vslc
ASFLAGS=-32
LDFLAGS=-m32
SOURCES=$(shell ls *.vsl)
ASSEMBLY=$(subst .vsl,.s,${SOURCES})
ENTRIES=$(subst .vsl,.entries,${SOURCES})
STRINGS=$(subst .vsl,.strings,${SOURCES})
DIFFS=$(patsubst %,%.diff,$(ENTRIES)) $(patsubst %,%.diff,$(STRINGS)) 
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

%.entries.diff: %.vsl $(VSLC)
	@echo "Testing $<..." 
	$(VSLC) $(VSLFLAGS) -f $(*F).vsl >/dev/null 2> $(*F).tmp
	@diff $(*F).tmp $(*F).entries > $@||true
	@rm $(*F).tmp
	@echo "Found `cat $@|wc -l` differences. diff output saved to $@."

%.strings.diff: %.vsl $(VSLC)
	@echo "Testing $<..." 
	$(VSLC) $(VSLFLAGS) -f $(*F).vsl 2>/dev/null|diff - $(*F).strings > $@||true
	@echo "Found `cat $@|wc -l` differences. diff output saved to $@."

%.tree:%.vsl
	${VSLC} ${VSLFLAGS} -f $< 2> $@

%.s: %.vsl
	${VSLC} ${VSLFLAGS} -f $*.vsl -o $*.s
