
#CC=g++
CC=llvm-g++
PWD=`pwd`

# external libraries used for this interpreter
INCDIRS := ./include/
LIBS :=

# include the interpreter source and its inferred dependencies
include project.mk
include build/dep/Makefile.dep

OBJECTS := $(addprefix build/obj/, $(addsuffix .o, $(basename $(SOURCES))))

ifdef DEBUG
	OPTARG := -g
else
	OPTARG := -O3
endif

ifdef PROFILE
	PROFARG := -pg
else
	PROFARG :=
endif

CPPFLAGS := -pthread $(INCDIRS:%=-I%) $(OPTARG) $(PROFARG) -m64 -Wall -Wno-deprecated

scm: depend $(OBJECTS)
	$(CC) -shared $(CPPFLAGS) $(OBJECTS) -o build/lib/libscm.so

build/obj/%.o:%.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

build/obj/%.o:%.mm
	$(CC) $(CPPFLAGS) -c $< -o $@

build/obj/%.o:%.c
	$(CC) $(CPPFLAGS) -c $< -o $@

build/obj/%.o:%.m
	$(CC) $(CPPFLAGS) -c $< -o $@

dirs:
	mkdir -p build/dep/
	mkdir -p build/obj/
	mkdir -p build/lib/
	chmod +rw build/dep/Makefile.dep
	echo "" > build/dep/Makefile.dep
	find . -type d | grep -v "build" | grep -v '\.$$' | awk '{print "build/obj/" $$0}' | xargs mkdir -p

depend: dirs
	makedepend $(INC_TEXT) -f build/dep/Makefile.dep $(SCM_SOURCES) > /dev/null 2>&1

clean:
	rm -rf build
	mkdir -p build/dep/
	echo "" > build/dep/Makefile.dep

install: scm
	ln -sf $(PWD)/build/lib/libscm.so /usr/lib/libscm.so
	ln -sf $(PWD)/include/scm /usr/include/scm
