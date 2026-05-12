#include <Bluepad32.h>
#include "driver/gpio.h"

/* 
 * CONFIGURATION
 * 
 */

#define NES_LATCH_PIN     25
#define NES_CLOCK_PIN     26
#define NES_DATA_PIN      33

/* NES button bit layout
 *
 * Bit:  7      6      5        4       3    2      1      0
 *       A      B      SELECT  START   UP   DOWN   LEFT   RIGHT
 */

#define NES_BTN_A         0x80
#define NES_BTN_B         0x40
#define NES_BTN_SELECT    0x20
#define NES_BTN_START     0x10
#define NES_BTN_UP        0x08
#define NES_BTN_DOWN      0x04
#define NES_BTN_LEFT      0x02
#define NES_BTN_RIGHT     0x01

#define NES_NO_BUTTONS    0xFF


/* 
 * GLOBAL STATE
 * 
 */

volatile uint8_t nesState = NES_NO_BUTTONS;
volatile uint8_t shiftReg = NES_NO_BUTTONS;
volatile uint8_t shiftCount = 0;

ControllerPtr controllers[BP32_MAX_GAMEPADS];


/* 
 * FAST GPIO
 * 
 */

static inline void IRAM_ATTR nesDataWrite(bool high) {

    gpio_set_level(
        (gpio_num_t) NES_DATA_PIN,
        high ? 1 : 0
    );
}


/* 
 * NES SHIFT REGISTER EMULATION
 * 
 */

void IRAM_ATTR onLatch() {

    shiftReg = nesState;
    shiftCount = 0;

    /* Send first bit immediately */
    nesDataWrite(shiftReg & NES_BTN_A);
}

void IRAM_ATTR onClock() {

    shiftCount++;

    if (shiftCount >= 8) {
        return;
    }

    shiftReg <<= 1;

    nesDataWrite(shiftReg & NES_BTN_A);
}


/* 
 * CONTROLLER CALLBACKS
 * 
 */

void onConnectedController(ControllerPtr ctl) {

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {

        if (controllers[i] == nullptr) {

            controllers[i] = ctl;

            Serial.println("[BT] Controller connected");

            break;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {

        if (controllers[i] == ctl) {

            controllers[i] = nullptr;

            nesState = NES_NO_BUTTONS;

            Serial.println("[BT] Controller disconnected");

            break;
        }
    }
}


/* 
 * BUTTON MAPPING
 * 
 */

static inline uint8_t buildNesState(ControllerPtr ctl) {

    uint8_t state = NES_NO_BUTTONS;

    /* FACE BUTTONS */

    if (ctl->a()) {
        state &= ~0x02;
    }

    if (ctl->b()) {
        state &= ~0x04;
    }

    if (ctl->x()) {
        state &= ~0x08;
    }

    if (ctl->y()) {
        state &= ~0x10;
    }

    /* DPAD */

    uint8_t dpad = ctl->dpad();

    if (dpad & DPAD_UP) {
        state &= ~0x20;
    }

    if (dpad & DPAD_DOWN) {
        state &= ~0x40;
    }

    if (dpad & DPAD_LEFT) {
        state &= ~0x80;
    }

    if (dpad & DPAD_RIGHT) {
        state &= ~0x01;
    }

    return state;
}


/* 
 * UPDATE NES STATE
 * 
 */

void updateNesState() {

    for (auto ctl : controllers) {

        if (!ctl) {
            continue;
        }

        if (!ctl->isConnected()) {
            continue;
        }

        nesState = buildNesState(ctl);

        return;
    }

    nesState = NES_NO_BUTTONS;
}


/* 
 * SETUP
 * 
 */

void setup() {

    Serial.begin(115200);

    pinMode(NES_DATA_PIN, OUTPUT);
    pinMode(NES_LATCH_PIN, INPUT_PULLUP);
    pinMode(NES_CLOCK_PIN, INPUT_PULLUP);

    digitalWrite(NES_DATA_PIN, HIGH);

    attachInterrupt(
        digitalPinToInterrupt(NES_LATCH_PIN),
        onLatch,
        FALLING
    );

    attachInterrupt(
        digitalPinToInterrupt(NES_CLOCK_PIN),
        onClock,
        RISING
    );

    BP32.setup(
        &onConnectedController,
        &onDisconnectedController
    );

    BP32.enableVirtualDevice(false);

    Serial.println("[INIT] Waiting for controller...");
}


/* 
 * MAIN LOOP
 * 
 */

void loop() {

    BP32.update();

    updateNesState();

    vTaskDelay(1);
}