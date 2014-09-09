TARGET = librpclib.a

CXXFLAGS=-DUNDEBUG -O2 -W -Wall -I../../include

.cpp.o:
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

$(TARGET): rpclib.o
	ar csr $(TARGET) rpclib.o
	rm rpclib.o

clean:
	find . -name "*~" | xargs --replace=file rm -rf "file"
	rm -rf $(TARGET)

.PHONY: clean
