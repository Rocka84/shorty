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

bool backlight = false;
uint32_t color_default = pixels.Color(70, 70, 70);
uint32_t color_off = pixels.Color(0, 0, 0);

int button_pixels[] = { 0, 1, 2, 5, 4, 3 };
int button_colors[] = { 0, 0, 0, 0, 0, 0 };

WS2812FX pixels_effect = WS2812FX(BUTTON_COUNT - 1, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);
bool effect_active = false;
int effect_color = 0;
uint32_t effect_speed = 3000;

int effects[] = {
    FX_MODE_BREATH,
    FX_MODE_RAINBOW,
    FX_MODE_FIRE_FLICKER,
    FX_MODE_FADE,
    FX_MODE_SCAN,
    FX_MODE_CHASE_COLOR
};

int effects_count = 6;
int effects_index = 0;

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

void flashPixel(int button, uint32_t color) {
    if (effect_active) return;
    setPixelColor(button, color);
    pixels.show();
    delay(80);
}

void setupEffects() {
    pixels_effect.init();
    pixels_effect.setColor(colors[effect_color]);
    pixels_effect.setSpeed(effect_speed);
    pixels_effect.setMode(effects[effects_index]);
}

void startEffect() {
    if (effect_active) {
        effects_index = (effects_index + 1) % effects_count;
        pixels_effect.setColor(colors[effect_color]);
        pixels_effect.setSpeed(effect_speed);

        // special defaults for some effects
        if (effects[effects_index] == FX_MODE_FIRE_FLICKER) {
            pixels_effect.setColor(RED);
            pixels_effect.setSpeed(1000);
        } else if (effects[effects_index] == FX_MODE_RAINBOW) {
            pixels_effect.setSpeed(8000);
        } else if (effects[effects_index] == FX_MODE_SCAN && effect_speed > 2000) {
            pixels_effect.setSpeed(2000);
        } else if (effects[effects_index] == FX_MODE_CHASE_COLOR && effect_speed > 2000) {
            pixels_effect.setSpeed(2000);
        }

#ifdef DEBUG_LOG
        Debug.print("effect mode "); Debug.print(effects_index); Debug.print(" "); Debug.print(effects[effects_index]); Debug.print(" "); Debug.println(pixels_effect.getModeName(effects[effects_index]));
#endif
    } else {
        pixels.setBrightness(66);
    }

    effect_active = true;
    pixels_effect.setMode(effects[effects_index]);
    pixels_effect.start();
}

void stopEffect() {
    if (!effect_active) return;
    effect_active = false;
    pixels_effect.stop();
    pixels.setBrightness(10);
}

void nextEffectColor() {
    effect_color = (effect_color + 1) % colors_count;
    pixels_effect.setColor(colors[effect_color]);
#ifdef DEBUG_LOG
    Debug.print("effect color "); Debug.print(effect_color); Debug.print(" 0x"); Debug.println(colors[effect_color], HEX);
#endif
}

void nextEffectSpeed() {
    effect_speed -= 1000;
    if (effect_speed < 1000 || effect_speed > 10000) effect_speed = 10000;
    pixels_effect.setSpeed(effect_speed);
#ifdef DEBUG_LOG
    Debug.print("effect speed "); Debug.println(effect_speed);
#endif
}

/*
 * commands:
 * 0    change backlight
 * 1-6  change button light
 * 7    effect (param 1) / reset (param 0)
 *
 * exemptions while effect is on:
 * 1    next effect
 * 2    next effect color
 * 4    next effect speed
 */
void handleLedStatus() {
    uint8_t led_status;

#if defined(DEBUG_LOG) && !defined(DEBUG_SERIAL)
    if (Debug.available() == 0) return;

    int read = Debug.read();
    if (read >= 48 && read <= 57) {
        led_status = read - 48;
    } else if (read >= 97 && read <= 102) {
        led_status = read - 87;
    } else {
        return;
    }

    Debug.print("serial read ");
    Debug.print(led_status >> 3 & 1 ? '1' : '0');
    Debug.print(led_status >> 2 & 1 ? '1' : '0');
    Debug.print(led_status >> 1 & 1 ? '1' : '0');
    Debug.print(led_status >> 0 & 1 ? '1' : '0');
    Debug.print(" ("); Debug.print(led_status, HEX); Debug.println(")");

#else
    led_status = Keyboard.readLedStatus();

    int latch = led_status >> 4; // bit 5
    if (latch == 1 && led_latch_handled) {
        return;
    }
    if (latch == 0) {
        led_latch_handled = false;
        return;
    }
#endif


    led_latch_handled = true;
    int command = led_status & 7; // bits 1-3
    bool param = ((led_status >> 3 & 1) == 1); // bit 4

#ifdef DEBUG_LOG
    Debug.print("led command received "); Debug.print(command); Debug.print(" param "); Debug.println(param);
#endif

    if (effect_active) {
        if (command == 1) { // 0bX001 next effect
            startEffect();
        } else if (command == 2) { // 0bX010
            nextEffectColor();
        } else if (command == 4) { // 0bX100
            nextEffectSpeed();
        }

    } else if (command == 0) { // 0b0000 backlight on / 0b1000 backlight off
        backlight = param;

    } else if (command >= 1 && command <= 6) { // 0bX001 - 0bX110 button on/next/off
        if (param && !buttons_lit[command - 1]) { // turn on
            buttons_lit[command - 1] = true;
        } else if (param) { // already on, next color
            button_colors[command - 1]++;
            if (button_colors[command - 1] > colors_count - 1) button_colors[command - 1] = 0;

        } else if (buttons_lit[command - 1]) { // not off, turn off
            buttons_lit[command - 1] = false;
        } else { // already off, reset color
            button_colors[command - 1] = 0;
        }
    }


    if (command == 7 && param) { // 0b1111 effect
        if (!effect_active) {
            startEffect();
        } else {
            stopEffect();
        }

    } else if (command == 7 && !param) { // 0b0111 reset
        for (int i = 0; i < BUTTON_COUNT; i++) {
            buttons_lit[i] = false;
            button_colors[i] = 0;
        }
        stopEffect();
        effects_index = 0;
        effect_color = 0;
        effect_speed = 3000;
    }
}

void sendButtonKey(int index){
#if defined(DEBUG_LOG) && !defined(DEBUG_SERIAL)
    Debug.print("key "); Debug.println(button_keys[index]);
    return;
#endif
    if (button_keys[index] == 0) return;
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
        nextEffectColor();
        buttons_suppressed[3] = true;
        return true;
    }

    if (effect_active && buttons[3].isPressed() && buttons[1].wasReleased()) {
        nextEffectSpeed();
        buttons_suppressed[3] = true;
        return true;
    }

    return false;
}

void handleButton(int i) {
    if (buttons[i].pressedFor(1000) && !buttons_suppressed[i]) {
#ifdef DEBUG_LOG
        Debug.print("long-press "); Debug.println(i);
#endif

        sendButtonKey(i + BUTTON_COUNT);
        flashPixel(i, PURPLE);
        buttons_suppressed[i] = true;

    } else if (buttons[i].wasReleased() && !buttons_suppressed[i]) {
#ifdef DEBUG_LOG
        Debug.print("press "); Debug.println(i);
#endif

        sendButtonKey(i);
        flashPixel(i, CYAN);

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
    startEffect();
    if (!Keyboard.isConnected()) {
        pixels_effect.setColor(RED);
    }
}

int boot_anim = 9000;

void loop() {
    handleLedStatus();
    handleRotary();
    handleButtons();

    if (effect_active) pixels_effect.service();
    else pixels.show();

    if (effect_active && boot_anim == 5) {
        stopEffect();
    }
    if (boot_anim > 0) {
        boot_anim -= 5;
    }

    delay(5);
}

