CC = gcc
CFLAGS = -pedantic -Wall -std=c99 -O2 -D_GNU_SOURCE

PROG = ioseqw
OBJECTS = setup.o record.o
COMMONS = common.h

all: $(PROG)

# default dependencies and compile for objects
$(OBJECTS): %.o: %.c %.h $(COMMONS)
	$(CC) $(CFLAGS) -c $< -o $@

# default dependencies and compile for prog
$(PROG:%=%.o): %.o: %.c $(COMMONS)
	$(CC) $(CFLAGS) -c $< -o $@

# default dependencies and link method for prog
$(PROG): %: %.o $(OBJECTS)
	$(CC) $(CFLAGS) $< $(OBJECTS) -o $@

# additional header dependencies for objects
# N/A

# additional header dependencies for prog
ioseqw.o: setup.h

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(PROG:%=%.o) $(PROG)
