#include "tasks.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sys/stat.h>

// facilitar uso de la librería
using json = nlohmann::json;

// Ruta fija del archivo de datos (~/.local/share/lit-pomodoro/tasks.json)
std::string tasksPath() {
  const char *home = getenv("HOME");
  std::string dir = std::string(home) + "/.local/share/lit-pomodoro";
  std::string path = dir + "/tasks.json";
  return path;
}

// FUncinoes auxiliares para la conversión de json a struct y viceversa
void to_json(json &j, const Task &t) {
  j = json{{"name", t.name}, {"seconds", t.seconds}};
}

void from_json(const json &j, Task &t) {
  t.name = j.at("name").get<std::string>();
  t.seconds = j.at("seconds").get<int>();
}

// Cargar json a un vector de estructuras
std::vector<Task> loadTasks() {
  std::ifstream f(tasksPath());
  std::vector<Task> tasks;
  if (f.is_open()) {
    json data;
    f >> data;
    tasks = data["tasks"].get<std::vector<Task>>();
  }
  return tasks;
}

// Guardar struct en un json
void saveTasks(const std::vector<Task> &tasks) {
  std::string path = tasksPath();
  std::string dir = path.substr(0, path.find_last_of('/'));
  mkdir(dir.c_str(), 0755);

  // Crear json y guardarlo
  json data;
  data["tasks"] = tasks;
  std::ofstream f(path);
  f << data.dump(2);
}
// Añadir una tarea a tasks
void addTask(std::vector<Task> &tasks, const std::string &name) {
  for (auto &t : tasks) {
    if (t.name == name) {
      std::cout << "Task already exists" << std::endl;
      return;
    }
  }
  tasks.push_back({name, 0});
  std::cout << "Task: " << name << " Added" << std::endl;
}

// Eliminar tarea de tasks
void deleteTask(std::vector<Task> &tasks, const std::string &name) {
  for (auto it = tasks.begin(); it != tasks.end(); ++it) {
    if (it->name == name) {
      tasks.erase(it);
      std::cout << "Task deleted successfully" << std::endl;
      return;
    }
  }
  std::cout << "Task does not exits " << name << std::endl;
}

// Mostrar tareas [name HHh MMm SSs]
void showTasks(const std::vector<Task> &tasks) {
  long long total = 0;
  for (size_t i = 0; i < tasks.size(); i++) {
    int time_i = tasks[i].seconds;
    total += tasks[i].seconds;
    int hours = time_i / 3600;
    int minutes = (time_i % 3600) / 60;
    int seconds = (time_i % 3600) % 60;
    std::cout << "[" << i + 1 << "] " << tasks[i].name << " " << hours << "h "
              << minutes << "m " << seconds << "s" << std::endl;
  }
  int hours = total / 3600;
  int minutes = (total % 3600) / 60;
  int seconds = (total % 3600) % 60;
  std::cout << "Total: " << hours << "h " << minutes << "m " << seconds << "s"
            << std::endl;
}
// Buscar tarea dentro del vectro Tasks
Task *searchTask(std::vector<Task> &tasks, const std::string &name) {
  for (size_t i = 0; i < tasks.size(); i++) {
    if (tasks[i].name == name) {
      return &tasks[i];
    }
  }
  return nullptr;
}
