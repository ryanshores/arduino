/*
 * Button-Push
 * Hold the button to flash the built-in LED and drive the continuous servo.
 *
 * Target: Arduino Nano (ATmega328P). Everything here is cooperative and
 * non-blocking: no delay(), no dynamic allocation, no unbounded buffers.
 */

#include <Arduino.h>
#include <ContinuousServo.h>

// #include <IRremote.hpp>
// #define DECODE_NEC       // IR protocol (Apple and Onkyo) -- only with IRremote

// ---------------------------------------------------------------- pins ----
// Servo.h owns Timer1 on the Nano, which disables analogWrite() on pins 9/10.
constexpr uint8_t SERVO_PIN  = 9;
constexpr uint8_t BUTTON_PIN = 12;  // active low via INPUT_PULLUP

// -------------------------------------------------------------- tuning ----
constexpr unsigned long LED_FLASH_INTERVAL_MS = 200;
constexpr unsigned long BUTTON_DEBOUNCE_MS    = 25;
constexpr uint8_t       SERVO_SPEED_PERCENT   = 10;
constexpr uint16_t      SERVO_STEP_MS         = 1000;
constexpr unsigned long SERIAL_BAUD           = 115200;  // matches monitor_speed

// ------------------------------------------------- servo start-up sweep ----
// Fixed-length table in flash. Read one entry at a time into a stack copy, so
// RAM cost is sizeof(ServoStep) regardless of how many steps are added.
struct ServoStep {
    bool     forward;
    uint8_t  speedPercent;
    uint16_t durationMs;
};

static const ServoStep SERVO_SELF_TEST[] PROGMEM = {
    { true,  SERVO_SPEED_PERCENT, SERVO_STEP_MS },
    { false, SERVO_SPEED_PERCENT, SERVO_STEP_MS },
};
constexpr uint8_t SERVO_SELF_TEST_LEN =
    sizeof(SERVO_SELF_TEST) / sizeof(SERVO_SELF_TEST[0]);

// --------------------------------------------------------------- state ----
static ContinuousServo servo;

static uint8_t       selfTestIndex      = 0;
static unsigned long selfTestStepMs     = 0;
static bool          selfTestRunning    = false;

static bool          ledIsOn            = false;
static unsigned long lastLedToggleMs    = 0;

static bool          buttonPressed      = false;  // debounced
static bool          buttonRawPressed   = false;  // last raw sample
static unsigned long buttonLastChangeMs = 0;

static bool          servoRunning       = false;

// ------------------------------------------------------------- helpers ----
static ServoStep readServoStep(uint8_t index) {
    ServoStep step;
    memcpy_P(&step, &SERVO_SELF_TEST[index], sizeof(step));
    return step;
}

static void beginServoStep(uint8_t index, unsigned long now) {
    const ServoStep step = readServoStep(index);
    if (step.forward) {
        servo.forward(step.speedPercent);
    } else {
        servo.back(step.speedPercent);
    }
    selfTestStepMs = now;
}

// True while the start-up sweep still owns the servo.
static bool updateServoSelfTest(unsigned long now) {
    if (!selfTestRunning) {
        return false;
    }
    if (now - selfTestStepMs < readServoStep(selfTestIndex).durationMs) {
        return true;
    }
    if (++selfTestIndex >= SERVO_SELF_TEST_LEN) {
        selfTestRunning = false;
        servo.stop();
        return false;
    }
    beginServoStep(selfTestIndex, now);
    return true;
}

// Sampled every pass; the debounced state only follows the raw level once it
// has held steady for BUTTON_DEBOUNCE_MS.
static bool updateButton(unsigned long now) {
    const bool rawPressed = (digitalRead(BUTTON_PIN) == LOW);
    if (rawPressed != buttonRawPressed) {
        buttonRawPressed   = rawPressed;
        buttonLastChangeMs = now;
    } else if (now - buttonLastChangeMs >= BUTTON_DEBOUNCE_MS) {
        buttonPressed = rawPressed;
    }
    return buttonPressed;
}

// Edge triggered: ContinuousServo logs a line on every forward()/stop(), so
// calling it unconditionally would flood the 64-byte serial TX buffer and
// stall loop() once that buffer fills.
static void setServoRunning(bool run) {
    if (run == servoRunning) {
        return;
    }
    servoRunning = run;
    if (run) {
        servo.forward(SERVO_SPEED_PERCENT);
    } else {
        servo.stop();
    }
}

static void setLed(bool on) {
    ledIsOn = on;
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
}

// --------------------------------------------------------------- setup ----
void setup() {
    Serial.begin(SERIAL_BAUD);  // no while(!Serial): the Nano has no USB CDC
    Serial.println(F("[main] setup"));

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    setLed(false);

    servo.setLogger(&Serial);
    if (!servo.begin(SERVO_PIN)) {
        Serial.println(F("[main] servo attach failed"));
    } else if (SERVO_SELF_TEST_LEN > 0) {
        selfTestIndex   = 0;
        selfTestRunning = true;
        beginServoStep(0, millis());
    }
}

// ---------------------------------------------------------------- loop ----
void loop() {
    const unsigned long now = millis();  // one timebase for the whole pass

    servo.update();                      // retires any timed library command
    const bool pressed = updateButton(now);

    if (updateServoSelfTest(now)) {
        return;                          // sweep owns the servo; LED stays off
    }

    if (pressed) {
        if (now - lastLedToggleMs >= LED_FLASH_INTERVAL_MS) {
            lastLedToggleMs = now;
            setLed(!ledIsOn);
            setServoRunning(true);
        }
    } else {
        if (ledIsOn) {
            setLed(false);
        }
        setServoRunning(false);
        // Pre-age the timer so the next press toggles on the first pass.
        lastLedToggleMs = now - LED_FLASH_INTERVAL_MS;
    }
}
