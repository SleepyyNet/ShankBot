#------------------------------------------------------------------------------#
# This makefile was generated by 'cbp2make' tool rev.147                       #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CC = gcc
CXX = g++
AR = ar
LD = g++
WINDRES = windres

INC = -Iincl
CFLAGS = -Wall -std=c++0x -m32 -fPIC
RESINC = 
LIBDIR = 
LIB = -ldl
LDFLAGS = -m32

INC_RELEASE = $(INC)
CFLAGS_RELEASE = $(CFLAGS) -O2
RESINC_RELEASE = $(RESINC)
RCFLAGS_RELEASE = $(RCFLAGS)
LIBDIR_RELEASE = $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE = $(LDFLAGS) -s
OBJDIR_RELEASE = obj/Release
DEP_RELEASE = 
OUT_RELEASE = bin/Release/libGraphicsMonitor.so

INC_DEBUG = $(INC)
CFLAGS_DEBUG = $(CFLAGS) -O2
RESINC_DEBUG = $(RESINC)
RCFLAGS_DEBUG = $(RCFLAGS)
LIBDIR_DEBUG = $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG = $(LDFLAGS) -s
OBJDIR_DEBUG = obj/Release
DEP_DEBUG = 
OUT_DEBUG = bin/Release/libGraphicsMonitor.so

OBJ_RELEASE = $(OBJDIR_RELEASE)/src/inject.o

OBJ_DEBUG = $(OBJDIR_DEBUG)/src/inject.o

all: release debug

clean: clean_release clean_debug

before_release: 
	test -d bin/Release || mkdir -p bin/Release
	test -d $(OBJDIR_RELEASE)/src || mkdir -p $(OBJDIR_RELEASE)/src

after_release: 

release: before_release out_release after_release

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(LD) -shared $(LIBDIR_RELEASE) $(OBJ_RELEASE)  -o $(OUT_RELEASE) $(LDFLAGS_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/src/inject.o: src/inject.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/inject.cpp -o $(OBJDIR_RELEASE)/src/inject.o

clean_release: 
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf bin/Release
	rm -rf $(OBJDIR_RELEASE)/src

before_debug: 
	test -d bin/Release || mkdir -p bin/Release
	test -d $(OBJDIR_DEBUG)/src || mkdir -p $(OBJDIR_DEBUG)/src

after_debug: 

debug: before_debug out_debug after_debug

out_debug: before_debug $(OBJ_DEBUG) $(DEP_DEBUG)
	$(LD) -shared $(LIBDIR_DEBUG) $(OBJ_DEBUG)  -o $(OUT_DEBUG) $(LDFLAGS_DEBUG) $(LIB_DEBUG)

$(OBJDIR_DEBUG)/src/inject.o: src/inject.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/inject.cpp -o $(OBJDIR_DEBUG)/src/inject.o

clean_debug: 
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf bin/Release
	rm -rf $(OBJDIR_DEBUG)/src

.PHONY: before_release after_release clean_release before_debug after_debug clean_debug

