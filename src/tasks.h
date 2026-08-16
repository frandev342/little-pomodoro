#ifndef TASKS_H
#define TASKS_H

#include <string>
#include <vector>

// Estructura Task (para facilitar el uso del json)
struct Task {
  std::string name;
  int seconds;
};

// cargar json a un vector de estructuras
std::vector<Task> loadTasks();
// guardar struct en un json
void saveTasks(const std::vector<Task> &tasks);
// añadir un tarea a task
void addTask(std::vector<Task> &tasks, const std::string &name);
// Eliminar tarea de tasks
void deleteTask(std::vector<Task> &tasks, const std::string &name);
// Mostrar tareas [name HHh MMm SSs]
void showTasks(const std::vector<Task> &tasks);
// Buscar tarea dentro del vectro Tasks
Task *searchTask(std::vector<Task> &tasks, const std::string &name);

#endif
