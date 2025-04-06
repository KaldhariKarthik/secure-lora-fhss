#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2

#define SYNC_FREQ 433.05  // Fixed SYNC Frequency
#define CHANNEL_WIDTH 0.01  // 10 kHz = 0.01 MHz channel width

#define NOISE_PIN 34    // Single noise input pin - using ADC1_CH6 (GPIO 34)

// Define a larger set of base frequencies with 10kHz spacing
// Range from 433.10 MHz to 433.50 MHz with 10kHz spacing
const int numFrequencies = 41;  // (433.50 - 433.10) / 0.01 + 1
float frequencies[numFrequencies];

// Encryption key (will be derived from noise entropy)
uint8_t encryptionKey[16];

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("LoRa Transmitter with Multiple 10kHz Channels and Encryption");
    LoRa.setPins(SS, RST, DIO0);
    
    if (!LoRa.begin(SYNC_FREQ * 1E6)) {
        Serial.println("LoRa initialization failed. Check connections.");
        while (true);
    }
    
    // Initialize ADC for higher resolution
    analogReadResolution(12); // 12-bit resolution
    
    Serial.println("LoRa initialized. Ready for input...");
}

// Function to generate random frequency hopping pattern
class NoiseEntropy {
private:
    uint32_t collectNoiseSample() {
        uint32_t sample = 0;
        int minVal = 4095, maxVal = 0;
        int readings[64];
        
        // Take multiple readings to assess noise range
        for (int i = 0; i < 64; i++) {
            readings[i] = analogRead(NOISE_PIN);
            if (readings[i] < minVal) minVal = readings[i];
            if (readings[i] > maxVal) maxVal = readings[i];
            delayMicroseconds(1);  // Short delay between readings
        }
        
        // Skip collection if there's not enough variation
        if (maxVal - minVal < 3) {
            Serial.println("Warning: Low noise variation detected");
            // Use system time as fallback entropy source
            return micros() ^ (millis() << 10);
        }
        
        // Collect 32 bits of noise
        for (int i = 0; i < 32; i++) {
            // Take multiple readings and compare
            int val1 = analogRead(NOISE_PIN);
            delayMicroseconds(3);
            int val2 = analogRead(NOISE_PIN);
            delayMicroseconds(7);
            int val3 = analogRead(NOISE_PIN);
            
            // Extract bit from least significant bits of multiple readings
            bool bit = ((val1 & 0x01) ^ (val2 & 0x01)) ^ (val3 & 0x01);
            
            // Add to sample
            sample = (sample << 1) | bit;
            
            // Variable delay based on reading
            delayMicroseconds(val1 & 0x0F);
        }
        
        // Debug output
        Serial.print("Noise sample: ");
        Serial.println(sample, HEX);
        
        return sample;
    }

public:
    void begin() {
        pinMode(NOISE_PIN, INPUT);
        // Test noise source
        Serial.println("Testing noise source...");
        int readings[10];
        for (int i = 0; i < 10; i++) {
            readings[i] = analogRead(NOISE_PIN);
            Serial.print(readings[i]);
            Serial.print(" ");
            delay(5);
        }
        Serial.println();
    }
    
    uint32_t getEntropy() {
        // Collect multiple noise samples
        uint32_t noise1 = collectNoiseSample();
        delay(5);
        uint32_t noise2 = collectNoiseSample();
        delay(7);
        uint32_t noise3 = collectNoiseSample();
        
        // Mix noise samples with system time
        uint32_t timeEntropy = micros() ^ (millis() << 16);
        uint32_t entropy = noise1 ^ (noise2 << 11) ^ (noise3 >> 7) ^ timeEntropy;
        
        // Avalanche effect (improved mixing)
        entropy = (entropy ^ (entropy >> 16)) * 0x85ebca6b;
        entropy = (entropy ^ (entropy >> 13)) * 0xc2b2ae35;
        entropy = entropy ^ (entropy >> 16);
        
        return entropy;
    }
} noiseSource;

// Generate encryption key from noise entropy
void generateEncryptionKey() {
    for (int i = 0; i < 16; i++) {
        encryptionKey[i] = (uint8_t)(noiseSource.getEntropy() & 0xFF);
    }
    
    Serial.println("Generated encryption key: ");
    for (int i = 0; i < 16; i++) {
        if (encryptionKey[i] < 16) Serial.print("0");
        Serial.print(encryptionKey[i], HEX);
        if (i < 15) Serial.print(" ");
    }
    Serial.println();
}

// Simple XOR encryption/decryption function
String encryptMessage(const String& message) {
    String encrypted = "";
    for (unsigned int i = 0; i < message.length(); i++) {
        // XOR each character with corresponding key byte (with wrap-around)
        encrypted += (char)(message[i] ^ encryptionKey[i % 16]);
    }
    return encrypted;
}

void generateRandomSequence() {
    static bool initialized = false;
    if (!initialized) {
        noiseSource.begin();
        initialized = true;
    }
    
    // Populate initial frequencies array
    for (int i = 0; i < numFrequencies; i++) {
        frequencies[i] = 433.10 + (i * CHANNEL_WIDTH);
    }
    
    // Create temporary array for sorting with entropy values
    struct FreqEntropyPair {
        float frequency;
        uint32_t entropyValue;
    };
    FreqEntropyPair* pairs = new FreqEntropyPair[numFrequencies];
    
    // Collect entropy for each frequency
    for (int i = 0; i < numFrequencies; i++) {
        pairs[i].frequency = frequencies[i];
        pairs[i].entropyValue = noiseSource.getEntropy();
    }
    
    // Enhanced shuffle using entropy values
    for (int i = numFrequencies - 1; i > 0; i--) {
        uint32_t j = noiseSource.getEntropy() % (i + 1);
        
        // Swap
        FreqEntropyPair temp = pairs[i];
        pairs[i] = pairs[j];
        pairs[j] = temp;
    }
    
    // Copy randomized frequencies back to main array
    for (int i = 0; i < numFrequencies; i++) {
        frequencies[i] = pairs[i].frequency;
    }
    
    delete[] pairs;
}

void sendSyncPacket() {
    // Generate a new randomized frequency sequence
    generateRandomSequence();
    
    // Generate a new encryption key for this session
    generateEncryptionKey();
    
    // Create SYNC message with the directly shared hopping sequence
    String syncMessage = "SYNC|" + String(numFrequencies) + "|";
    
    // Add the actual randomized frequencies to use (first 20 only to keep packet size reasonable)
    int frequenciesToSend = min(20, numFrequencies);
    syncMessage += String(frequenciesToSend) + "|";
    
    for (int i = 0; i < frequenciesToSend; i++) {
        syncMessage += String(frequencies[i], 3); // Three decimal places precision (needed for 10kHz)
        if (i < frequenciesToSend - 1) syncMessage += ",";
    }
    
    // Add encryption key to the sync message
    syncMessage += "|";
    for (int i = 0; i < 16; i++) {
        if (encryptionKey[i] < 16) syncMessage += "0";
        syncMessage += String(encryptionKey[i], HEX);
    }

    LoRa.setFrequency(SYNC_FREQ * 1E6);
    LoRa.beginPacket();
    LoRa.print(syncMessage);
    LoRa.endPacket();
    
    Serial.println("Sent SYNC with randomized frequency sequence (first 5 shown):");
    for (int i = 0; i < 5; i++) {
        Serial.print(frequencies[i], 3);
        Serial.print(" MHz");
        if (i < 4) Serial.print(", ");
    }
    Serial.println("...");
    Serial.print("Total channels: ");
    Serial.println(frequenciesToSend);
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

// In the sendMessage function, make these changes:

void sendMessage(const String& message) {
    while (true) {  // Keep retrying SYNC until ACK is received
        sendSyncPacket();
        if (waitForAck()) break;
        delay(1000);  // Prevent flooding
    }

    Serial.println("Starting Multi-Channel Frequency Hopping Transmission...");
    
    // Encrypt the message
    String encryptedMessage = encryptMessage(message);
    Serial.print("Original message: ");
    Serial.println(message);
    Serial.print("Encrypted message (hex): ");
    for (unsigned int i = 0; i < encryptedMessage.length(); i++) {
        if ((uint8_t)encryptedMessage[i] < 16) Serial.print("0");
        Serial.print((uint8_t)encryptedMessage[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    // Use only the first 20 frequencies for transmission (or whatever was shared in SYNC)
    int frequenciesToUse = min(20, numFrequencies);
    
    // Calculate how many characters we can send per frequency
    // Make sure we don't split the message into more chunks than we have frequencies
    int msgLength = encryptedMessage.length();
    int chunkSize = ceil((float)msgLength / frequenciesToUse);
    
    // Debug output
    Serial.print("Message length: ");
    Serial.print(msgLength);
    Serial.print(", Frequencies: ");
    Serial.print(frequenciesToUse);
    Serial.print(", Chunk size: ");
    Serial.println(chunkSize);

    int msgIndex = 0;

    for (int i = 0; i < frequenciesToUse && msgIndex < msgLength; i++) {
        String chunk = encryptedMessage.substring(msgIndex, min(msgIndex + chunkSize, msgLength));
        msgIndex += chunkSize;

        // Add END symbol if this is the last chunk
        if (msgIndex >= msgLength) {
            chunk += "<END>";
        }

        LoRa.setFrequency(frequencies[i] * 1E6);
        // Set bandwidth to 10kHz for narrow channel operation
        LoRa.setSignalBandwidth(10E3);
        
        Serial.print("Hopping to: ");
        Serial.print(frequencies[i], 3);
        Serial.print(" MHz (Channel ");
        Serial.print(i+1);
        Serial.print(" of ");
        Serial.print(frequenciesToUse);
        Serial.println(")");

        LoRa.beginPacket();
        LoRa.print(chunk);
        LoRa.endPacket();

        Serial.print("Sent chunk ");
        Serial.print(i+1);
        Serial.print("/");
        Serial.print(ceil((float)msgLength / chunkSize));
        Serial.print(" (");
        Serial.print(chunk.length());
        Serial.print(" bytes): ");
        // Print hex values to avoid control characters
        for (unsigned int j = 0; j < chunk.length(); j++) {
            if ((uint8_t)chunk[j] < 16) Serial.print("0");
            Serial.print((uint8_t)chunk[j], HEX);
            Serial.print(" ");
        }
        Serial.println();

        if (!waitForAck()) {
            i--;  // Resend if ACK not received
        }
        
        // Reset to default bandwidth after transmission
        LoRa.setSignalBandwidth(125E3);
    }

    Serial.println("Transmission Complete.");
}

void loop() {
    if (Serial.available()) {
        String message = Serial.readString();
        sendMessage(message);
    }
}
