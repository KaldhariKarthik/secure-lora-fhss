#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2

#define SYNC_FREQ 433.05  // Fixed SYNC Frequency

float frequencies[] = {433.10, 433.30, 433.50, 433.70};
const int numFrequencies = sizeof(frequencies) / sizeof(frequencies[0]);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("LoRa Transmitter");
    LoRa.setPins(SS, RST, DIO0);
    
    if (!LoRa.begin(SYNC_FREQ * 1E6)) {
        Serial.println("LoRa initialization failed. Check connections.");
        while (true);
    }
    
    Serial.println("LoRa initialized. Ready for input...");
}

void sendSyncPacket() {
    String syncMessage = "SYNC|" + String(numFrequencies) + "|" + String(millis()) + "|";
    for (int i = 0; i < numFrequencies; i++) {
        syncMessage += String(frequencies[i]);
        if (i < numFrequencies - 1) syncMessage += ",";
    }

    LoRa.setFrequency(SYNC_FREQ * 1E6);
    LoRa.beginPacket();
    LoRa.print(syncMessage);
    LoRa.endPacket();
    
    Serial.print("Sent SYNC: ");
    Serial.println(syncMessage);
}

bool waitForAck() {
    long startTime = millis();
    while (millis() - startTime < 2000) {  // Wait 2 seconds for ACK
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            String ack = "";
            while (LoRa.available()) {
                ack += (char)LoRa.read();
            }
            if (ack == "ACK") {
                Serial.println("ACK received! Starting transmission...");
                return true;
            }
        }
    }
    Serial.println("ACK not received. Retrying SYNC...");
    return false;
}

void sendMessage(const String& message) {
    while (true) {  // Keep retrying SYNC until ACK is received
        sendSyncPacket();
        if (waitForAck()) break;
        delay(1000);  // Prevent flooding
    }

    Serial.println("Starting Frequency Hopping Transmission...");

    int msgIndex = 0;
    int msgLength = message.length();
    int chunkSize = msgLength / numFrequencies;
    if (chunkSize == 0) chunkSize = 1;

    for (int i = 0; i < numFrequencies && msgIndex < msgLength; i++) {
        String chunk = message.substring(msgIndex, min(msgIndex + chunkSize, msgLength));
        msgIndex += chunkSize;

        LoRa.setFrequency(frequencies[i] * 1E6);
        Serial.print("Hopping to: ");
        Serial.print(frequencies[i]);
        Serial.println(" MHz");

        LoRa.beginPacket();
        LoRa.print(chunk);
        LoRa.endPacket();

        Serial.print("Sent: ");
        Serial.println(chunk);

        if (!waitForAck()) {
            i--;  // Resend if ACK not received
        }
    }

    Serial.println("Transmission Complete.");
}

void loop() {
    if (Serial.available()) {
        String message = Serial.readString();
        sendMessage(message);
    }
}
