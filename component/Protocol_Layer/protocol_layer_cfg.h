/*
Este archivo contiene las configuraciones de la biblioteca, como la clave de encriptación y el vector de inicialización para AES.
*/

#ifndef _PROTOCOL_LAYER_CFG_H_
#define _PROTOCOL_LAYER_CFG_H_

#define AES_KEY {"My16byteKey00000"}
#define AES_IV  {"My16byteKey00000"}

static const uint8_t aes_key[16] = "My16byteKey00000";
static const uint8_t aes_iv[16] = "My16byteKey00000";

#define DEST_MAC_ADDRESS {0x00, 0x2b, 0x67, 0x36, 0x70, 0x0F}
#define SRC_MAC_ADDRESS {0x54, 0x27, 0x8d, 0x24, 0x2a, 0xf2}

#endif // _PROTOCOL_LAYER_CFG_H_

