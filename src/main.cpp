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

int button_keys[] = { KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F,  // short presses
                        KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L };  // long presses
// int button_keys[] = { KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18,  // short presses
//                         KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24 };  // long presses

Button buttons[] = { Button(PIN_BTN_A), Button(PIN_BTN_B), Button(PIN_BTN_C),
                     Button(PIN_BTN_D), Button(PIN_BTN_E), Button(PIN_BTN_F), };

bool buttons_suppressed[] = { false, false, false, false, false, false };

Adafruit_NeoPixel pixels(BUTTON_COUNT, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

bool light_on = false;
uint32_t color_default = pixels.Color(3, 3, 3);
uint32_t color_off = pixels.Color(0, 0, 0);

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
    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].read();

        if (i == 0 && buttons[0].wasReleased() && buttons[BUTTON_COUNT-1].isPressed()) {
            light_on = !light_on;
            buttons_suppressed[0] = true;
            buttons_suppressed[BUTTON_COUNT-1] = true;
            i = BUTTON_COUNT;
        }

        if (buttons[i].pressedFor(1000) && !buttons_suppressed[i]) {
            sendButtonKey(i + BUTTON_COUNT);
            pixels.setPixelColor(i, pixels.Color(0, 50, 50));
            buttons_suppressed[i] = true;

        } else if (buttons[i].wasReleased() && !buttons_suppressed[i]) {
            sendButtonKey(i);
            pixels.setPixelColor(i, pixels.Color(0, 0, 50));

        } else if (buttons[i].isPressed()) {
            pixels.setPixelColor(i, pixels.Color(0, 30, 0));

        } else if(light_on) {
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

