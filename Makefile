CC=g++
CFLAGS=-c -Wall -std=c++11 -pthread -Ofast
LDFLAGS=-pthread
SOURCES=main.cpp Hotspot.cpp Coordinate.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=heatmap

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
