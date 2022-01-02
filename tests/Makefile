CC             := gcc
CPP            := g++
CFLAGS         := -std=gnu99 -Wall -Wextra -Wconversion
LINK_FLAGS     := -rdynamic -Bstatic -lm -lboost_filesystem -lboost_system -lconfuse -lgtest -Bdynamic -lpthread
CPPFLAGS       := -std=gnu++0x -Wall -Wextra -Wconversion

PROGRAM        := testrunner
HEADERS        := $(wildcard ../*.h)
TESTFILES      := $(wildcard ./*.cpp)
TESTOBJECTS    := $(patsubst %.c,%.o,$(TESTFILES))
OBJFILES       := $(patsubst %.c,%.o,$(wildcard ../*.c))
OBJFILES       := $(subst ../cli.o,,$(OBJFILES))

all: $(PROGRAM)

$(PROGRAM): testrunner.o $(OBJFILES)
	$(CPP) $(CPPFLAGS) -o $(PROGRAM) $(TESTOBJECTS) $(OBJFILES) $(LINK_FLAGS)

%.o: %.cpp %.hpp
	$(CC) $(CFLAGS) -c -o $@ $<

testrunner.o: testrunner.cpp $(TESTOBJECTS)
	$(CPP) testrunner.cpp  $(CPPFLAGS) -c

.PHONY: clean
clean:
	rm *.o $(PROGRAM)

.PHONY : always-remake
 always-remake:
	@true
