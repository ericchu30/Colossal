QUIET		?= @

INCPATH		= ../include
LIBPATH		= ../lib

EXTRALIB	?=

CXXFLAGS	?= -g3 -O2 -W -Wall
LDFLAGS		?= -lulib $(EXTRALIB) -lpthread
DEBUG		?=

TARGET		= $(patsubst %.cpp, %.test, $(wildcard *.cpp))

%.test: %.cpp $(LIBPATH)/libulib.a
	$(QUIET)echo "GEN "$@;
	$(QUIET)$(CXX) -I $(INCPATH) $(CXXFLAGS) $(DEBUG) $< -o $@ -L $(LIBPATH) $(LDFLAGS);

all: $(TARGET)

clean:
	$(QUIET)rm -rf $(TARGET) *~

test:
	$(QUIET)./run_all_test.sh

.PHONY: all clean test
