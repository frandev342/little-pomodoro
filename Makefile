CXX = g++
FLAGS = -std=c++17 -Wall -Wextra
SRC = src/main.cpp src/tasks.cpp src/pomodoro.cpp
BIN = lit-pomodoro
lit-pomodoro: $(SRC)
	$(CXX) $(FLAGS) -o $(BIN) $(SRC)

run: $(BIN)
	./$(BIN)

install: $(BIN)
	sudo cp $(BIN) /usr/local/bin/$(BIN)

clean:
	rm -f lit-pomodoro


