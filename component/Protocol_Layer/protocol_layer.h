/*
This file must contain the definitions of the library's 
interface, the API itself. This usually includes function 
prototypes, data type definitions, or definitions that are 
necessary to use the library.
*/

#ifndef _PROTOCOL_LAYER_H_
#define _PROTOCOL_LAYER_H_

#include <stdint.h>
#include <stddef.h>
#include "fsl_enet.h"
#include "fsl_phy.h"
#include "protocol_layer_cfg.h"

#include "aes.h"        // libray from https://github.com/kokke/tiny-AES-c
#include "fsl_crc.h"  // library of CRC from SDK
#include "app.h"        // library of Ethernet from SDK

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define ENET_RXBD_NUM          (4)
#define ENET_TXBD_NUM          (4)
#define ENET_RXBUFF_SIZE       (ENET_FRAME_MAX_FRAMELEN)
#define ENET_TXBUFF_SIZE       (ENET_FRAME_MAX_FRAMELEN)
#define ENET_DATA_LENGTH       (1000)
#define ENET_TRANSMIT_DATA_NUM (20)
#ifndef APP_ENET_BUFF_ALIGNMENT
#define APP_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT
#endif
#ifndef PHY_AUTONEGO_TIMEOUT_COUNT
#define PHY_AUTONEGO_TIMEOUT_COUNT (300000)
#endif
#ifndef EXAMPLE_PHY_LINK_INTR_SUPPORT
#define EXAMPLE_PHY_LINK_INTR_SUPPORT (0U)
#endif
#ifndef EXAMPLE_USES_LOOPBACK_CABLE
#define EXAMPLE_USES_LOOPBACK_CABLE (1U)
#endif

#ifndef PHY_STABILITY_DELAY_US
#if EXAMPLE_USES_LOOPBACK_CABLE
#define PHY_STABILITY_DELAY_US (0U)
#else
/* If cable is not used there is no "readiness wait" caused by auto negotiation. Lets wait 100ms.*/
#define PHY_STABILITY_DELAY_US (100000U)
#endif
#endif

/* @TEST_ANCHOR */

#ifndef MAC_ADDRESS
#define MAC_ADDRESS                        \
    {                                      \
        0x54, 0x27, 0x8d, 0x00, 0x00, 0x00 \
    }
#else
#define USER_DEFINED_MAC_ADDRESS
#endif


/*******************************************************************************
 * Variables
 ******************************************************************************/
extern enet_handle_t g_handle;
extern uint8_t g_frame[ENET_DATA_LENGTH + 14];
extern uint8_t g_macAddr[6];
extern phy_handle_t phyHandle;
#if ((EXAMPLE_USES_LOOPBACK_CABLE) && defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
extern bool linkChange;
#endif

extern phy_config_t phyConfig;
extern uint32_t testTxNum;
extern uint32_t length;
extern enet_data_error_stats_t eErrStatic;
extern status_t status;
extern enet_config_t config;
#if EXAMPLE_USES_LOOPBACK_CABLE
extern volatile uint32_t count;
extern phy_speed_t speed;
extern phy_duplex_t duplex;
extern bool autonego;
extern bool link;
extern bool tempLink;
#endif


/*******************************************************************************
 * Prototypes
 ******************************************************************************/


void ProtocolLayer_init(void);
void ProtocolLayer_send(const uint8_t* message, size_t length);
void ProtocolLayer_receive(uint8_t* buffer, size_t length);

void ENET_BuildBroadCastFrame(void);
#if (defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
void GPIO_EnableLinkIntr(void);
#endif

#endif // _PROTOCOL_LAYER_H_


