TARGETS = scrawl

all : $(TARGETS)

check : scrawl
	sh run_tests.sh

clean :

distclean : clean
	-rm $(TARGETS)

scrawl : c/scrawl.c
	$(CC) -o scrawl -Wall -Os c/scrawl.c

.PHONY : all check clean distclean
