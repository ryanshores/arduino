#include "ContinuousServo.h"

bool ContinuousServo::begin(uint8_t pin, int neutral, bool forwardClockwise) {
    neutral_ = constrain(neutral, 1, 179);
    forwardClockwise_ = forwardClockwise;
    servo_.write(neutral_);
    servo_.attach(pin);
    stop();
    if (logger_) {
        logger_->print(F("[Servo] Begin pin: "));
        logger_->print(pin);
        logger_->print(F(" neutral: "));
        logger_->println(neutral_);
    }
    return servo_.attached();
}

void ContinuousServo::move(int percent, bool forward) {
    timed_ = false;
    percent = constrain(percent, 0, 100);
    const bool clockwise = forward == forwardClockwise_;
    const int endpoint = clockwise ? 0 : 180;
    servo_.write(neutral_ + (endpoint - neutral_) * percent / 100);
    if (logger_) {
        logger_->print(F("[Servo] ms: "));
        logger_->print(millis());
        logger_->print(forward ? F(" Forward ") : F(" Back "));
        logger_->print(percent);
        logger_->println(F("%"));
    }
}

void ContinuousServo::forward(int percent) { move(percent, true); }
void ContinuousServo::back(int percent) { move(percent, false); }

void ContinuousServo::stop() {
    timed_ = false;
    servo_.write(neutral_);
    if (logger_) {
        logger_->print(F("[Servo] Stop ms: "));
        logger_->println(millis());
    }
}

void ContinuousServo::moveFor(int percent, bool forward, unsigned long durationMs, bool withDelay) {
    if (durationMs == 0) { stop(); return; }
    // Start timing before move(), since its optional logging can take time.
    startedAt_ = millis();
    move(percent, forward);
    durationMs_ = durationMs;
    timed_ = true;
    if (withDelay) {
        const unsigned long elapsed = millis() - startedAt_;
        if (elapsed < durationMs_) delay(durationMs_ - elapsed);
        stop();
    }
}

void ContinuousServo::forwardFor(int percent, unsigned long durationMs, bool withDelay) {
    moveFor(percent, true, durationMs, withDelay);
}

void ContinuousServo::backFor(int percent, unsigned long durationMs, bool withDelay) {
    moveFor(percent, false, durationMs, withDelay);
}

void ContinuousServo::update() {
    if (timed_ && millis() - startedAt_ >= durationMs_) stop();
}
