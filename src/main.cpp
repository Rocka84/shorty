#include <Arduino.h>
#include <USBKeyboard.h>
#include <JC_Button.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// #define SERIALDBG
// #define WIFI_ESP

#ifdef WIFI_ESP
#include <MqttWrapper.h>
#include <secrets.h>

#define PIN_ESP_TX  14
#define PIN_ESP_RX  15
#endif

#define BUTTON_COUNT 6
#define PIN_BTN_A   2
#define PIN_BTN_B   3
#define PIN_BTN_C   4
#define PIN_BTN_D   5
#define PIN_BTN_E   6
#define PIN_BTN_F   7

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

Adafruit_NeoPixel pixels(BUTTON_COUNT, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

bool backlight = false;
uint32_t color_default = pixels.Color(3, 3, 3);
uint32_t color_off = pixels.Color(0, 0, 0);

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

#ifdef WIFI_ESP
MqttWrapper mqtt(WIFI_SSID, WIFI_PASSWORD, MQTT_HOST, 1883, "shorty");
#endif

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
        pixels.setPixelColor(i, pixels.Color(0, 20, 20));
        buttons_suppressed[i] = true;

#ifdef SERIALDBG
        Debug.print("long-press ");
        Debug.println(i);
#endif

    } else if (buttons[i].wasReleased() && !buttons_suppressed[i]) {
        sendButtonKey(i);
        pixels.setPixelColor(i, pixels.Color(20, 0, 20));

#ifdef SERIALDBG
        Debug.print("press ");
        Debug.println(i);
#endif

    } else if (buttons[i].isPressed()) {
        pixels.setPixelColor(i, pixels.Color(20, 0, 0));

    } else if (buttons_lit[i]) {
        pixels.setPixelColor(i, colors[button_colors[i]]);

    } else if(backlight) {
        pixels.setPixelColor(i, color_default);

    } else {
        pixels.setPixelColor(i, color_off);
    }

    if (buttons[i].wasReleased()) {
        buttons_suppressed[i] = false;
    }
}

#ifdef WIFI_ESP
bool payloadToBool(String payload_str, bool current) {
    if (payload_str == "off" || payload_str == "OFF" || payload_str == "0") {
        return false;
    } else if (payload_str == "on" || payload_str == "ON" || payload_str == "1") {
        return true;
    } else if (payload_str == "toggle" || payload_str == "TOGGLE" || payload_str == "2") {
        return !current;
    }

    return current;
}

int hexToInt(String hex){
    char c[hex.length() + 1];
    hex.toCharArray(c, hex.length() + 1);
    return (int) strtol(c, NULL, 16);
}

uint32_t payloadToColor(String payload_str, uint32_t current) {
    payload_str.trim();
    if (payload_str.length() == 1) {
        int index = payload_str.toInt() % 7;
        return colors[index];

    } else if (payload_str.startsWith("#")) {
        // hex color
        return pixels.Color(
                hexToInt(payload_str.substring(1, 2)),
                hexToInt(payload_str.substring(3, 2)),
                hexToInt(payload_str.substring(5, 2)));

    } else if (payload_str.startsWith("(")) {
        // int color
        return pixels.Color(
                payload_str.substring(1, 3).toInt(),
                payload_str.substring(5, 3).toInt(),
                payload_str.substring(9, 3).toInt());
    }

    return current;
}

String payloadToString(byte* payload, unsigned int length) {
    char payload_chars[length];
    for (unsigned int i = 0; i < length; i++) {
        payload_chars[i] = (char) payload[i];
    }
    return String(payload_chars);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
#ifdef SERIALDBG
    Debug.print("Message arrived [");
    Debug.print(topic);
    Debug.print("] ");
#endif

    String topic_str = String(topic);
    String payload_str = payloadToString(payload, length);

#ifdef SERIALDBG
    Debug.println(payload_str);
#endif

    if (topic_str == "shorty/command/backlight/power") {
        backlight = payloadToBool(payload_str, backlight);

    } else if (topic_str.startsWith("shorty/command/led")) {
        int index = topic_str.substring(19, 1).toInt();

        if (topic_str.endsWith("/power")) {
            buttons_lit[index] = payloadToBool(payload_str, buttons_lit[index]);
        } else if (topic_str.endsWith("/color")) {
            button_colors[index] = payloadToColor(payload_str, button_colors[index]);
        }
    }
}
#endif // WIFI_ESP


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

#ifdef WIFI_ESP
    mqtt.connect();
    mqtt.setCallback(mqttCallback);
    mqtt.subscribe("shorty/#");
#endif
}

void loop() {
    handleLedStatus();
#ifdef WIFI_ESP
    mqtt.loop();
#endif

    for (int i = 0; i < BUTTON_COUNT; i++) {
        handleButton(i);
    }

    pixels.show();

    delay(50);
}

