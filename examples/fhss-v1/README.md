# FHSS Version 1 – LoRa Frequency Hopping Example

This example demonstrates a basic implementation of **Frequency Hopping Spread Spectrum (FHSS)** using LoRa SX127x modules and ESP32 boards.

---

## 🧠 What This Does

- Uses a fixed **SYNC frequency (433.05 MHz)** to initiate communication.
- The transmitter sends a `SYNC` packet with:
  - Number of channels
  - Frequencies to be used for hopping
  - A timestamp
- The receiver parses the SYNC message and replies with an **ACK**.
- The transmitter then hops across the provided frequency list and sends parts of the encrypted message chunk-by-chunk.
- The receiver reconstructs the message and sends ACKs for each chunk.

---

## 🛠 Requirements

- 2x **ESP32** boards  
- 2x **LoRa SX1278** modules  
- Arduino IDE (with [LoRa library](https://github.com/sandeepmistry/arduino-LoRa))

---

## 📁 Files

- `transmitter.ino`:  
  Run this on the transmitting ESP32. Waits for user input on Serial and transmits it using frequency hopping.

- `receiver.ino`:  
  Run this on the receiving ESP32. Listens for SYNC and reconstructs the original message from transmitted chunks.

---

## ⚙️ Default Settings

| Parameter         | Value              |
|------------------|--------------------|
| SYNC Frequency   | `433.05 MHz`       |
| Hopping Channels | `433.10`, `433.30`, `433.50`, `433.70` MHz |
| Channel Width    | Approx. `200 kHz`  |
| ACK Timeout      | `2 seconds`        |

---

## 🚀 Getting Started

1. Upload `transmitter.ino` to one ESP32 and `receiver.ino` to the other.
2. Open Serial Monitors at 115200 baud.
3. Enter a message in the transmitter's Serial Monitor.
4. The system handles synchronization, frequency hopping, ACKs, and reconstruction automatically.

---

## 📦 Future Versions

This is version `v1`. Future versions will add:

- True pseudo-random frequency hopping (PRNG or analog entropy)
- AES encryption
- Packet validation
- Parallel frequency transmission
- CRC and security improvements

---
