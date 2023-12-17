#include <Arduino.h>
#include <USBKeyboard.h>
#include <JC_Button.h>
#include <Encoder.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// #define SERIALDBG

#define BUTTON_COUNT 6
#define PIN_BTN_A   4
#define PIN_BTN_B   3
#define PIN_BTN_C   2
#define PIN_BTN_D   7
#define PIN_BTN_E   6
#define PIN_BTN_F   5

#define PIN_ROTARY_CLK 11
#define PIN_ROTARY_DT 10

#define PIN_NEOPIXELS 9

#ifdef SERIALDBG
#include <SoftwareSerial.h>
SoftwareSerial Debug(12, 13); //rx,tx
#endif

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

Encoder rotary(PIN_ROTARY_DT, PIN_ROTARY_CLK);

int rotary_pos=0;
int rotary_key_cw = KEY_VOLUME_UP;
int rotary_key_ccw = KEY_VOLUME_DOWN;

// Adafruit_NeoPixel pixels(BUTTON_COUNT, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels(8, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

bool backlight = false;
uint32_t color_default = pixels.Color(3, 3, 3);
uint32_t color_off = pixels.Color(0, 0, 0);

int button_pixels[] = { 0, 1, 2, 5, 4, 3 };
int button_colors[] = { 0, 0, 0, 0, 0, 0 };
uint32_t colors[] = {
    pixels.Color(0, 0, 30),
    pixels.Color(0, 20, 20),
    pixels.Color(0, 30, 0),
    pixels.Color(20, 20, 0),
    pixels.Color(30, 0, 0),
    pixels.Color(20, 0, 20),
    pixels.Color(12, 12, 12)
};

bool led_latch_handled = false;

void handleLedStatus() {
    uint8_t led_status = Keyboard.readLedStatus();

    int latch = led_status >> 4; // bit 5
    if (latch == 1 && led_latch_handled) {
        return;
    }
    if (latch == 0) {
        led_latch_handled = false;
        return;
    }


#ifdef SERIALDBG
    Debug.println("led command received");
#endif
    led_latch_handled = true;
    bool target_state = ((led_status >> 3 & 1) == 1); // bit 4
    int data = led_status & 7; // bits 1-3

    if (data == 0) { // 0b0000 / 0b1000
        backlight = target_state;

    } else if (data >= 1 && data <= 6) { // 0bX001 - 0bX110
        if (target_state && !buttons_lit[data - 1]) { // turn on
            buttons_lit[data - 1] = true;
        } else if (target_state) { // already on, next color
            button_colors[data - 1]++;
            if (button_colors[data - 1] > 6) button_colors[data - 1] = 0;

        } else if (buttons_lit[data - 1]) { // turn off
            buttons_lit[data - 1] = false;
        } else { // already off, reset color
            button_colors[data - 1] = 0;
        }

    } else if (data == 7) { // 0bX111
        if (!target_state) { // 0b0111
            for (int i = 0; i < BUTTON_COUNT; i++) {
                buttons_lit[i] = false;
                button_colors[i] = 0;
            }
        } else { // 0b1111
            //@ToDo: add simple effect
        }
    }
}

void sendButtonKey(int index){
    if (button_keys[index] == 0) return;

    Keyboard.sendKeyStroke(button_keys[index]);
}

void setPixelColor(int button, uint32_t color) {
        pixels.setPixelColor(button_pixels[button], color);
}

void handleButton(int i) {
    buttons[i].read();

    if (i == 0 && buttons[0].isPressed() && buttons[BUTTON_COUNT-1].isPressed()) {
        backlight = !backlight;
        buttons_suppressed[0] = true;
        buttons_suppressed[BUTTON_COUNT-1] = true;
        i = BUTTON_COUNT;

#ifdef SERIALDBG
        Debug.print("backlight switched to ");
        Debug.println(backlight);
#endif
    }

    if (buttons[i].pressedFor(1000) && !buttons_suppressed[i]) {
        sendButtonKey(i + BUTTON_COUNT);
        setPixelColor(i, pixels.Color(0, 20, 20));
        buttons_suppressed[i] = true;

#ifdef SERIALDBG
        Debug.print("long-press ");
        Debug.println(i);
#endif

    } else if (buttons[i].wasReleased() && !buttons_suppressed[i]) {
        sendButtonKey(i);
        setPixelColor(i, pixels.Color(20, 0, 20));

#ifdef SERIALDBG
        Debug.print("press ");
        Debug.println(i);
#endif

    } else if (buttons[i].isPressed()) {
        setPixelColor(i, pixels.Color(20, 0, 0));

    } else if (buttons_lit[i]) {
        setPixelColor(i, colors[button_colors[i]]);

#ifdef SERIALDBG
        if (i==5) {
            Debug.print("buttons_lit ");
            Debug.println(i);
        }
#endif

    } else if(backlight) {
        setPixelColor(i, color_default);

    } else {
        setPixelColor(i, color_off);

    }

    if (buttons[i].wasReleased()) {
        buttons_suppressed[i] = false;
    }
}

void setup() {
#ifdef SERIALDBG
    Debug.begin(9600);
#endif
    for (int i = 0; i < BUTTON_COUNT; i++) {
        pinMode(button_pins[i], INPUT_PULLUP);
        buttons[i].begin();
    }

    Keyboard.init();
    pixels.begin();
    pixels.clear();
}

int loop_cnt=0;
void loop() {
    handleLedStatus();

    int rotary_pos_new = rotary.read();
    if (rotary_pos_new != rotary_pos) {
#ifdef SERIALDBG
        Debug.print("rotary ");
        Debug.print(rotary_pos_new);
        if (rotary_pos_new > rotary_pos) {
            Debug.println(" >>");
        } else {
            Debug.println(" <<");
        }
#endif
        if (rotary_pos_new > rotary_pos) {
            Keyboard.sendKeyStroke(rotary_key_cw);
        } else {
            Keyboard.sendKeyStroke(rotary_key_ccw);
        }

        rotary_pos = rotary_pos_new;
    }

    if (++loop_cnt>10) {
        loop_cnt=0;

        for (int i = 0; i < BUTTON_COUNT; i++) {
            handleButton(i);
        }

        pixels.show();
    }

    // delay(50);
}

