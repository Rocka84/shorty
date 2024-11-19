#include <Arduino.h>
#include <USBKeyboard.h>
#include <LightweightRingBuff.h>
#include <JC_Button.h>
#include <Encoder.h>

#include <Adafruit_NeoPixel.h>
#include <WS2812FX.h>

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

#define DIM_75(c) (uint32_t)(((c >> 1) + c) & 0x3f3f3f3f)

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

int colors_count = 7;
uint32_t colors[] = {
    BLUE,
    CYAN,
    GREEN,
    YELLOW,
    RED,
    MAGENTA,
    WHITE
};

bool backlight = false;
int backlight_color = colors_count - 1;

uint32_t color_default = pixels.Color(70, 70, 70);
uint32_t color_off = pixels.Color(0, 0, 0);

int button_pixels[] = {
    0, 1, 2,
    5, 4, 3
};
int button_colors[] = { 0, 0, 0, 0, 0, 0 };

WS2812FX pixels_effect = WS2812FX(BUTTON_COUNT - 1, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);
bool effect_active = false;
int effect_color = 0;
uint32_t effect_speed = 3000;

int effects_count = 6;
int effects[] = {
    FX_MODE_BREATH,
    FX_MODE_RAINBOW,
    FX_MODE_FIRE_FLICKER,
    FX_MODE_FADE,
    FX_MODE_SCAN,
    FX_MODE_CHASE_COLOR
};
int effect_index = 0;

int boot_anim = 7700;

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

void nextBacklightColor() {
    backlight_color = (backlight_color + 1) % colors_count;
#ifdef DEBUG_LOG
    Debug.print("backlight color "); Debug.print(backlight_color); Debug.print(" 0x"); Debug.println(colors[backlight_color], HEX);
#endif
}

void setupEffects() {
    pixels_effect.init();
    pixels_effect.setColor(colors[effect_color]);
    pixels_effect.setSpeed(effect_speed);
    pixels_effect.setMode(effects[effect_index]);
}

void setEffect(int index) {
    if (effect_index == index) return;

    effect_index = index;
    pixels_effect.setMode(effects[effect_index]);
    pixels_effect.setColor(colors[effect_color]);
    pixels_effect.setSpeed(effect_speed);

    // special defaults for some effects
    if (effects[effect_index] == FX_MODE_FIRE_FLICKER) {
        pixels_effect.setColor(RED);
        pixels_effect.setSpeed(1000);
    } else if (effects[effect_index] == FX_MODE_RAINBOW) {
        pixels_effect.setSpeed(8000);
    } else if (effects[effect_index] == FX_MODE_SCAN && effect_speed > 2000) {
        pixels_effect.setSpeed(2000);
    } else if (effects[effect_index] == FX_MODE_CHASE_COLOR && effect_speed > 2000) {
        pixels_effect.setSpeed(2000);
    }

#ifdef DEBUG_LOG
    Debug.print("effect mode "); Debug.print(effect_index); Debug.print(" "); Debug.print(effects[effect_index]); Debug.print(" "); Debug.println(pixels_effect.getModeName(effects[effect_index]));
#endif
}

void nextEffect() {
    setEffect((effect_index + 1) % effects_count);
}

void startEffect() {
    if (effect_active) return;

    effect_active = true;
    pixels.setBrightness(66);
    pixels_effect.start();
}

void stopEffect() {
    if (!effect_active) return;
    effect_active = false;
    pixels_effect.stop();
    pixels.setBrightness(10);
}

void setEffectColor(int index) {
    if (effect_color == index) return;

    effect_color = index;
    pixels_effect.setColor(colors[effect_color]);
#ifdef DEBUG_LOG
    Debug.print("effect color "); Debug.print(effect_color); Debug.print(" 0x"); Debug.println(colors[effect_color], HEX);
#endif
}

void nextEffectColor() {
    setEffectColor((effect_color + 1) % colors_count);
}

void setEffectSpeed(int speed) {
    if (effect_speed == speed || speed < 1000 || speed > 10000) return;

    effect_speed = speed;
    pixels_effect.setSpeed(effect_speed);
#ifdef DEBUG_LOG
    Debug.print("effect speed "); Debug.println(effect_speed);
#endif
}

void nextEffectSpeed() {
    setEffectSpeed(effect_speed - 1000);
}

void reset() {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons_lit[i] = false;
        button_colors[i] = 0;
    }
    backlight = false;
    backlight_color = colors_count - 1;
    stopEffect();
    effect_index = 0;
    effect_color = 0;
    effect_speed = 3000;
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
void handleLedStatus(uint8_t led_status) {

    int latch = led_status >> 4; // bit 5
    if (latch == 1 && led_latch_handled) {
        return;
    }
    if (latch == 0) {
        led_latch_handled = false;
        return;
    }

    led_latch_handled = true;
    int command = led_status & 7; // bits 1-3
    bool param = ((led_status >> 3 & 1) == 1); // bit 4

#ifdef DEBUG_LOG
    Debug.print("led command received "); Debug.print(command); Debug.print(" param "); Debug.println(param);
#endif

    if (effect_active) {
        if (command == 1) { // 0bX001 next effect
            nextEffect();
        } else if (command == 2) { // 0bX010
            nextEffectColor();
        } else if (command == 4) { // 0bX100
            nextEffectSpeed();
        }

    } else if (command == 0) { // 0bX000 backlight on/next/off
        if (param && !backlight) { // turn on
            backlight = true;
        } else if (param) { // already on, next color
            nextBacklightColor();

        } else if (backlight) { // not off, turn off
            backlight = false;
        } else { // already off, reset color
            backlight_color = colors_count - 1;
        }

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

    } else if (command == 7 && !param) {
        reset();
    }
}

void sendButtonKey(int index){
#if defined(DEBUG_LOG) && !defined(DEBUG_SERIAL)
    Debug.print("key "); Debug.println(button_keys[index]);
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
        if (effect_active) nextEffect();
        else startEffect();
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

    if (backlight && buttons[3].isPressed() && buttons[2].wasReleased()) {
        nextBacklightColor();
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
        setPixelColor(i, DIM_75(colors[backlight_color]));

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
        Debug.print("rotary "); Debug.print(rotary_pos_new);
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


RingBuff_t Serial_Buffer;
uint8_t serial_data[7] = { 0 };
uint8_t serial_buffer_bytes = 0;

bool onOffToggle(uint8_t data, bool current) {
    if (data == 0) {
        return false;
    }
    if (data == 1) {
        return true;
    }
    if (data == 2) {
        return !current;
    }
    return current;
}

int getIndex(uint8_t data, uint8_t current, uint8_t count) {
    if (data > 0 && data <= count) {
        return data - 1;
    }
    return current;
}

void bufferSerial() {
    if (!Serial.available())
        return;

    if (serial_buffer_bytes > 0 ) {
        while (Serial.available() && serial_buffer_bytes > 0) {
            serial_buffer_bytes--;
            RingBuffer_Insert(&Serial_Buffer, Serial.read());
        }
    }
    else if (Serial.read() == 0xCC) {
        serial_buffer_bytes = 7;
    }
}

void handleSerial() {
    if (RingBuffer_GetCount(&Serial_Buffer) < 7)
        return;

    for (uint8_t i=0; i<7; i++) {
        serial_data[i] = RingBuffer_Remove(&Serial_Buffer);
    }

    if (serial_data[0] == 0xB0) {
        backlight = onOffToggle(serial_data[1], backlight);
        backlight_color = getIndex(serial_data[2], backlight_color, colors_count);
    }

    else if (serial_data[0] == 0xBF && serial_data[1] > 0 && serial_data[1] <= BUTTON_COUNT) {
        int8_t index = serial_data[1] - 1;
        if (index >= 0 && index < BUTTON_COUNT) {
            buttons_lit[index] = onOffToggle(serial_data[2], buttons_lit[index]);
            button_colors[index] = getIndex(serial_data[3], button_colors[index], colors_count);
        }
    }

    else if (serial_data[0] == 0xF0) {
        bool target = onOffToggle(serial_data[1], effect_active);
        setEffect(getIndex(serial_data[2], effect_index, effects_count));
        setEffectColor(getIndex(serial_data[3], effect_color, colors_count));
        if (serial_data[4] > 0) {
            setEffectSpeed(serial_data[4]*1000);
        }

        if (target && !effect_active) {
            startEffect();
        }else if (!target && effect_active) {
            stopEffect();
        }
    }

    else if (serial_data[0] == 0xDD) {
        handleLedStatus(serial_data[1]);
    }

    else if (serial_data[0] == 0x99) {
        reset();
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
    delay(500);
    if (!Keyboard.isConnected()) {
        pixels_effect.setColor(RED);
    }

	RingBuffer_InitBuffer(&Serial_Buffer);
}

void loop() {
    // handleLedStatus();
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

    bufferSerial();
    handleSerial();

    delay(5);
}

