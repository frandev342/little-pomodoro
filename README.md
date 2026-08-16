# Pomodoro CLI

Temporizador Pomodoro de terminal en C++ con reloj de arte ASCII, gestión de
tareas persistentes, ciclos de trabajo/descanso, pausa y notificaciones de
escritorio.

## Características

- Reloj grande de arte ASCII con formato `HH:MM:SS`
- Ciclos de **trabajo** → **descanso** → ... con descanso largo cada 4 ciclos
- **Gestión de tareas**: crear, listar, eliminar y acumular tiempo por tarea
- Tiempo de trabajo persistido en `tasks.json`
- Visualización del tiempo de la sesión actual y el pomodoro en curso
- Pausa con `p`, salida limpia con `q`, salto de fase con `c`
- Notificaciones de escritorio con `notify-send`
- Guardado automático del tiempo al salir (incluso con Ctrl+C)
- Centrado dinámico según el tamaño de la ventana

## Requisitos

- Linux (usa `termios`, `poll`, `ioctl`)
- `g++` con soporte C++17
- `nlohmann-json` (para la persistencia en JSON)
- `notify-send` (opcional, para las notificaciones)

## Compilar y ejecutar

```bash
make          # compila → genera el binario pomodoro
make run      # compila y ejecuta
make clean    # borra el binario
```

O manualmente:

```bash
g++ -std=c++17 -Wall -Wextra -o pomodoro src/main.cpp src/tasks.cpp src/pomodoro.cpp
./pomodoro
```

## Uso

### Temporizador

Sin argumentos corre un pomodoro libre con los tiempos por defecto (25/5/15):

```bash
./pomodoro
```

Con una tarea, acumula el tiempo de trabajo en ella:

```bash
./pomodoro -t "Study English"
```

| Tecla | Acción |
|-------|--------|
| `p` / `P` | Pausar / continuar |
| `q` / `Q` | Salir del programa |
| `c` / `C` | Saltar a la siguiente fase |

### Gestión de tareas

| Comando | Acción |
|---------|--------|
| `./pomodoro -c "Task"` | Crear una tarea |
| `./pomodoro -d "Task"` | Eliminar una tarea |
| `./pomodoro -s` / `--show` | Mostrar el tiempo acumulado por tarea |
| `./pomodoro -t "Task"` | Trabajar en una tarea (la crea si no existe) |
| `./pomodoro -w 50` | Pomodoro libre con 50 min de trabajo |
| `./pomodoro -b 10` | Descanso de 10 min |
| `./pomodoro -l 20` | Descanso largo de 20 min |
| `./pomodoro -h` / `--help` | Mostrar la ayuda completa |

## Persistencia

El tiempo de trabajo de cada tarea se guarda en `tasks.json` en el directorio de
ejecución:

```json
{
  "tasks": [
    {
      "name": "Study English",
      "seconds": 480
    }
  ]
}
```

El archivo se escribe al salir del programa. `Ctrl+C` también guarda el avance
(no se pierde el tiempo acumulado).

## Estructura del proyecto

```
src/
├── main.cpp      # orquestación: parseo de opciones y dispatch
├── tasks.h       # declaraciones del módulo de tareas
├── tasks.cpp     # implementación: Task, JSON, crear/listar/eliminar
├── pomodoro.h    # declaraciones de la clase Pomodoro
└── pomodoro.cpp  # implementación: reloj, terminal, señales
```

## Licencia

MIT — ver [LICENSE](LICENSE).