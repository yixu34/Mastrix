include Makefile.config

EXECUTABLE=mastrix.exe
OBJDIR=Debug

INCLUDES=-ISDL_Image/include -Ifmod/inc -I/usr/local/include/SDL -Ihawknl
CC=gcc
CXX=g++
LD=g++
rm=rm -f
ALL_CXXFLAGS=$(CXXFLAGS) $(INCLUDES) -O2 -Wall -DDISABLE_NETWORKING
ALL_CCFLAGS=$(CFLAGS) $(INCLUDES) -O2 -Wall
LIBS=-framework AppKit -framework Carbon -framework OpenGL -framework GLUT -framework Foundation -lSDL -lSDL_image -lSDLmain -lfmod -lm

SOURCES=$(wildcard *.cpp ../shared/*.cpp ../net2/*.c)
OBJFILES=$(foreach file, $(SOURCES), $(basename $(notdir $(file))).o)
OBJECTS=$(foreach file, $(OBJFILES), $(OBJDIR)/$(file))

$(EXECUTABLE): $(OBJECTS)
	$(echo) " LINK" $(OBJFILES)
	$(Q)$(LD) -o $@ $(OBJECTS) $(LIBS)

$(OBJDIR)/%.o: %.cpp
	$(echo) " Cxx " $<
	$(Q)$(CXX) $(ALL_CXXFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.c
	$(echo) " CC  " $<
	$(Q)$(CC) $(ALL_CFLAGS) -c -o $@ $<

clean:
	$(echo) " clean"
	$(Q)$(rm) $(OBJDIR)/*.o

DO_DEP = $(foreach file, $(SOURCES), $(CXX) $(INCLUDES) -MT 'obj/$(file)')
dep:
	$(echo) " DEP "
	$(Q)$(CXX) -MM $(INCLUDES) $(SOURCES) \
	  |sed -e 's/\([^:]*\):\(.*\)/$(OBJDIR)\/\1:\2/'  \
      >Makefile.dep

#include Makefile.dep

