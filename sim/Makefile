CXX = g++
CXXFLAGS = -std=c++11 -Wall -g -O0
# 调试模式: CXXFLAGS = -std=c++11 -Wall -g -O0

TARGET = systolic_sim
SRCS = systolic.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp systolic.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS = -std=c++11 -Wall -g -O0
debug: clean $(TARGET)
	gdb ./$(TARGET)

profile: CXXFLAGS = -std=c++11 -Wall -O3 -pg
profile: clean $(TARGET)
	./$(TARGET)
	gprof ./$(TARGET) gmon.out > profile.txt

.PHONY: all clean run debug profile