# Secure LoRa FHSS Communication System

A secure wireless communication system built for ESP32 devices using LoRa technology with Frequency-Hopping Spread Spectrum (FHSS) and entropy-based encryption.

## Features

- **Multiple Narrow-Band Channels**: Operates across 41 different 10kHz-wide channels in the 433MHz band
- **Frequency Hopping**: Dynamically changes transmission frequency for enhanced security and interference resistance
- **Environmental Entropy**: Uses analog noise sampling for true random number generation
- **Secure Key Exchange**: Implements encryption key generation and exchange via sync packets
- **Simple XOR Encryption**: Provides basic message security with dynamically generated keys
- **Automatic Retransmission**: Ensures reliable message delivery with ACK/retry mechanism
- **Bandwidth Optimization**: Uses narrow 10kHz channels for efficient spectrum usage

## Hardware Requirements

- ESP32 development board
- LoRa module (e.g., SX1276/SX1278)
- Analog noise source connected to GPIO 34

## Pin Configuration

```
#define SS 5     // LoRa SPI Chip Select
#define RST 14   // LoRa Reset
#define DIO0 2   // LoRa DIO0 (Interrupt)
#define NOISE_PIN 34  // Analog noise input
```

## System Architecture

The system consists of two main components:

1. **Transmitter**: Generates random frequency hopping sequence, encrypts messages, and sends data across multiple channels
2. **Receiver**: Listens for sync packets, acknowledges receipt, follows the frequency hopping pattern, and decrypts messages

### Communication Protocol

1. **SYNC Phase**:
   - Transmitter sends a SYNC packet containing the frequency hopping sequence and encryption key
   - Receiver acknowledges with an ACK
   
2. **Data Transmission Phase**:
   - Message is encrypted and divided into chunks
   - Each chunk is transmitted on a different frequency
   - Receiver acknowledges each chunk
   - END marker indicates completion

## Entropy Generation

The system uses environmental noise for generating secure random numbers:

- Samples analog noise from an unconnected pin
- Combines with system time for additional entropy
- Implements avalanche effect for better distribution
- Generates both frequency hopping patterns and encryption keys

## Usage

1. Upload the transmitter code to one ESP32 device
2. Upload the receiver code to another ESP32 device
3. Connect to the transmitter's serial monitor
4. Enter a message to send
5. View the received and decrypted message on the receiver's serial monitor

## Security Considerations

- The basic XOR encryption can be enhanced for production use
- Physical security of devices is required as keys are exchanged in plaintext
- Consider adding authentication mechanisms for production use
- The entropy system works best with a proper noise source

## License

MIT License

Copyright (c) 2025 KaldhariKarthik

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Contributing

Contributions to improve the system are welcome. Please feel free to submit a pull request.

## Future Improvements

- Add message authentication code (MAC) for integrity verification
- Implement more robust encryption algorithms
- Add support for multiple receivers
- Optimize power consumption for battery-powered operation
