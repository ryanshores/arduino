#pragma once

#include <Arduino.h>
#include <Servo.h>

class ContinuousServo {
public:
    // Forward defaults to clockwise for this servo. Neutral can be calibrated.
    bool begin(uint8_t pin, int neutral = 90, bool forwardClockwise = true);
    void forward(int percent);
    void back(int percent);
    void stop();

    // Optional output; caller initializes Serial (or another Print implementation).
    void setLogger(Print* logger) { logger_ = logger; }

    // Default: return immediately; call update() regularly.
    // withDelay=true: wait for the duration and stop before returning.
    void forwardFor(int percent, unsigned long durationMs, bool withDelay = false);
    void backFor(int percent, unsigned long durationMs, bool withDelay = false);
    void update();

private:
    void move(int percent, bool forward);
    void moveFor(int percent, bool forward, unsigned long durationMs, bool withDelay);
    Print* logger_ = nullptr;
    Servo servo_;
    int neutral_ = 90;
    bool forwardClockwise_ = true;
    bool timed_ = false;
    unsigned long startedAt_ = 0;
    unsigned long durationMs_ = 0;
};
