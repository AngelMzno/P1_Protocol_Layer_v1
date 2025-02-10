# Practice 1: Communication Protocol Layer 🌐

This practice involves designing and implementing a communication protocol layer in C for the FRDM-RW612 board using the mcuXpresso SDK.  The layer provides message encryption and integrity verification services.  This project demonstrates skills in layered software development and secure communication protocols.

**Objectives:** 🎯

* To practice layered communication software development. 🧱
* To implement a protocol layer that includes:
    * Message integrity validation using CRC32. 🔒
    * Message encryption using AES128. 🔐
* To test the protocol layer through bidirectional communication between the FRDM-RW612 and a computer using Ethernet. 💻

**Functionality:** ⚙️

The implemented library (`ProtocolLayer.c`, `ProtocolLayer.h`, `ProtocolLayer_cfg.h`) offers these functions:

* `ProtocolLayer_init()`: Initializes the protocol layer (including the Ethernet interface).
* `ProtocolLayer_send()`: Sends an encrypted message with CRC32 over Ethernet.  ➡️
* `ProtocolLayer_receive()`: Receives a message from Ethernet, verifies the CRC32, and decrypts it. ⬅️

**Ethernet Packet Format:** 📦

The protocol uses Ethernet packets with this format:

* 6 bytes: Destination MAC address
* 6 bytes: Source MAC address
* 2 bytes: Data length (excluding MAC addresses and CRC32)
* n bytes: Encrypted data (minimum 48 bytes, maximum 1488 bytes)
* 4 bytes: CRC32

**Libraries:** 📚

* **tiny-AES-c:** For AES128 encryption. (Link: [https://github.com/kokke/tiny-AES-c](https://github.com/kokke/tiny-AES-c))
* **mcuXpresso SDK CRC32 Library:** For CRC32 calculation.

**Testing:** 🧪

The library is tested with a Python application (on the PC side) that exchanges 32 packets of varying sizes and contents with the FRDM-RW612.  Predefined messages and responses are used to validate functionality.

**Repository Structure:** 📁

* `ProtocolLayer.c`: Library implementation.
* `ProtocolLayer.h`: Library header file.
* `ProtocolLayer_cfg.h`: Library configuration file (AES key, IV).
* `[Python Application]:` Python test application (PC side).
* `[FRDM-RW612 Test Code]:` Code to test the library on the FRDM-RW612 board.
* `README.md`: This file.


**Author:** Miguel Angel Manzano Hernandez

**GitHub:** [https://github.com/AngelMzno](https://github.com/AngelMzno)
