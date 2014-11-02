TARGET=$(shell basename `pwd`)
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:%.cpp=%.o)
CXXFLAGS=-std=c++11 $(CFLAGS)

all: $(TARGET)

$(OBJECTS): $(SOURCES)

$(TARGET): $(OBJECTS) 
	$(CXX) -pthread -o $(TARGET) $(LDFLAGS) $(OBJECTS) $(LOADLIBES) $(LDLIBS)

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET) 
