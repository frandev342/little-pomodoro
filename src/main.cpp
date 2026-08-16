// Inclimo nuestras funciones y class
#include "pomodoro.h"
#include "tasks.h"

// LIbrería para los argumentos y flags
#include <getopt.h>

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  std::vector<Task> tasks = loadTasks();
  int w = 25, b = 5, l = 15;
  bool showHelp = false, showList = false, useTask = false, delTask = false,
       createTask = false;
  std::string task;
  Task *task_ = nullptr;

  // Tabla de opciones (longOpts)
  static struct option longOpts[] = {
      {"work", required_argument, nullptr, 'w'},
      {"break", required_argument, nullptr, 'b'},
      {"longbreak", required_argument, nullptr, 'l'},
      {"help", no_argument, nullptr, 'h'},
      {"show", no_argument, nullptr, 's'},
      {"create", required_argument, nullptr, 'c'},
      {"task", required_argument, nullptr, 't'},
      {"delete", required_argument, nullptr, 'd'},
      {nullptr, 0, nullptr, 0},
  };
  // Habilitar opciones
  int op;
  while ((op = getopt_long(argc, argv, "w:b:l:hsc:t:d:", longOpts, nullptr)) !=
         -1) {
    switch (op) {
    case 'w':
      w = std::stoi(optarg);
      break;
    case 'b':
      b = std::stoi(optarg);
      break;
    case 'l':
      l = std::stoi(optarg);
      break;
    case 'h':
      showHelp = true;
      break;
    case 's':
      showList = true;
      break;
    case 'c':
      createTask = true;
      task = optarg;
      break;
    case 't':
      useTask = true;
      task = optarg;
      break;
    case 'd':
      delTask = true;
      task = optarg;
      break;
    default:
      std::cout << "Unknown option. Use --help." << std::endl;
      return 1;
    }
  }
  // Dispatch
  if (showHelp) {
    std::cout << "Usage: pomodoro [OPTION]...\n\n";
    std::cout << "Terminal Pomodoro timer with task tracking.\n\n";
    std::cout << "Options:\n";
    std::cout << "  -w, --work N         Work minutes (default: 25)\n";
    std::cout << "  -b, --break N        Break minutes (default: 5)\n";
    std::cout << "  -l, --longbreak N    Long break minutes (default: 15)\n";
    std::cout << "  -s, --show           Show the task list\n";
    std::cout << "  -c, --create NAME    Create a new task\n";
    std::cout << "  -t, --task NAME      Work on a task (creates it if it "
                 "doesn't exist)\n";
    std::cout << "  -d, --delete NAME    Delete a task\n";
    std::cout << "  -h, --help           Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  pomodoro                         Free pomodoro (25/5/15)\n";
    std::cout << "  pomodoro --work 50               Free pomodoro with 50 min "
                 "of work\n";
    std::cout << "  pomodoro -t \"Study English\"      Pomodoro tracking time "
                 "on a task\n";
    std::cout << "  pomodoro --show                  View time accumulated per "
                 "task\n";
    return 0;

  } else {
    if (showList) {
      showTasks(tasks);
      return 0;
    }
    if (createTask) {
      addTask(tasks, task);
      saveTasks(tasks);
      return 0;
    }
    if (delTask) {
      deleteTask(tasks, task);
      saveTasks(tasks);
      return 0;
    }
    if (useTask && task != "") {
      task_ = searchTask(tasks, task);
      if (!task_) {
        addTask(tasks, task);
        task_ = &tasks.back();
        saveTasks(tasks);
      }
    }
  }

  Pomodoro timer(w, b, l);
  timer.setTask(task_);
  timer.start();
  saveTasks(tasks);
  return 0;
}
