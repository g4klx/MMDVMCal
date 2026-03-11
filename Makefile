LD  = c++
CXX = c++

CXXFLAGS = -g -O2 -Wall -MMD -MD -std=c++0x
LDFLAGS = -g

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all:	MMDVMCal

MMDVMCal:	$(OBJS)
		$(CXX) $(OBJS) $(LDFLAGS) -o MMDVMCal $(LIBS)

%.o: %.cpp
		$(CXX) $(CXXFLAGS) -c -o $@ $<
-include $(DEPS)

install:
		install -m 755 MMDVMCal /usr/local/bin/

clean:
		rm -f *.o *.d *.bak *~ MMDVMCal

