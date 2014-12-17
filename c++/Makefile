CXXFLAGS=-pthread --std=c++11 -Wl,--no-as-needed -O3
LDFLAGS=-lboost_system
LIBS=http_handler.o
HEADERS=http_handler.hpp
BINS=server

all: ${BINS}

server: server.o ${LIBS} ${HEADERS}
	g++ server.o ${LIBS} ${LDFLAGS} -o $@

clean:
	rm -rf ${bins} *.o *.core core
