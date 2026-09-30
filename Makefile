CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = chip8
SOURCES = src/main.cpp src/chip8.cpp src/ui_font.cpp src/app_state.cpp src/ui_renderer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) -lSDL2

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Headless CPU-core tests (no SDL needed)
test: tests/test_core.cpp src/chip8.cpp
	$(CXX) -std=c++17 -Wall -Wextra -O2 -o run_tests tests/test_core.cpp src/chip8.cpp
	./run_tests

clean:
ifeq ($(OS),Windows_NT)
	-cmd /C "del /Q /F src\*.o $(TARGET).exe $(TARGET) run_tests.exe run_tests 2>NUL"
else
	rm -f $(OBJECTS) $(TARGET) run_tests run_tests.exe
endif

.PHONY: all clean test