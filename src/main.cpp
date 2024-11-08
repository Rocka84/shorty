#include <Arduino.h>
#include <USBKeyboard.h>
#include <JC_Button.h>
#include <Encoder.h>
#include <math.h>

#include <Adafruit_NeoPixel.h>
#include <WS2812FX.h>

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// #define DEBUG_LOG
// #define DEBUG_SERIAL

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

#if defined(DEBUG_LOG) && defined(DEBUG_SERIAL)
#include <SoftwareSerial.h>
    SoftwareSerial Debug(12, 13); //rx,tx
#elif defined(DEBUG_LOG)
#define Debug Serial
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

Adafruit_NeoPixel pixels(BUTTON_COUNT, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);
// Adafruit_NeoPixel pixels(8, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

bool backlight = false;
uint32_t color_default = pixels.Color(70, 70, 70);
uint32_t color_off = pixels.Color(0, 0, 0);

int button_pixels[] = { 0, 1, 2, 5, 4, 3 };
int button_colors[] = { 0, 0, 0, 0, 0, 0 };

WS2812FX pixels_effect = WS2812FX(BUTTON_COUNT - 1, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);
bool effect_active = false;
int effect_mode = FX_MODE_BREATH;
int effect_color = 0;
uint32_t effect_speed = 3000;

int colors_count = 7;
uint32_t colors[] = {
    BLUE,
    CYAN,
    GREEN,
    YELLOW,
    RED,
    PURPLE,
    WHITE
};

bool led_latch_handled = false;


void setPixelColor(int button, uint32_t color) {
    pixels.setPixelColor(button_pixels[button], color);
}

void setupEffects() {
    pixels_effect.init();
    pixels_effect.setColor(colors[effect_color]);
    pixels_effect.setSpeed(3000);
    pixels_effect.setMode(effect_mode);
}

void startEffect() {
    if (effect_active) {
        effect_mode = (effect_mode + 1) % (pixels_effect.getModeCount() - 8);
#ifdef DEBUG_LOG
        Debug.print("effect mode "); Debug.print(effect_mode); Debug.print(" "); Debug.println(pixels_effect.getModeName(effect_mode));
#endif
    } else {
        pixels.setBrightness(66);
    }

    effect_active = true;
    pixels_effect.setMode(effect_mode);
    pixels_effect.start();
}

void stopEffect() {
    if (!effect_active) return;
    effect_active = false;
    pixels_effect.stop();
    pixels.setBrightness(10);
}

void handleLedStatus() {
#if defined(DEBUG_LOG) && !defined(DEBUG_SERIAL)
    uint8_t led_status = 0;
#else
    uint8_t led_status = Keyboard.readLedStatus();
#endif

    int latch = led_status >> 4; // bit 5
    if (latch == 1 && led_latch_handled) {
        return;
    }
    if (latch == 0) {
        led_latch_handled = false;
        return;
    }


#ifdef DEBUG_LOG
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
            if (button_colors[data - 1] > colors_count - 1) button_colors[data - 1] = 0;

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

            stopEffect();
            effect_mode = FX_MODE_BREATH;
        } else { // 0b1111
            if (!effect_active) {
                startEffect();
            } else {
                stopEffect();
            }
        }
    }
}

void sendButtonKey(int index){
// #ifdef DEBUG_LOG
//     return;
// #endif

    if (button_keys[index] == 0) return;

#if defined(DEBUG_LOG) && !defined(DEBUG_SERIAL)
    return;
#endif
    Keyboard.sendKeyStroke(button_keys[index]);
}

bool handleButtonCombos() {
    if (buttons[3].isPressed() && buttons[0].wasReleased()) {
        backlight = !backlight;
        buttons_suppressed[3] = true;

#ifdef DEBUG_LOG
        Debug.print("backlight switched "); Debug.println(backlight?"on":"off");
#endif
        return true;
    }

    if (buttons[3].isPressed() && buttons[5].wasReleased()) {
        startEffect();
        buttons_suppressed[3] = true;

#ifdef DEBUG_LOG
        Debug.println("effect switched on");
#endif
        return true;
    }

    if (buttons[3].isPressed() && buttons[4].wasReleased()) {
        stopEffect();
        buttons_suppressed[3] = true;

#ifdef DEBUG_LOG
        Debug.println("effect switched off");
#endif
        return true;
    }

    if (effect_active && buttons[3].isPressed() && buttons[2].wasReleased()) {
        effect_color = (effect_color + 1) % colors_count;
        pixels_effect.setColor(colors[effect_color]);
        buttons_suppressed[3] = true;
        return true;
    }

    if (effect_active && buttons[3].isPressed() && buttons[1].wasReleased()) {
        effect_speed -= 1000;
        if (effect_speed < 1000) effect_speed = 10000;
        pixels_effect.setSpeed(effect_speed);
        buttons_suppressed[3] = true;
        return true;
    }

#if defined(DEBUG_LOG) && !defined(DEBUG_SERIAL)
    if (buttons[0].wasReleased()){
        if (!buttons_lit[0]) {
            buttons_lit[0] = true;
            button_colors[0] = 0;
        } else if (++button_colors[0] >= colors_count) {
            buttons_lit[0] = false;
            button_colors[0] = 0;
        }
        return true;
    }
#endif

    return false;
}

void handleButton(int i) {
    if (buttons[i].pressedFor(1000) && !buttons_suppressed[i]) {
        sendButtonKey(i + BUTTON_COUNT);
        setPixelColor(i, PURPLE);
        buttons_suppressed[i] = true;

#ifdef DEBUG_LOG
        Debug.print("long-press "); Debug.println(i);
#endif

    } else if (buttons[i].wasReleased() && !buttons_suppressed[i]) {
        sendButtonKey(i);
        setPixelColor(i, CYAN);

#ifdef DEBUG_LOG
        Debug.print("press "); Debug.println(i);
#endif

    } else if (buttons[i].isPressed()) {
        setPixelColor(i, BLUE);

    } else if (buttons_lit[i]) {
        setPixelColor(i, colors[button_colors[i]]);

    } else if(effect_active) {
        //skip

    } else if(backlight) {
        setPixelColor(i, color_default);

    } else {
        setPixelColor(i, color_off);

    }

    if (buttons[i].wasReleased()) {
        buttons_suppressed[i] = false;
    }
}

void handleButtons() {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].read();
    }

    if (handleButtonCombos()){
        return;
    }

    for (int i = 0; i < BUTTON_COUNT; i++) {
        handleButton(i);
    }
}

void handleRotary() {
    int rotary_pos_new = rotary.read();
    if (rotary_pos_new != rotary_pos && rotary_pos_new % 4 == 0) {
#ifdef DEBUG_LOG
        Debug.println(); Debug.print("rotary "); Debug.print(rotary_pos_new);
        if (rotary_pos_new > rotary_pos) Debug.println(" >>");
        else Debug.println(" <<");
#endif
        if (rotary_pos_new > rotary_pos) {
            Keyboard.sendKeyStroke(rotary_key_cw);
        } else {
            Keyboard.sendKeyStroke(rotary_key_ccw);
        }

        rotary_pos = rotary_pos_new;
    }
}

void setup() {
#ifdef DEBUG_LOG
    Debug.begin(9600);
#endif
    for (int i = 0; i < BUTTON_COUNT; i++) {
        pinMode(button_pins[i], INPUT_PULLUP);
        buttons[i].begin();
    }

#if !defined(DEBUG_LOG) || defined(DEBUG_SERIAL)
    Keyboard.init();
#endif
    pixels.begin();
    pixels.clear();
    setupEffects();
    pixels.setBrightness(10);
    // startEffect();
}

int loop_cnt=0;
void loop() {
    handleLedStatus();
    handleRotary();
    handleButtons();

    if (effect_active) pixels_effect.service();
    else pixels.show();

    delay(5);
}

