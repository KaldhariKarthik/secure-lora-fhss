#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2
#define SYNC_FREQ 433.05
#define NUM_FREQS 20
#define ANALOG_PIN 34

float frequencies[NUM_FREQS];
uint8_t xorKey[32];

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Transmitter V2");

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(SYNC_FREQ * 1E6)) {
    Serial.println("LoRa init failed!");
    while (true);
  }

  generateFrequencies();
  generateXORKey();
}

void generateFrequencies() {
  float base = 433.10;
  for (int i = 0; i < NUM_FREQS; i++) {
    frequencies[i] = base + (i * 0.02);  // 10 kHz spacing
  }
  shuffleFrequencies();
}

void shuffleFrequencies() {
  for (int i = 0; i < NUM_FREQS; i++) {
    int j = analogRead(ANALOG_PIN) % NUM_FREQS;
    float temp = frequencies[i];
    frequencies[i] = frequencies[j];
    frequencies[j] = temp;
    delay(5);
  }
}

void generateXORKey() {
  for (int i = 0; i < 32; i++) {
    xorKey[i] = analogRead(ANALOG_PIN) & 0xFF;
    delay(5);
  }
}

String encodeKey() {
  String out = "";
  for (int i = 0; i < 32; i++) {
    out += String(xorKey[i], HEX);
    if (i < 31) out += ",";
  }
  return out;
}

String xorEncrypt(String msg) {
  String encrypted = "";
  for (int i = 0; i < msg.length(); i++) {
    char c = msg[i] ^ xorKey[i % 32];
    encrypted += c;
  }
  return encrypted;
}

void sendSyncPacket() {
  String sync = "SYNC|" + String(NUM_FREQS) + "|";
  for (int i = 0; i < NUM_FREQS; i++) {
    sync += String(frequencies[i], 2);
    if (i < NUM_FREQS - 1) sync += ",";
  }
  sync += "|" + encodeKey();

  LoRa.setFrequency(SYNC_FREQ * 1E6);
  LoRa.beginPacket();
  LoRa.print(sync);
  LoRa.endPacket();
  Serial.println("SYNC sent.");
}

bool waitForAck(int timeout = 2000) {
  long start = millis();
  while (millis() - start < timeout) {
    int size = LoRa.parsePacket();
    if (size) {
      String ack = "";
      while (LoRa.available()) ack += (char)LoRa.read();
      if (ack == "ACK") {
        Serial.println("ACK received.");
        return true;
      }
    }
  }
  return false;
}

void sendMessage(String msg) {
  sendSyncPacket();
  if (!waitForAck()) {
    Serial.println("No ACK. Aborting.");
    return;
  }

  String encrypted = xorEncrypt(msg);
  int chunkSize = encrypted.length() / NUM_FREQS;
  if (chunkSize == 0) chunkSize = 1;

  int index = 0;
  for (int i = 0; i < NUM_FREQS && index < encrypted.length(); i++) {
    String chunk = encrypted.substring(index, index + chunkSize);
    index += chunkSize;

    LoRa.setFrequency(frequencies[i] * 1E6);
    LoRa.beginPacket();
    LoRa.print(chunk);
    LoRa.endPacket();
    Serial.print("Sent chunk on ");
    Serial.print(frequencies[i]);
    Serial.print(": ");
    Serial.println(chunk);

    waitForAck();  // Optional per-hop ACK
    delay(200);
  }
}

void loop() {
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    sendMessage(msg);
  }
}
