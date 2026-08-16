#ifndef POMODORO_H
#define POMODORO_H
#include "tasks.h"

class Pomodoro {
private:
  int workMinutes;
  int breakMinutes;
  int longBreakMinutes;
  Task *task_ = nullptr;
  // Activar el RawMode
  void enableRawMode();
  // Desactivar el rawMode
  void disableRawMode();
  // Verificar si hay una tecla disponible en el buffer
  bool keyAvailable() const;
  // Leer tecla disponible
  char readKey() const;
  // Obtener ancho y altura de la terminal
  void getTerminalSize(int &rows, int &cols) const;
  // Imprimir temporizador
  void printTime(int totalSeconds, const std::string &status, bool paused,
                 int &lastRows, int &lastCols) const;
  // Correr POMODOWO
  void runTimer(int minutes, const std::string &status);
  // ENVIAR NOTIFICACIÓN POMODORO
  void notifyTimer(const std::string &title, const std::string &message,
                   int notifyTime = 5000);

public:
  Pomodoro(int timeWork = 25, int timeBreak = 5, int timeLongBreak = 15);
  void setTask(Task *t);
  void start();
};

#endif
