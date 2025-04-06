#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2

#define SYNC_FREQ 433.05  // Fixed sync frequency
#define CHANNEL_WIDTH 0.01  // 10 kHz = 0.01 MHz channel width

float frequencies[50];    // Will hold the hopping frequencies
int numFrequencies = 0;
int frequenciesToUse = 0;

// Encryption key (will be received in SYNC packet)
uint8_t encryptionKey[16];

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("LoRa Receiver with Multiple 10kHz Channels and Encryption");
    LoRa.setPins(SS, RST, DIO0);
    
    if (!LoRa.begin(SYNC_FREQ * 1E6)) {
        Serial.println("LoRa initialization failed. Check connections.");
        while (true);
    }

    Serial.println("LoRa initialized. Waiting for SYNC...");
}

// Convert a hex string to a byte array
bool hexStringToBytes(const String& hexString, uint8_t* byteArray, int length) {
    if (hexString.length() != length * 2) {
        return false;
    }
    
    for (int i = 0; i < length; i++) {
        String byteString = hexString.substring(i * 2, i * 2 + 2);
        byteArray[i] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
    }
    
    return true;
}

// Simple XOR decryption function (same as encryption)
String decryptMessage(const String& encryptedMessage) {
    String decrypted = "";
    for (unsigned int i = 0; i < encryptedMessage.length(); i++) {
        // XOR each character with corresponding key byte (with wrap-around)
        decrypted += (char)(encryptedMessage[i] ^ encryptionKey[i % 16]);
    }
    return decrypted;
}

bool receiveSyncPacket() {
    LoRa.setFrequency(SYNC_FREQ * 1E6);  // Ensure we are tuned to SYNC
    LoRa.setSignalBandwidth(125E3);      // Use wider bandwidth for sync
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
                int fourthBar = syncMessage.indexOf('|', thirdBar + 1);
                
                numFrequencies = syncMessage.substring(firstBar + 1, secondBar).toInt();
                frequenciesToUse = syncMessage.substring(secondBar + 1, thirdBar).toInt();
                String freqString = syncMessage.substring(thirdBar + 1, fourthBar);
                String keyString = syncMessage.substring(fourthBar + 1);
                
                Serial.println("Frequency hopping sequence (first 5 shown):");
                
                int idx = 0;
                while (freqString.length() > 0 && idx < frequenciesToUse) {
                    int comma = freqString.indexOf(',');
                    if (comma == -1) comma = freqString.length();
                    
                    frequencies[idx] = freqString.substring(0, comma).toFloat();
                    if (idx < 5) {
                        Serial.print(frequencies[idx], 3);
                        Serial.print(" MHz");
                        if (idx < 4 && idx < frequenciesToUse - 1) Serial.print(", ");
                        if (idx == 4 && frequenciesToUse > 5) Serial.print("...");
                    }
                    
                    idx++;
                    freqString = freqString.substring(comma + 1);
                }
                Serial.println();
                Serial.print("Total channels: ");
                Serial.println(frequenciesToUse);
                
                // Extract encryption key
                if (hexStringToBytes(keyString, encryptionKey, 16)) {
                    Serial.print("Received encryption key: ");
                    for (int i = 0; i < 16; i++) {
                        if (encryptionKey[i] < 16) Serial.print("0");
                        Serial.print(encryptionKey[i], HEX);
                        if (i < 15) Serial.print(" ");
                    }
                    Serial.println();
                } else {
                    Serial.println("Error: Invalid encryption key format");
                    return false;
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

// In the receiveMessage function, make these changes:

void receiveMessage() {
    String encryptedMessage = "";
    bool messageComplete = false;
    int channelsChecked = 0;
    
    Serial.println("Starting to receive data across channels...");

    for (int i = 0; i < frequenciesToUse && !messageComplete; i++) {
        LoRa.setFrequency(frequencies[i] * 1E6);
        // Set bandwidth to 10kHz for narrow channel operation
        LoRa.setSignalBandwidth(10E3);
        
        Serial.print("Listening on: ");
        Serial.print(frequencies[i], 3);
        Serial.print(" MHz (Channel ");
        Serial.print(i+1);
        Serial.print(" of ");
        Serial.print(frequenciesToUse);
        Serial.println(")");

        bool chunkReceived = false;
        int retries = 0;
        const int maxRetries = 3;
        
        while (!chunkReceived && retries < maxRetries) {
            long startTime = millis();
            while (millis() - startTime < 2000 && !chunkReceived) {  // Wait for data on this frequency
                int packetSize = LoRa.parsePacket();
                if (packetSize) {
                    String chunk = "";
                    while (LoRa.available()) {
                        chunk += (char)LoRa.read();
                    }
                    chunkReceived = true;
                    channelsChecked++;

                    // Check for end symbol
                    int endPos = chunk.indexOf("<END>");
                    if (endPos != -1) {
                        // Remove the end symbol and mark message as complete
                        chunk = chunk.substring(0, endPos);
                        messageComplete = true;
                        Serial.println("END marker found");
                    }

                    encryptedMessage += chunk;
                    
                    Serial.print("Received encrypted chunk ");
                    Serial.print(i+1);
                    Serial.print(" on ");
                    Serial.print(frequencies[i], 3);
                    Serial.print(" MHz (");
                    Serial.print(chunk.length());
                    Serial.print(" bytes, hex): ");
                    for (unsigned int j = 0; j < chunk.length(); j++) {
                        if ((uint8_t)chunk[j] < 16) Serial.print("0");
                        Serial.print((uint8_t)chunk[j], HEX);
                        Serial.print(" ");
                    }
                    Serial.println();

                    // Send ACK
                    delay(100);  // Small delay before sending ACK
                    LoRa.beginPacket();
                    LoRa.print("ACK");
                    LoRa.endPacket();
                    Serial.println("ACK sent");
                }
            }
            
            if (!chunkReceived) {
                retries++;
                Serial.print("No data received, retry ");
                Serial.print(retries);
                Serial.print("/");
                Serial.println(maxRetries);
            }
        }
        
        if (!chunkReceived) {
            Serial.print("Failed to receive data on channel ");
            Serial.println(i+1);
        }
        
        // Reset to default bandwidth after reception
        LoRa.setSignalBandwidth(125E3);
    }

    Serial.print("Total channels checked: ");
    Serial.print(channelsChecked);
    Serial.print("/");
    Serial.println(frequenciesToUse);

    // Decrypt the message
    String decryptedMessage = decryptMessage(encryptedMessage);
    
    Serial.print("Encrypted message (hex): ");
    for (unsigned int i = 0; i < encryptedMessage.length(); i++) {
        if ((uint8_t)encryptedMessage[i] < 16) Serial.print("0");
        Serial.print((uint8_t)encryptedMessage[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    Serial.print("Decrypted message: ");
    Serial.println(decryptedMessage);
    
    if (messageComplete) {
        Serial.println("Message reception complete - END symbol received.");
    } else {
        Serial.println("Warning: Message reception ended without receiving END symbol.");
        Serial.println("This may indicate message truncation or transmission issues.");
    }
}

void loop() {
    while (true) {
        if (receiveSyncPacket()) {
            receiveMessage();
            Serial.println("Message received. Waiting for next SYNC...");
        }
    }
}
