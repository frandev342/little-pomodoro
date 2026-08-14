#include <algorithm>
#include <chrono>
// #include <iomanip>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

#include <poll.h>
#include <termios.h>

#include <vector>

// Librerías para la mejora del pomodoro
#include <fstream>
#include <getopt.h>
#include <nlohmann/json.hpp>
#include <string.h>

// timer drawing
const std::vector<std::vector<std::string>> DIGITS = {
    {"███", "█ █", "█ █", "█ █", "███"}, {"  █", "  █", "  █", "  █", "  █"},
    {"███", "  █", "███", "█  ", "███"}, {"███", "  █", "███", "  █", "███"},
    {"█ █", "█ █", "███", "  █", "  █"}, {"███", "█  ", "███", "  █", "███"},
    {"███", "█  ", "███", "█ █", "███"}, {"███", "  █", "  █", "  █", "  █"},
    {"███", "█ █", "███", "█ █", "███"}, {"███", "█ █", "███", "  █", "  █"},
};
const std::vector<std::string> COLON = {"   ", " █ ", "   ", " █ ", "   "};

// Guardar configuración de la terminal original
termios originalTermios;

// Restauración de la terminal al salir
void restoreTerminal(int) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
  std::cout << "\033[?25h\033[2J\033[H" << std::flush;
  _exit(0);
}

// Manejador global: el manejador de señal activa
volatile sig_atomic_t running = 1;

// Manjeador de señal, solo ponel a bandera
void onSignal(int) { running = 0; }

// facilitar uso de la librería
using json = nlohmann::json;
// Estructura Task
struct Task {
  std::string name;
  int seconds;
};

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
  std::ifstream f("tasks.json");
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
  json data;
  data["tasks"] = tasks;
  std::ofstream f("tasks.json");
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
  std::cout << "Task does not exits" << name << std::endl;
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

class Pomodoro {
private:
  int workMinutes;
  int breakMinutes;
  int longBreakMinutes;
  // bool running;
  Task *task_ = nullptr;
  void enableRawMode() {
    // Guardamos configuración inicial de la terminal en originalTErmios para
    // aplicar al salir
    tcgetattr(STDIN_FILENO, &originalTermios);
    // Quitamos el echo y el ENTER_necesario
    termios raw = originalTermios;
    raw.c_lflag &= ~(ECHO | ICANON);
    // Aplicamos el cambio
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  }
  // Regresamos los cambios originales de la terminal
  void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
  }
  // Ver si hay teclas en el buffer
  bool keyAvailable() const {
    pollfd pfd{STDIN_FILENO, POLLIN, 0};
    return poll(&pfd, 1, 0) == 1; // retornar 1 si hay datos, 0 si no hay
  }
  char readKey() const {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0)
      return c;
    return 0;
  }
  // OBtener ancho y altura de la terminal
  void getTerminalSize(int &rows, int &cols) const {
    // PEDIR TAMAÑO DE LA VENTANA
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
      rows = w.ws_row;
      cols = w.ws_col;
    } else {
      rows = 24;
      cols = 80;
    }
  }

  // Imprimir temporizador
  void printTime(int totalSeconds, const std::string &status, bool paused,
                 int &lastRows, int &lastCols) const {
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    // Asignamos el número en cada dígito del TEMPORIZADOR
    int d1 = hours / 10;
    int d2 = hours % 10;
    int d3 = minutes / 10;
    int d4 = minutes % 10;
    int d5 = seconds / 10;
    int d6 = seconds % 10;

    int termRows, termCols;
    getTerminalSize(termRows, termCols);

    // HACER LIMPIEZA DE PANTALLA Y GUARDAR ÚLTIMAS FILAS Y COLUMNAS
    if (termRows != lastRows || termCols != lastCols) {
      std::cout << "\033[2J" << std::flush;
      lastRows = termRows;
      lastCols = termCols;
    }

    // Tamaño del reloj
    int clockWidth = 31;
    int clockHeight = 5;

    // Centrado
    int startColumn = (termCols - clockWidth) / 2;
    int startRow = (termRows - clockHeight) / 2 + 1;

    // Impresión del reloj
    for (int r = 0; r < clockHeight; r++) {
      std::cout << "\033[" << startRow + r << ";" << startColumn << "H"
                << DIGITS[d1][r] << " " << DIGITS[d2][r] << " " << COLON[r]
                << " " << DIGITS[d3][r] << " " << DIGITS[d4][r] << " "
                << COLON[r] << " " << DIGITS[d5][r] << " " << DIGITS[d6][r];
    }
    // Centrado de la etiqueta status
    int labelColumn = (termCols - (status.size() + 2)) / 2;
    int labelRow = std::max(1, startRow - 2);

    std::string statusText = status;

    // Mostrar estado en pantalla
    if (paused) {
      statusText += " [PAUSED]";
      labelColumn = (termCols - (statusText.size() + 2)) / 2;
      labelRow = std::max(1, startRow - 2);
    }
    // Limpieza de línea para escribir el status en pantalla
    std::string color = (status == "WORK") ? "\033[1;31m" : "\033[1;36m";
    std::cout << "\033[1m\033[" << labelRow << ";" << labelColumn << "H"
              << "\033[2K" << color << "<" << statusText << ">\033[0m";

    // Mostrar tarea en pantalla y tiempo
    if (task_ != nullptr) {
      int taskNameCol = 2;

      int taskH = task_->seconds / 3600;
      int taskM = (task_->seconds % 3600) / 60;
      int taskS = (task_->seconds % 3600) % 60;
      std::string taskTime = "Time spent: ";
      int taskTimeCol = termCols - 8 - taskTime.size();
      std::cout << "\033[" << termRows << ";" << taskNameCol << "H"
                << "\033[2K" << task_->name;
      std::cout << "\033[" << termRows << ";" << taskTimeCol << "H" << taskTime
                << std::setfill('0') << std::setw(2) << taskH << ":"
                << std::setw(2) << taskM << ":" << std::setw(2) << taskS
                << std::setfill(' ');
    }

    // Sugerencias disponibles
    std::string hint =
        paused ? "[P] Reanudar [Q] Salir" : "[P] Pausar  [Q] Salir";
    int hintCol = (termCols - hint.length()) / 2;
    if (hintCol < 1)
      hintCol = 1;
    std::cout << "\033[" << termRows << ";" << hintCol << "H" << "\033[2m"
              << hint << "\033[0m";

    std::cout << std::flush;
  }

  // CORRER POMODORO
  void runTimer(int minutes, const std::string &status) {
    int totalSeconds = minutes * 60;
    bool paused = false;
    int ticks = 0;
    // Variables control, actualización del centrado
    int lastRows = 0;
    int lastCols = 0;

    std::cout << "\033[2J\033[H\033[?25l" << std::flush;
    printTime(totalSeconds, status, paused, lastRows, lastCols);

    // ACTUALIZACIÓN DEL TEMPORIZADOR
    while (totalSeconds >= 0 && running) {
      // Rastrear teclas presionadas
      while (keyAvailable()) {
        char c = readKey();
        if (c == 'p' || c == 'P') {
          paused = !paused;
          printTime(totalSeconds, status, paused, lastRows, lastCols);
        } else if (c == 'q' || c == 'Q') {
          running = false;
          break;
        } else if (c == 'c' || c == 'C') {
          totalSeconds = 0;
        }
      }
      // Salir en caso running sea falso
      if (!running)
        break;
      // IMPRIMIR RELOj
      printTime(totalSeconds, status, paused, lastRows, lastCols);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Avance del tiempo controlado por ticks
      if (!paused && running) {
        ticks++;
        if (ticks >= 10) {
          ticks = 0;
          totalSeconds--;
          // Actualizar tiemp mostrado, si es que se indico un trabajo
          if (task_ != nullptr && status == "WORK") {
            task_->seconds++;
          }
        }
      }
    }
    std::cout << std::flush;
  }
  // SEND NOTIFICATION POMODORO
  void notifyTimer(const std::string &title, const std::string &message,
                   int notifyTime = 5000) {
    std::string notify_msg = "notify-send -t " + std::to_string(notifyTime) +
                             " '" + title + "' '" + message +
                             "' --urgency=critical";
    std::system(notify_msg.c_str());
  }

public:
  // Constructor pomodor (asignar tiempo de trabajo, descanso y descanso largo)
  Pomodoro(int timeWork = 25, int timeBreak = 5, int timeLongBreak = 15)
      : workMinutes(timeWork), breakMinutes(timeBreak),
        longBreakMinutes(timeLongBreak) {}

  // Establecer tarea a apuntar y acceder a datos rapidamente
  void setTask(Task *t) { task_ = t; }

  // Configuración del pomodoro y comienzo
  void start() {
    // Restaurar terminal en caso de salidas forzosas
    // signal(SIGINT, restoreTerminal);
    // signal(SIGTERM, restoreTerminal);
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    // Establecer el modo raw
    enableRawMode();
    running = true;
    int cycles = 0;

    // CICLOS DEL POMODORO
    while (running) {
      notifyTimer("POMODORO", "Time to work");
      runTimer(workMinutes, "WORK");
      if (!running) {
        break;
      }
      cycles++;
      if (cycles % 4 == 0) {
        notifyTimer("POMODORO", "Time to long break");
        runTimer(longBreakMinutes, "BREAK");
      } else {
        notifyTimer("POMODORO", "Time to break");
        runTimer(breakMinutes, "BREAK");
      }
    }
    disableRawMode();
    std::cout << "\033[?25h\033[2J\033[H" << std::flush;
  }
};

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
