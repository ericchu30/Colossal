QUIET		?= @

INCPATH		= ../../include
LIBPATH		= ../../lib

EXTRAINC	?= -I../../../ulib/include -I../../../libconfig/include
EXTRALIB	?= -L../../../ulib/lib -lulib -L../../../libconfig/lib -lconfig++ -L../../../gperftools/lib -lprofiler

CXXFLAGS	?= -O3 -flto -W -Wall
LDFLAGS		?= -lcolossal $(EXTRALIB)
DEBUG		?=

TARGET		= $(patsubst %.cpp, %.app, $(wildcard *.cpp))

%.app: %.cpp $(LIBPATH)/libcolossal.a
	$(QUIET)echo "GEN "$@;
	$(QUIET)$(CXX) -I $(INCPATH) $(EXTRAINC) $(CXXFLAGS) $(DEBUG) $< -o $@ -L $(LIBPATH) $(LDFLAGS);

all: $(TARGET)

clean:
	$(QUIET)rm -rf $(TARGET)
	$(QUIET)find . -name "*~" | xargs rm -rf

.PHONY: all clean test
