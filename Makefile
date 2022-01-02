CFLAGS         := -std=gnu99 -Wall -Wextra -Wconversion -D'CONFIG="vrptwms.conf"'
CFLAGS_DEBUG   := -O0 -ggdb3 -fbounds-check -DDEBUG
CFLAGS_PROFILE := -O2 -pg
CFLAGS_RELEASE := -O3
CFLAGS_STATS   := -DSTATS
LINK_FLAGS     := -lm -lconfuse
LFLAGS_DEBUG   :=

PROGRAM        := vrptwms
CC             := gcc
HEADERS        := $(wildcard *.h)
OBJFILES       := $(patsubst %.c,%.o,$(wildcard *.c))

# Makefile currently doesn't work as cpp files are not compiled
CPP            := g++
CPPHEADERS     := $(wildcard *.hpp)
CPPOBJFILES    := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

TESTDIR        := tests

all: $(PROGRAM)

$(PROGRAM): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OBJFILES) $(LINK_FLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<
	gcc -MM $(CFLAGS) $*.c > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

-include $(OBJS:.o=.d)

doc: *.c *.h Doxyfile
	doxygen Doxyfile

tests: always-remake
	cppcheck --std=c99 .
	cd $(TESTDIR) && $(MAKE)

.PHONY: clean
clean:
	find . -name '*~' \
	       -o -name '*.out' \
	       -o -name 'run_tests' \
	       -o -name '*.d' \
	       -o -name '*.gch' \
	       -o -name '*.o' -o -name '$(PROGRAM)' | xargs rm

# # automatically take care of dependencies
# depend: .depend.mk
# .depend.mk: $(HEADERS)
# 	rm -f ./.depend.mk
# 	$(CC) $(CFLAGS) -MM $^ >> ./.depend.mk
# include .depend.mk

.PHONY: always-remake
 always-remake:
	@true

# make clean needs to be executed in order to create a new debug build
.PHONY: debug
debug: CFLAGS += $(CFLAGS_DEBUG)
debug: LINK_FLAGS += $(LFLAGS_DEBUG)
debug: all

.PHONY: profile
profile: CFLAGS += $(CFLAGS_PROFILE)
profile: all

.PHONY: release
release: CFLAGS += $(CFLAGS_RELEASE)
release: all

.PHONY: stats
stats: CFLAGS += $(CFLAGS_STATS)
stats: all
