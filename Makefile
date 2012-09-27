
CC=g++
PWD=`pwd`

# external libraries used for this interpreter
INCDIRS := ./include/
LIBS := 

# include the interpreter source and its inferred dependencies
include project.mk

ifdef DEBUG
	OPTARG := -g
	TDIR   := debug
	EXNAME := dscm
else
	OPTARG := -O4
	TDIR   := release
	EXNAME := scm
endif

ifdef PROFILE
	PROFARG := -pg
else
	PROFARG :=
endif

OBJECTS := $(addprefix build/obj/$(TDIR)/, $(addsuffix .o, $(basename $(SOURCES))))

CPPFLAGS := -pthread $(INCDIRS:%=-I%) $(OPTARG) $(PROFARG) -m64 -Wall -Wno-deprecated
LIBTEXT  := $(addprefix -l, $(LIBS))

scm: dirs $(OBJECTS)
	$(CC) -rdynamic $(CPPFLAGS) $(OBJECTS) $(LIBTEXT) -o build/bin/$(EXNAME)

build/obj/$(TDIR)/%.o:%.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

build/obj/$(TDIR)/%.o:%.mm
	$(CC) $(CPPFLAGS) -c $< -o $@

build/obj/$(TDIR)/%.o:%.c
	$(CC) $(CPPFLAGS) -c $< -o $@

build/obj/$(TDIR)/%.o:%.m
	$(CC) $(CPPFLAGS) -c $< -o $@

dirs:
	mkdir -p build/dep/
	mkdir -p build/obj/$(TDIR)/
	mkdir -p build/bin/
	find . -type d | grep -v "build" | grep -v '\.$$' | awk '{print "build/obj/$(TDIR)/" $$0}' | xargs mkdir -p

clean:
	rm -rf build
	mkdir -p build/dep/

install: scm
	ln -sf $(PWD)/build/bin/scm /usr/bin/scm
	ln -sf $(PWD)/build/bin/dscm /usr/bin/dscm
	ln -sf $(PWD)/include/scm /usr/include/scm
