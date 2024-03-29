#define PIN_ESP_TX  14
#define PIN_ESP_RX  15

// #define SERIALDBG

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

#ifdef SERIALDBG
#include <SoftwareSerial.h>
SoftwareSerial Debug2 = SoftwareSerial(12, 13);
#endif

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
#ifdef SERIALDBG
            Debug2.println();
            Debug2.println("Communication with WiFi module failed!");
#endif
            while(true);
        }
#ifdef SERIALDBG
        Debug2.println("connecting wifi");
#endif

        // WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
#ifdef SERIALDBG
            Debug2.print(".");
#endif
        }

#ifdef SERIALDBG
        Debug2.println(" connected");
        Debug2.print("IP address: ");
        Debug2.println(WiFi.localIP());
#endif
        delay(1500);
    }

    public:
        MqttWrapper(const char* ssid, const char* password, const char* broker, uint16_t port, String base_topic) : ssid(ssid), password(password), base_topic(base_topic) {
#ifdef SERIALDBG
        Debug2.println("MqttWrapper constructed");
#endif
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
                    Debug2.print("mqtt connection failed: ");
                    Debug2.println(client.state());
                #endif

                delay(50);
                wifiConnect();
            }
        }

        int publish(String topic, String payload, bool retain) {
            #ifdef SERIALDBG
                Debug2.println("publish " + base_topic + "/" + topic + " " + payload);
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

