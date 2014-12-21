CXXFLAGS=-pthread --std=c++11 -Wl,--no-as-needed -O3
LDFLAGS=-lboost_system -pthread
LIBS=http_handler.o sse_server.o sse_client.o
HEADERS=http_handler.hpp sse_server.hpp sse_client.hpp
BINS=server

all: ${BINS}

server: server.o ${LIBS} ${HEADERS}
	g++ server.o ${LIBS} ${LDFLAGS} -o $@

clean:
	rm -rf ${bins} *.o *.core core
