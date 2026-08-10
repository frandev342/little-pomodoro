CXX = g++
FLAGS = -std=c++17 -Wall -Wextra
pomodoro: src/main.cpp
	$(CXX) $(FLAGS) -o pomodoro src/main.cpp

run: pomodoro
	./pomodoro

clean:
	rm -f pomodoro
