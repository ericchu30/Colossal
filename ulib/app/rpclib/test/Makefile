TARGET = svr cli cli1 stress test

CXXFLAGS=-DUNDEBUG -O2 -W -Wall -I../../../include -I../
LDFLAGS=-L../ -lrpclib -L../../../lib -lulib -lpthread

.cpp.o:
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

$(TARGET): rpc_svr.o rpc_cli.o rpc_cli1.o rpc_stress.o test.o
	$(CXX) -o svr rpc_svr.o $(LDFLAGS)
	$(CXX) -o cli rpc_cli.o $(LDFLAGS)
	$(CXX) -o cli1 rpc_cli1.o $(LDFLAGS)
	$(CXX) -o test test.o $(LDFLAGS)
	$(CXX) -o stress rpc_stress.o $(LDFLAGS)
	rm -rf rpc_svr.o rpc_cli.o rpc_stress.o test.o

clean:
	find . -name "*~" | xargs --replace=file rm -rf "file"
	rm -rf $(TARGET)

.PHONY: clean
