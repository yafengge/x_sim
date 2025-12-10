CXX = g++
# Use C++17 for std::filesystem and shared_mutex
CXXFLAGS = -std=c++17 -Wall -g -O0
# 调试模式: CXXFLAGS = -std=c++14 -Wall -g -O0

TARGET = systolic_sim
SRCS = systolic.cpp main.cpp pe.cpp mem_if.cpp clock.cpp cube.cpp sim_top.cpp config_loader.cpp config_cache.cpp mini_toml.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)
	# Clean up object files after successful link to save space
	rm -f $(OBJS)

%.o: %.cpp systolic.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS = -std=c++14 -Wall -g -O0
debug: clean $(TARGET)
	gdb ./$(TARGET)

profile: CXXFLAGS = -std=c++14 -Wall -O3 -pg
profile: clean $(TARGET)
	./$(TARGET)
	gprof ./$(TARGET) gmon.out > profile.txt

.PHONY: all clean run debug profile