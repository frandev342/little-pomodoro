CXX = g++
FLAGS = -std=c++17 -Wall -Wextra
SRC = src/main.cpp src/tasks.cpp src/pomodoro.cpp
pomodoro: $(SRC)
	$(CXX) $(FLAGS) -o pomodoro $(SRC)

run: pomodoro
	./pomodoro

clean:
	rm -f pomodoro
