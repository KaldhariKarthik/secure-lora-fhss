---

# 🔒 Secure LoRa FHSS Communication System

A lightweight yet powerful wireless communication system built on **ESP32** and **LoRa SX127x modules**, utilizing **Frequency-Hopping Spread Spectrum (FHSS)** and **entropy-based encryption** for secure data transmission.

---

## ✨ Features

- 📡 **41 Narrow-Band Channels**  
  Operates across 41 channels (10 kHz each) within the 433 MHz ISM band

- 🔁 **Dynamic Frequency Hopping**  
  Utilizes pseudo-random hopping to reduce interception and jamming

- 🎲 **True Random Entropy**  
  Generates randomness from environmental analog noise (GPIO 34)

- 🔐 **Key Exchange via SYNC Packet**  
  Securely sends hopping pattern and encryption key during sync

- 🔑 **Dynamic XOR Encryption**  
  Encrypts each session with a unique key generated from noise + timestamp

- 🔁 **Reliable Delivery**  
  ACK-based retry mechanism ensures all chunks are received correctly

- 📶 **Efficient Bandwidth Use**  
  Minimizes spectral footprint using narrowband channels

---

## 🔧 Hardware Requirements

- 🧠 ESP32 Dev Board (e.g., DOIT, NodeMCU ESP32)
- 📻 LoRa SX1276/SX1278 Module
- 🎚️ Analog noise source (unconnected pin on GPIO 34)

---

## 🛠️ Wiring Diagram (Pin Configuration)

```cpp
// LoRa SX1278 to ESP32 Pin Mapping

#define SS         5     // SPI Chip Select
#define RST       14     // Reset Pin
#define DIO0       2     // Interrupt Pin (used for RX done)

// SPI Pins (ESP32 default)
#define SCK       18     // SPI Clock
#define MISO      19     // SPI MISO
#define MOSI      23     // SPI MOSI

// Noise Input
#define NOISE_PIN 34     // Analog input for entropy generation
```

---

## 🧱 System Architecture

The system operates as a **point-to-point** communication link with the following architecture:

### 1️⃣ Transmitter
- Generates a frequency-hopping sequence using entropy
- Creates an encryption key
- Sends a **SYNC packet** to initiate communication
- Encrypts and transmits the message in chunks across multiple frequencies

### 2️⃣ Receiver
- Listens for the SYNC packet
- Extracts the hopping sequence and decryption key
- Listens on correct frequencies
- Decrypts incoming message chunks
- Sends ACKs for each chunk

---

## 🔄 Communication Protocol

### 🔁 SYNC Phase
- Transmitter sends a SYNC packet containing:
  - Hopping sequence
  - Encryption key
- Receiver replies with an ACK to confirm sync

### 📤 Data Phase
- Message is encrypted and split into chunks
- Each chunk is sent over a different frequency (pseudo-randomly chosen)
- Receiver sends ACK for each chunk
- Transmission ends with an "END" marker

---

## 🔐 Entropy-Based Randomness

Randomness is crucial for security. This system uses:

- Unconnected analog pin (GPIO 34) to capture environmental noise
- System timestamp to ensure time-based uniqueness
- An **avalanche hash-like method** to combine these sources
- Output used for:
  - Frequency hopping sequences
  - Session-specific encryption keys

---

## ▶️ How to Use

1. Flash `transmitter.ino` on ESP32-TX
2. Flash `receiver.ino` on ESP32-RX
3. Open the **serial monitor** on transmitter
4. Enter a message to send
5. Watch decrypted message appear on the receiver's serial monitor

---

## 🔐 Security Notes

- XOR encryption is **simple** and not production-grade  
  ➤ Upgrade to AES or ChaCha20 for stronger security

- SYNC packet is sent **in plaintext**  
  ➤ Use pre-shared keys or Diffie-Hellman for secure key exchange

- Physical access can compromise the system  
  ➤ Consider secure boot or hardware encryption on ESP32

---

## 🚀 Future Improvements

- ✅ Add **Message Authentication Code (MAC)** for integrity verification  
- ✅ Integrate **AES-128** or **ChaCha20** for robust encryption  
- ✅ Support for **multi-receiver** broadcast environments  
- ✅ Optimize for **low-power use** in battery-operated deployments  
- ✅ Add **session timeout** or rekeying mechanisms

---

## 📜 License

This project is licensed under the [MIT License](LICENSE).  
Copyright © 2025 [Kaldhari Karthik]

---

## 🤝 Contributing

Found a bug or have an idea to improve this system? Contributions are welcome!  
- Fork the repo  
- Make your changes  
- Submit a pull request 🚀

---
