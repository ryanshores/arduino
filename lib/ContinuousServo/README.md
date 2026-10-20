# ContinuousServo

Controls a continuous-rotation servo using speed percentages (0 to 100).
Values outside that range are clamped. Percentages represent the control
signal range, not a measured RPM; physical speed is not necessarily linear.

```cpp
#include <ContinuousServo.h>
ContinuousServo motor;

void setup() {
    motor.begin(9);           // Starts stopped; neutral defaults to 90
    motor.forwardFor(50, 500); // Forward at 50% for 500 ms, then stop
}

void loop() {
    motor.update(); // Required for automatic timed stops
}
```

- `forward(50)` / `back(50)`: keep moving until another command.
- `stop()`: send neutral, keeping the servo attached.
- `forwardFor(50, 500)` / `backFor(50, 500)`: stop after 500 ms.
- New movement commands replace any pending timed stop.
- Zero speed means neutral; zero duration stops immediately.
- `begin(9, 92)` changes neutral if the servo creeps at 90.
- `begin(9, 90, false)` reverses which direction is called forward.
- Forward defaults to clockwise, matching the observed servo.

Keep calling `update()` frequently: blocking delays postpone timed stops.
The default mode does not use `delay()`. Stopping keeps the servo attached.

For sequential movement in `setup()`, explicitly use blocking mode:

```cpp
motor.forwardFor(50, 500, true); // Wait 500 ms, stop, then return
motor.backFor(50, 1000, true);   // Then reverse for 1000 ms and stop
```

Blocking mode pauses button handling and other sketch code. Without `true`,
commands return immediately; a second command replaces the first rather than
queuing behind it. Call `update()` frequently for these nonblocking commands.

Logging is disabled by default. To enable it, call `Serial.begin(115200)` in
your sketch and `motor.setLogger(&Serial)` before `begin()`. Use
`motor.setLogger(nullptr)` to disable it. Commands are sent before logging,
but slow serial output can still delay subsequent code and timed updates.

To use this shared folder from the current project:

```ini
lib_deps =
    symlink://lib/ContinuousServo
```

From another project, use a relative or absolute `symlink://` path to this
same folder. Servo is installed automatically as a dependency.
