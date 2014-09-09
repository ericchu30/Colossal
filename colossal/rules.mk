QUIET		?= @
PREFIX		?= ..
INCPATH		?= $(PREFIX)/include/colossal
LIBPATH		?= $(PREFIX)/lib
BINPATH		?= $(PREFIX)/bin
LIBNAME		?= undef
BINNAME		?= undef

VERSION		?= 1.4
BUILDTIME	?= $(shell date +"%D %H:%M:%S")

EXTINC		?= -I$(PREFIX)/../ulib/include

CFLAGS		?= -O3 -flto -Wall -W -pipe -c -fPIC
CXXFLAGS	?= -O3 -flto -Wall -W -pipe -c -fPIC
DEBUG		?= -DNDEBUG

OBJS		= \
		$(patsubst %.c, %.o, $(wildcard *.c)) \
		$(patsubst %.cpp, %.o, $(wildcard *.cpp))

HEADERS		= $(wildcard *.hpp)

.c.o:
	$(QUIET)echo "CC "$<
	$(QUIET)$(CC) -DVERSION="\"$(VERSION)\"" -DBUILDTIME="\"$(BUILDTIME)\"" \
		$(CFLAGS) -I$(INCPATH) $(EXTINC) $(DEBUG) $< -o $@

.cpp.o:
	$(QUIET)echo "CXX "$<
	$(QUIET)$(CXX) -DVERSION="\"$(VERSION)\"" -DBUILDTIME="\"$(BUILDTIME)\"" \
		$(CXXFLAGS) -I$(INCPATH) $(EXTINC) $(DEBUG) $< -o $@

.PHONY: install_headers install_libs install_binaries \
	uninstall_headers uninstall_libs uninstall_binaries \
	clean

clean: uninstall_headers uninstall_libs uninstall_binaries
	$(QUIET)rm -rf $(OBJS)
	$(QUIET)find . -name "*~" | xargs rm -rf

install_headers:
	$(QUIET)mkdir -p $(INCPATH)
	$(QUIET)cp $(HEADERS) $(INCPATH)/

install_libs: $(OBJS)
	$(QUIET)mkdir -p $(LIBPATH)
	$(QUIET)echo "AR "$(LIBPATH)/$(LIBNAME)
	$(QUIET)ar csr $(LIBPATH)/$(LIBNAME) $(OBJS)

install_binaries:
	$(QUIET)echo "GEN "$(BINNAME)
	$(QUIET)$(CXX) -o $(BINPATH)/$(BINNAME) $(OBJS)uninstall_headers:
	$(QUIET)for file in $(HEADERS); do rm -rf $(INCPATH)/$$file; done

uninstall_libs:
	$(QUIET)rm -rf $(LIBPATH)/$(LIBNAME)

uninstall_binaries:
	$(QUIET)rm -rf $(BINPATH)/$(BINNAME)
