
CXX     := g++
CXXFLAGS =-O3 -Wall -std=c++0x
LDFLAGS  =-lasound

SRCS   = src/main.cpp
OBJS   = $(subst .cpp,.o,$(SRCS))
TARGET = axerig2

#default: all

all: $(TARGET)

.cpp.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

axerig2: $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)
	strip $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)


