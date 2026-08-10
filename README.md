# Pomodoro CLI

Temporizador Pomodoro de terminal en C++ con reloj de arte ASCII, ciclos de
trabajo/descanso, pausa y notificaciones de escritorio.

## Características

- Reloj grande de arte ASCII con formato `HH:MM:SS`
- Ciclos de **trabajo** → **descanso** → ... con descanso largo cada 4 ciclos
- Pausa con `p`, salida limpia con `q`, salto de fase con `c`
- Notificaciones de escritorio con `notify-send`
- Restauración automática de la terminal al salir (incluso con Ctrl+C)
- Centrado dinámico según el tamaño de la ventana

## Requisitos

- Linux (usa `termios`, `poll`, `ioctl`)
- `g++` con soporte C++17
- `notify-send` (opcional, para las notificaciones)

## Compilar y ejecutar

```bash
make          # compila → genera el binario pomodoro
make run      # compila y ejecuta
make clean    # borra el binario
```

O manualmente:

```bash
g++ -std=c++17 -Wall -Wextra -o pomodoro src/main.cpp
./pomodoro
```

## Uso

| Tecla | Acción |
|-------|--------|
| `p` / `P` | Pausar / continuar |
| `q` / `Q` | Salir del programa |
| `c` / `C` | Saltar a la siguiente fase |

## Configuración

Los tiempos se configuran en `main.cpp`:

```cpp
Pomodoro timer(120, 20);   // 120 min de trabajo, 20 min de descanso
timer.start();
```

El tercer argumento (opcional) es el descanso largo (por defecto 15 min):

```cpp
Pomodoro timer(25, 5, 20); // 25 min trabajo, 5 min descanso, 20 min descanso largo
```

## Licencia

MIT — ver [LICENSE](LICENSE).
