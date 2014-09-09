QUIET	?= @

TARGET	= libulib.a
LIBS	= $(filter-out $(TARGET), $(wildcard *.a))

$(TARGET): $(LIBS)
	$(QUIET)for lib in $(LIBS); do $(AR) x $${lib}; done;
	$(QUIET)$(AR) csr $(TARGET) *.o
	$(QUIET)rm -rf *.o

debug: $(TARGET)

release: $(TARGET)
	$(QUIET)strip --strip-debug --strip-unneeded -w -K \* $(TARGET)

clean:
	$(QUIET)rm -rf *.a $(TARGET)

.PHONY: debug release clean
