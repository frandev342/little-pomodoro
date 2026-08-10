#include <algorithm>
#include <chrono>
// #include <iomanip>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <ostream>
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

// Restauración de la terminal al salir
void restoreTerminal(int) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
  std::cout << "\033[?25h\033[2J\033[H" << std::flush;
  _exit(0);
}

class Pomodoro {
private:
  int workMinutes;
  int breakMinutes;
  int longBreakMinutes;
  bool running;
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
        }
      }
    }
    std::cout << "\033[?25h" << std::flush;
  }
  // SEND NOTIFICATION POMODORO
  void notifyTimer(const std::string &title, const std::string &message,
                   int notifyTime = 3000) {
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

  // Configuración del pomodoro y comienzo
  void start() {
    // Restaurar terminal en caso de salidas forzosas
    signal(SIGINT, restoreTerminal);
    signal(SIGTERM, restoreTerminal);

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

int main() {
  Pomodoro timer(120, 20);
  timer.start();
  return 0;
}
