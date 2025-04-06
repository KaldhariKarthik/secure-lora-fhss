#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2

#define SYNC_FREQ 433.05  // Fixed sync frequency
float frequencies[10];    // Store frequencies from SYNC message
int numFrequencies = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("LoRa Receiver");
    LoRa.setPins(SS, RST, DIO0);
    
    if (!LoRa.begin(SYNC_FREQ * 1E6)) {
        Serial.println("LoRa initialization failed. Check connections.");
        while (true);
    }

    Serial.println("LoRa initialized. Waiting for SYNC...");
}

bool receiveSyncPacket() {
    LoRa.setFrequency(SYNC_FREQ * 1E6);  // Ensure we are tuned to SYNC
    delay(500);  // Allow LoRa module to stabilize

    long startTime = millis();
    while (millis() - startTime < 3000) {  // Wait 3 seconds for SYNC
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            String syncMessage = "";
            while (LoRa.available()) {
                syncMessage += (char)LoRa.read();
            }

            if (syncMessage.startsWith("SYNC|")) {
                Serial.print("SYNC received: ");
                Serial.println(syncMessage);

                // Parse SYNC Message
                int firstBar = syncMessage.indexOf('|');
                int secondBar = syncMessage.indexOf('|', firstBar + 1);
                int thirdBar = syncMessage.indexOf('|', secondBar + 1);
                
                numFrequencies = syncMessage.substring(firstBar + 1, secondBar).toInt();
                String freqString = syncMessage.substring(thirdBar + 1);
                
                int idx = 0;
                while (freqString.length() > 0 && idx < numFrequencies) {
                    int comma = freqString.indexOf(',');
                    if (comma == -1) comma = freqString.length();
                    frequencies[idx++] = freqString.substring(0, comma).toFloat();
                    freqString = freqString.substring(comma + 1);
                }

                // Send ACK
                delay(100);  // Small delay before sending ACK
                LoRa.beginPacket();
                LoRa.print("ACK");
                LoRa.endPacket();

                Serial.println("ACK sent. Ready for data transmission...");
                return true;
            }
        }
    }

    Serial.println("SYNC not received. Returning to SYNC scan...");
    return false;
}

void receiveMessage() {
    String fullMessage = "";

    for (int i = 0; i < numFrequencies; i++) {
        LoRa.setFrequency(frequencies[i] * 1E6);
        Serial.print("Listening on: ");
        Serial.print(frequencies[i]);
        Serial.println(" MHz");

        long startTime = millis();
        while (millis() - startTime < 2000) {  // Wait for data on this frequency
            int packetSize = LoRa.parsePacket();
            if (packetSize) {
                String chunk = "";
                while (LoRa.available()) {
                    chunk += (char)LoRa.read();
                }

                fullMessage += chunk;
                Serial.print("Received on ");
                Serial.print(frequencies[i]);
                Serial.print(" MHz: ");
                Serial.println(chunk);

                // Send ACK
                delay(100);  // Small delay before sending ACK
                LoRa.beginPacket();
                LoRa.print("ACK");
                LoRa.endPacket();

                break;  // Move to next frequency
            }
        }
    }

    Serial.print("Full message reconstructed: ");
    Serial.println(fullMessage);
}

void loop() {
    while (true) {
        if (receiveSyncPacket()) {
            receiveMessage();
            Serial.println("Message received. Waiting for next SYNC...");
        }
    }
}
