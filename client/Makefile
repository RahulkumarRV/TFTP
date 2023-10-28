# Compiler
CXX = g++
# Compiler flags
CXXFLAGS = -std=c++11 -Wall

# Source file and executable name
SRC = client.cpp
EXECUTABLE = client

$(EXECUTABLE): $(SRC)
	$(CXX) $(SRC) -o $(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE)
