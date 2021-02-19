TARGET = monitor
SRC = main.cpp Timer.cpp Log.cpp IMonitor.cpp MonitorNotify.cpp CPUMonitor.cpp MemoryMonitor.cpp NetMonitor.cpp
OBJS = $(SRC:.cpp=.o)
CXX = g++
LIBS = -lpthread
CXXFLAGS = -g -lpcap -lcurl -std=c++11 

all:$(TARGET)
$(TARGET):$(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)
$(OBJS):
clean:
	rm -f *.o
