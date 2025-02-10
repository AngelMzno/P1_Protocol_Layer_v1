#ifndef _PROTOCOL_LAYER_H_
#define _PROTOCOL_LAYER_H_

#include <stdint.h>
#include <stddef.h>

// Function to initialize the protocol layer
void ProtocolLayer_init(void);

// Function to send an encrypted message with CRC32 over Ethernet
void ProtocolLayer_send(const uint8_t* message, size_t length);

// Function to receive a message from Ethernet, verify the CRC32, and decrypt it
void ProtocolLayer_receive(uint8_t* buffer, size_t length);

#endif // _PROTOCOL_LAYER_H_


