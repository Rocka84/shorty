#ifndef __USBKeyboard_h__
#define __USBKeyboard_h__

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "hid_keys.h"

#define LED_NUMLOCK    (1 << 0)
#define LED_CAPSLOCK   (1 << 1)
#define LED_SCROLLLOCK (1 << 2)
#define LED_COMPOSE    (1 << 3)

class USBKeyboard {
    private:
        bool connected = false;

        void connectFirmware() {
            connected = false;

            uint8_t handshake[8] = { 0xE0, 1, 0xE0, 0, 0, 0, 0, 0 };
            Serial.write(handshake, 8);

            int wait = 500;
            while (Serial.available() < 3 && wait > 0) {
                delay(10);
                wait -= 10;
            }
            if (
                    Serial.read() == 0xEF
                    && Serial.read() == 0x01
                    && Serial.read() == 0xEF
               ) {
               connected = true;
            }
        }

    public:
        void init () {
            // We will talk to atmega8u2 using 9600 bps
            Serial.begin(9600);
            connectFirmware();
            _releaseKeys();
        }

        bool isConnected() {
            return connected;
        }

        void sendKeyStroke(byte keyStroke) {
            sendKeyStroke(keyStroke, 0);
        }

        void _releaseKeys() {
            if (!connected) return;
            uint8_t keyNone[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
            Serial.write(keyNone, 8);    // Release Key
        }

        void sendKeyStroke(byte keyStroke, byte modifiers) {
            if (!connected) return;
            Serial.write(modifiers);  // Modifier Keys
            Serial.write(0);          // Reserved
            Serial.write(keyStroke);  // Keycode 1
            Serial.write(0);          // Keycode 2
            Serial.write(0);          // Keycode 3
            Serial.write(0);          // Keycode 4
            Serial.write(0);          // Keycode 5
            Serial.write(0);          // Keycode 6

            _releaseKeys();
        }

        uint8_t readLedStatus() {
            if (!connected) return 0;
            uint8_t ledStatus;
            _releaseKeys();
            while (Serial.available() > 0) {
                ledStatus = Serial.read();
            }

            return ledStatus;
        }

        void print(char *chp)
        {
            if (!connected) return;
            uint8_t buf[8] = { 0 };	/* Keyboard report buffer */

            while (*chp) {
                if ((*chp >= 'a') && (*chp <= 'z')) {
                    buf[2] = *chp - 'a' + 4;
                } else if ((*chp >= 'A') && (*chp <= 'Z')) {
                    buf[0] = MOD_SHIFT_LEFT;	/* Caps */
                    buf[2] = *chp - 'A' + 4;
                } else if ((*chp >= '1') && (*chp <= '9')) {
                    buf[2] = *chp - '1' + 30;
                } else if (*chp == '0') {
                    buf[2] = KEY_0;
                } else {
                    switch (*chp) {
                        case ' ':
                            buf[2] = KEY_SPACE;	// Space
                            break;
                        case '-':
                            buf[2] = KEY_MINUS;
                            break;
                        case '=':
                            buf[2] = KEY_EQUALS;
                            break;
                        case '[':
                            buf[2] = KEY_LBRACKET;
                            break;
                        case ']':
                            buf[2] = KEY_RBRACKET;
                            break;
                        case '\\':
                            buf[2] = KEY_BACKSLASH;
                            break;
                        case ';':
                            buf[2] = KEY_SEMICOLON;
                            break;
                        case ':':
                            buf[0] = MOD_SHIFT_LEFT;	/* Caps */
                            buf[2] = KEY_SEMICOLON;
                            break;
                        case '"':
                            buf[2] = KEY_QUOTE;
                            break;
                        case '~':
                            buf[2] = KEY_TILDE;
                            break;
                        case ',':
                            buf[2] = KEY_COMMA;
                            break;
                        case '/':
                            buf[2] = KEY_SLASH;
                            break;
                        case '@':
                            buf[0] = MOD_SHIFT_LEFT;	/* Caps */
                            buf[2] = KEY_2;
                            break;
                        default:
                            /* Character not handled. To do: add rest of chars from HUT1_11.pdf */
                            buf[2] = KEY_PERIOD;// Period
                            break;
                    }
                }

                Serial.write(buf, 8);	// Send keystroke
                buf[0] = 0;
                buf[2] = 0;
                Serial.write(buf, 8);	// Release key
                chp++;
            }
        }
};

USBKeyboard Keyboard;

#endif // __USBKeyboard_h__
