CXX = g++
CXXFLAGS = -Wall -g

all:	oss	user

oss:	oss.cpp
	$(CXX)	$(CXXFLAGS)	-o	oss	oss.cpp

worker:	worker.cpp
	$(CXX)	$(CXXFLAGS)	-o	worker	worker.cpp

clean:
	rm	-f	oss	user

.PHONY:	all	clean
