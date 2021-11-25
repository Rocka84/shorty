#define PIN_ESP_TX  14
#define PIN_ESP_RX  15

#define SERIALDBG

#include <PubSubClient.h>
#include <WiFiEspAT.h>
// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial Serial1(PIN_ESP_RX, PIN_ESP_TX);
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

WiFiClient espClient;

class MqttWrapper {
    PubSubClient client;
    const char* ssid;
    const char* password;
    String base_topic = "";

    void wifiConnect() {
        if (WiFi.status() == WL_CONNECTED) {
            return;
        }
        delay(10);

        Serial1.begin(AT_BAUD_RATE);
        WiFi.init(Serial1);

        if (WiFi.status() == WL_NO_MODULE) {
            Serial.println();
            Serial.println("Communication with WiFi module failed!");
            return;
        }
        #ifdef SERIALDBG
            Serial.println("connecting wifi");
        #endif

        // WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            #ifdef SERIALDBG
                Serial.print(".");
            #endif
        }

        #ifdef SERIALDBG
            Serial.println(" connected");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        #endif
        delay(1500);
    }

    public:
        MqttWrapper(const char* ssid, const char* password, const char* broker, uint16_t port, String base_topic) : ssid(ssid), password(password), base_topic(base_topic) {
            client = PubSubClient(espClient);
            client.setServer(broker, port);
        }

        void connect() {
            wifiConnect();
            while (!client.connected()) {
                if (client.connect("ESP8266Client")) {
                    return;
                }
                #ifdef SERIALDBG
                    Serial.print("mqtt connection failed: ");
                    Serial.println(client.state());
                #endif

                delay(50);
                wifiConnect();
            }
        }

        int publish(String topic, String payload, bool retain) {
            #ifdef SERIALDBG
                Serial.println("publish " + base_topic + "/" + topic + " " + payload);
            #endif
            connect();

            char topic_c[50];
            char payload_c[50];
            (base_topic + "/" + topic).toCharArray(topic_c, 50);
            payload.toCharArray(payload_c, 50);
            return client.publish(topic_c, payload_c, retain);
        }

        int publish(String topic, String payload) {
            return publish(topic, payload, false);
        }

        bool connected() {
            return client.connected();
        }

        void ensureConnection() {
            if (client.connected()) return;
            connect();
        }

        void loop(bool ensure_connection) {
            if (ensure_connection) connect();
            client.loop();
        }

        void loop() {
            loop(false);
        }

        void setCallback(MQTT_CALLBACK_SIGNATURE) {
            client.setCallback(callback);
        }

        bool subscribe(const char* topic) {
            return client.subscribe(topic);
        }
};

