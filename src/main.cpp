#include <Arduino.h>
#include <USBKeyboard.h>
#include <JC_Button.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define BUTTON_COUNT 6
#define PIN_NEOPIXELS 6

#define PIN_BTN_A   2
#define PIN_BTN_B   3
#define PIN_BTN_C   4
#define PIN_BTN_D   5
#define PIN_BTN_E   7
#define PIN_BTN_F   8

byte button_pins[] = {PIN_BTN_A, PIN_BTN_B, PIN_BTN_C,
                        PIN_BTN_D, PIN_BTN_E, PIN_BTN_F};

// int button_keys[] = { KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F,  // short presses
//                         KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L };  // long presses
int button_keys[] = { KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18,  // short presses
                        KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24 };  // long presses

Button buttons[] = { Button(PIN_BTN_A), Button(PIN_BTN_B), Button(PIN_BTN_C),
                     Button(PIN_BTN_D), Button(PIN_BTN_E), Button(PIN_BTN_F), };

bool buttons_suppressed[] = { false, false, false, false, false, false };
bool buttons_lit[] = { false, false, false, false, false, false };

Adafruit_NeoPixel pixels(BUTTON_COUNT, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

bool backlight = false;
uint32_t color_default = pixels.Color(3, 3, 3);
uint32_t color_lit = pixels.Color(0, 0, 30);
uint32_t color_off = pixels.Color(0, 0, 0);

bool led_latch_handled = false;

void handleLedStatus() {
    uint8_t led_status = Keyboard.readLedStatus();

    int latch = led_status >> 3;
    if (latch == 1 && led_latch_handled) {
        return;
    }
    if (latch == 0) {
        led_latch_handled = false;
        return;
    }

    led_latch_handled = true;
    int data = led_status & 7;

    if (data == 0) { // b000
        backlight = !backlight;
    } else if (data >= 1 && data <= 6) { // b001 - b110
        buttons_lit[data - 1] = !buttons_lit[data - 1];
    } else if (data == 7) { // b111
        for (int i = 0; i < BUTTON_COUNT; i++) {
            buttons_lit[i] = false;
        }
        backlight = false;
    }
}

void sendButtonKey(int index){
    Keyboard.sendKeyStroke(button_keys[index]);
}


void setup() {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].begin();
    }

    Keyboard.init();
    pixels.begin();
    pixels.clear();
}

void loop() {
    handleLedStatus();

    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].read();

        if (i == 0 && buttons[0].isPressed() && buttons[BUTTON_COUNT-1].isPressed()) {
            backlight = !backlight;
            buttons_suppressed[0] = true;
            buttons_suppressed[BUTTON_COUNT-1] = true;
            i = BUTTON_COUNT;
        }

        if (buttons[i].pressedFor(1000) && !buttons_suppressed[i]) {
            sendButtonKey(i + BUTTON_COUNT);
            pixels.setPixelColor(i, pixels.Color(0, 20, 20));
            buttons_suppressed[i] = true;

        } else if (buttons[i].wasReleased() && !buttons_suppressed[i]) {
            sendButtonKey(i);
            pixels.setPixelColor(i, pixels.Color(0, 0, 30));

        } else if (buttons[i].isPressed()) {
            pixels.setPixelColor(i, pixels.Color(0, 20, 0));

        } else if (buttons_lit[i]) {
            pixels.setPixelColor(i, color_lit);

        } else if(backlight) {
            pixels.setPixelColor(i, color_default);

        } else {
            pixels.setPixelColor(i, color_off);
        }

        if (buttons[i].wasReleased()) {
            buttons_suppressed[i] = false;
        }
    }

    pixels.show();

    delay(100);
}

