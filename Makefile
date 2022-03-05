CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++17 $(CXXOPTIMIZE)
USERID=805419480_
CLASSES=

all: server client

.PHONY: debug
debug:
	$(CXX) -DDEBUG -o server $^ $(CXXFLAGS) server.cpp 
	$(CXX) -DDEBUG -o client $^ $(CXXFLAGS) client.cpp 

server: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

client: $(CLASSES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz

dist: tarball
tarball: clean
	tar -cvzf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .
