#include <SPI.h>
#include <Ethernet.h>
#define pin_dout  8
#define pin_slk   9
#define OUT_VOL   0.001f
#define LOAD      2000.0f

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //イーサネットシールドに記載がある場合は書き換え
char server[] = "aaaaa.aaa.com"; //初期ドメインを記入
EthernetClient client;
float offset;
float coinWeights[] = {1.0, 3.75, 4.5, 4.0, 4.8, 7.0};
int coinValues[] = {1, 5, 10, 50, 100, 500};

String urlEncode(String str) {
    String encoded = "";
    char hex[4];
    for (unsigned int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            sprintf(hex, "%%%02X", (unsigned char)c);
            encoded += hex;
        }
    }
    return encoded;
}

void sendDataToServer(String json) {
    Serial.println("サーバーに接続中...");
    
    if (client.connect(server, 80)) {
        Serial.println("サーバーに接続成功");

        String encodedJson = urlEncode(json);

        client.print("GET /CoinCounter_server/data_receiver.php?data=");
        client.print(encodedJson);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(server);
        client.println("Connection: close");
        client.println();
        delay(500);
        Serial.println("サーバーのレスポンス:");
        while (client.available()) {
            Serial.print((char)client.read());
        }
        Serial.println();
    } else {
        Serial.println("接続失敗！");
    }
    client.stop();
    Serial.println("接続終了");
}

void HX711_Init() {
    pinMode(pin_slk, OUTPUT);
    pinMode(pin_dout, INPUT);
}

void HX711_Reset() {
    digitalWrite(pin_slk, 1);
    delayMicroseconds(100);
    digitalWrite(pin_slk, 0);
    delayMicroseconds(100);
}

long HX711_Read() {
    long data = 0;
    while (digitalRead(pin_dout) != 0);
    for (int i = 0; i < 24; i++) {
        digitalWrite(pin_slk, 1);
        delayMicroseconds(5);
        digitalWrite(pin_slk, 0);
        delayMicroseconds(5);
        data = (data << 1) | digitalRead(pin_dout);
    }
    digitalWrite(pin_slk, 1);
    delayMicroseconds(10);
    digitalWrite(pin_slk, 0);
    return data ^ 0x800000;
}

long HX711_Averaging(char num) {
    long sum = 0;
    for (int i = 0; i < num; i++) sum += HX711_Read();
    return sum / num;
}

float HX711_getGram(char num) {
    #define HX711_ADC1bit   (4.2987 / 16777216)  
    #define HX711_SCALE     (OUT_VOL * 4.2987 / LOAD * 128)
    return (HX711_Averaging(num) * HX711_ADC1bit) / HX711_SCALE;
}

String createJson(int coinCount[]) {
    String json = "{";
    for (int i = 0; i < 6; i++) {
        json += "\"" + String(coinValues[i]) + "\": " + String(coinCount[i]);
        if (i < 5) json += ", ";
    }
    json += "}";
    return json;
}

void setup() {
    Serial.begin(9600);
    if (Ethernet.begin(mac) == 0) {
        Serial.println("Ethernet接続に失敗");
        while (true);
    }
    Serial.print("IPアドレス: ");
    Serial.println(Ethernet.localIP());
    HX711_Init();
    HX711_Reset();
    offset = HX711_getGram(30);
    Serial.println("準備完了");
}

void loop() { 
    float data = HX711_getGram(5) - offset;
    Serial.print("測定重量: ");
    Serial.print(data);
    Serial.println(" g");
    int coinCount[6] = {0};
    for (int i = 5; i >= 0; i--) {
        if (data >= coinWeights[i]) {
            coinCount[i] = int(data / coinWeights[i]);
            data -= coinCount[i] * coinWeights[i];
        }
    }
    String json = createJson(coinCount);
    Serial.println("送信データ: " + json);
    sendDataToServer(json);
    delay(5000);
}
