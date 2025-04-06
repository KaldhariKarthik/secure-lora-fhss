#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2
#define SYNC_FREQ 433.05

float frequencies[20];
uint8_t xorKey[32];
int numFreqs = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver V2");

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(SYNC_FREQ * 1E6)) {
    Serial.println("LoRa init failed!");
    while (true);
  }
}

bool parseSync(String packet) {
  if (!packet.startsWith("SYNC|")) return false;
  int first = packet.indexOf('|');
  int second = packet.indexOf('|', first + 1);
  int third = packet.indexOf('|', second + 1);

  numFreqs = packet.substring(first + 1, second).toInt();
  String freqStr = packet.substring(second + 1, third);
  String keyStr = packet.substring(third + 1);

  int idx = 0;
  while (freqStr.length() > 0 && idx < numFreqs) {
    int comma = freqStr.indexOf(',');
    if (comma == -1) comma = freqStr.length();
    frequencies[idx++] = freqStr.substring(0, comma).toFloat();
    freqStr = freqStr.substring(comma + 1);
  }

  int k = 0;
  while (keyStr.length() > 0 && k < 32) {
    int comma = keyStr.indexOf(',');
    if (comma == -1) comma = keyStr.length();
    xorKey[k++] = strtol(keyStr.substring(0, comma).c_str(), NULL, 16);
    keyStr = keyStr.substring(comma + 1);
  }

  return true;
}

String xorDecrypt(String enc) {
  String out = "";
  for (int i = 0; i < enc.length(); i++) {
    out += (char)(enc[i] ^ xorKey[i % 32]);
  }
  return out;
}

bool receiveSync() {
  LoRa.setFrequency(SYNC_FREQ * 1E6);
  long start = millis();
  while (millis() - start < 3000) {
    int size = LoRa.parsePacket();
    if (size) {
      String packet = "";
      while (LoRa.available()) packet += (char)LoRa.read();

      if (parseSync(packet)) {
        Serial.println("SYNC Received.");
        LoRa.beginPacket();
        LoRa.print("ACK");
        LoRa.endPacket();
        return true;
      }
    }
  }
  return false;
}

void receiveMessage() {
  String combined = "";
  for (int i = 0; i < numFreqs; i++) {
    LoRa.setFrequency(frequencies[i] * 1E6);
    long start = millis();
    while (millis() - start < 2000) {
      int size = LoRa.parsePacket();
      if (size) {
        String chunk = "";
        while (LoRa.available()) chunk += (char)LoRa.read();
        combined += chunk;
        Serial.print("Received on ");
        Serial.print(frequencies[i]);
        Serial.print(": ");
        Serial.println(chunk);

        LoRa.beginPacket();
        LoRa.print("ACK");
        LoRa.endPacket();
        break;
      }
    }
  }
  Serial.print("Decrypted Message: ");
  Serial.println(xorDecrypt(combined));
}

void loop() {
  if (receiveSync()) {
    receiveMessage();
  }
}
