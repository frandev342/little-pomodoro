#include "pomodoro.h"

#include <algorithm>
#include <chrono>
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

// Manejador global: el manejador de señal activa
volatile sig_atomic_t running = 1;

// Manjeador de señal, solo ponel a bandera
void onSignal(int) { running = 0; }

// Constructor pomodor (asignar tiempo de trabajo, descanso y descanso largo)
Pomodoro::Pomodoro(int timeWork, int timeBreak, int timeLongBreak)
    : workMinutes(timeWork), breakMinutes(timeBreak),
      longBreakMinutes(timeLongBreak) {}

// Establecer tarea a apuntar y acceder a datos rapidamente
void Pomodoro::setTask(Task *t) { task_ = t; }

// Configuración del pomodoro y comienzo
void Pomodoro::start() {
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

// Acivar el Raw Mode, eliminar eco y ENTER obligatorio
void Pomodoro::enableRawMode() {
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
void Pomodoro::disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
}

// Ver si hay teclas en el buffer
bool Pomodoro::keyAvailable() const {
  pollfd pfd{STDIN_FILENO, POLLIN, 0};
  return poll(&pfd, 1, 0) == 1; // retornar 1 si hay datos, 0 si no hay
}

// Retornar el caracter encontrado en el buffer
char Pomodoro::readKey() const {
  char c;
  if (read(STDIN_FILENO, &c, 1) > 0)
    return c;
  return 0;
}
// OBtener ancho y altura de la terminal
void Pomodoro::getTerminalSize(int &rows, int &cols) const {
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
void Pomodoro::printTime(int totalSeconds, const std::string &status,
                         bool paused, int &lastRows, int &lastCols) const {
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
              << DIGITS[d1][r] << " " << DIGITS[d2][r] << " " << COLON[r] << " "
              << DIGITS[d3][r] << " " << DIGITS[d4][r] << " " << COLON[r] << " "
              << DIGITS[d5][r] << " " << DIGITS[d6][r];
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
  std::cout << "\033[" << termRows << ";" << hintCol << "H" << "\033[2m" << hint
            << "\033[0m";

  std::cout << std::flush;
}

// CORRER POMODORO
void Pomodoro::runTimer(int minutes, const std::string &status) {
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
void Pomodoro::notifyTimer(const std::string &title, const std::string &message,
                           int notifyTime) {
  std::string notify_msg = "notify-send -t " + std::to_string(notifyTime) +
                           " '" + title + "' '" + message +
                           "' --urgency=critical";
  std::system(notify_msg.c_str());
}
