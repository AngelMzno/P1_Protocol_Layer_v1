/*
This file contains the implementations of function members,
both internal and API functions, as well as global 
variable declarations. It can also include #defines 
and data type declarations that are used only in this 
file.
*/

#include "fsl_debug_console.h"
#include "protocol_layer.h"
#include "fsl_enet.h"   // library of Ethernet from SDK
#include "aes.h"        // libray from https://github.com/kokke/tiny-AES-c
#include "fsl_crc.h"  // library of CRC from SDK
#include "app.h"        // library of Ethernet from SDK
#include "protocol_layer_cfg.h"  // Include the configuration header

/*******************************************************************************
 * Variables
 ******************************************************************************/
enet_handle_t g_handle;
uint8_t g_frame[ENET_DATA_LENGTH + 14];
uint8_t g_macAddr[6] = MAC_ADDRESS;
phy_handle_t phyHandle;
#if ((EXAMPLE_USES_LOOPBACK_CABLE) && defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
bool linkChange = false;
#endif

phy_config_t phyConfig = {0};
uint32_t testTxNum = 0;
uint32_t length = 0;
enet_data_error_stats_t eErrStatic;
status_t status;
enet_config_t config;
#if EXAMPLE_USES_LOOPBACK_CABLE
volatile uint32_t count = 0;
phy_speed_t speed;
phy_duplex_t duplex;
bool autonego = false;
bool link = false;
bool tempLink = false;
#endif

/*! @brief Buffer descriptors should be in non-cacheable region and should be align to "ENET_BUFF_ALIGNMENT". */
AT_NONCACHEABLE_SECTION_ALIGN(enet_rx_bd_struct_t g_rxBuffDescrip[ENET_RXBD_NUM], ENET_BUFF_ALIGNMENT);
AT_NONCACHEABLE_SECTION_ALIGN(enet_tx_bd_struct_t g_txBuffDescrip[ENET_TXBD_NUM], ENET_BUFF_ALIGNMENT);
/*! @brief The data buffers can be in cacheable region or in non-cacheable region.
 * If use cacheable region, the alignment size should be the maximum size of "CACHE LINE SIZE" and "ENET_BUFF_ALIGNMENT"
 * If use non-cache region, the alignment size is the "ENET_BUFF_ALIGNMENT".
 */
SDK_ALIGN(uint8_t g_rxDataBuff[ENET_RXBD_NUM][SDK_SIZEALIGN(ENET_RXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT)],
          APP_ENET_BUFF_ALIGNMENT);
SDK_ALIGN(uint8_t g_txDataBuff[ENET_TXBD_NUM][SDK_SIZEALIGN(ENET_TXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT)],
          APP_ENET_BUFF_ALIGNMENT);




/*******************************************************************************
 * Code
 ******************************************************************************/

// Function to initialize the protocol layer
void ProtocolLayer_init(void) {


    // Initialize Ethernet interface
    PRINTF("\r\nENET example start.\r\n");

    /* Prepare the buffer configuration. */
    enet_buffer_config_t buffConfig[] = {{
        ENET_RXBD_NUM,
        ENET_TXBD_NUM,
        SDK_SIZEALIGN(ENET_RXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT),
        SDK_SIZEALIGN(ENET_TXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT),
        &g_rxBuffDescrip[0],
        &g_txBuffDescrip[0],
        &g_rxDataBuff[0][0],
        &g_txDataBuff[0][0],
        true,
        true,
        NULL,
    }};

    /* Get default configuration. */
    /*
     * config.miiMode = kENET_RmiiMode;
     * config.miiSpeed = kENET_MiiSpeed100M;
     * config.miiDuplex = kENET_MiiFullDuplex;
     * config.rxMaxFrameLen = ENET_FRAME_MAX_FRAMELEN;
     */
    ENET_GetDefaultConfig(&config);

    /* The miiMode should be set according to the different PHY interfaces. */
#ifdef EXAMPLE_PHY_INTERFACE_RGMII
    config.miiMode = kENET_RgmiiMode;
#else
    config.miiMode = kENET_RmiiMode;
#endif
    phyConfig.phyAddr = EXAMPLE_PHY_ADDRESS;
#if EXAMPLE_USES_LOOPBACK_CABLE
    phyConfig.autoNeg = true;
#else
    phyConfig.autoNeg = false;
    config.miiDuplex  = kENET_MiiFullDuplex;
#endif
    phyConfig.ops      = EXAMPLE_PHY_OPS;
    phyConfig.resource = EXAMPLE_PHY_RESOURCE;
#if (defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
    phyConfig.intrType = kPHY_IntrActiveLow;
#endif

    /* Initialize PHY and wait auto-negotiation over. */
    PRINTF("Wait for PHY init...\r\n");
#if EXAMPLE_USES_LOOPBACK_CABLE
    do
    {
        status = PHY_Init(&phyHandle, &phyConfig);
        if (status == kStatus_Success)
        {
            PRINTF("Wait for PHY link up...\r\n");
            /* Wait for auto-negotiation success and link up */
            count = PHY_AUTONEGO_TIMEOUT_COUNT;
            do
            {
                PHY_GetLinkStatus(&phyHandle, &link);
                if (link)
                {
                    PHY_GetAutoNegotiationStatus(&phyHandle, &autonego);
                    if (autonego)
                    {
                        break;
                    }
                }
            } while (--count);
            if (!autonego)
            {
                PRINTF("PHY Auto-negotiation failed. Please check the cable connection and link partner setting.\r\n");
            }
        }
    } while (!(link && autonego));
#else
    while (PHY_Init(&phyHandle, &phyConfig) != kStatus_Success)
    {
        PRINTF("PHY_Init failed\r\n");
    }

    /* set PHY link speed/duplex and enable loopback. */
    PHY_SetLinkSpeedDuplex(&phyHandle, (phy_speed_t)config.miiSpeed, (phy_duplex_t)config.miiDuplex);
    PHY_EnableLoopback(&phyHandle, kPHY_LocalLoop, (phy_speed_t)config.miiSpeed, true);
#endif /* EXAMPLE_USES_LOOPBACK_CABLE */

#if PHY_STABILITY_DELAY_US
    /* Wait a moment for PHY status to be stable. */
    SDK_DelayAtLeastUs(PHY_STABILITY_DELAY_US, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
#endif

#if EXAMPLE_USES_LOOPBACK_CABLE
    /* Get the actual PHY link speed and set in MAC. */
    PHY_GetLinkSpeedDuplex(&phyHandle, &speed, &duplex);
    config.miiSpeed  = (enet_mii_speed_t)speed;
    config.miiDuplex = (enet_mii_duplex_t)duplex;
#endif

#ifndef USER_DEFINED_MAC_ADDRESS
    /* Set special address for each chip. */
uint8_t srcMacAddr[] = SRC_MAC_ADDRESS;
    memcpy(g_macAddr, srcMacAddr, 6);
#else
    SILICONID_ConvertToMacAddr(&g_macAddr);
#endif

    /* Init the ENET. */
    ENET_Init(EXAMPLE_ENET, &g_handle, &config, &buffConfig[0], &g_macAddr[0], EXAMPLE_CLOCK_FREQ);
    ENET_ActiveRead(EXAMPLE_ENET);

}

// Function to send an encrypted message with CRC32 over Ethernet
void ProtocolLayer_send(const uint8_t* message, size_t length) {
    // Encrypt the message
    // Calculate CRC32
    // Send the message over Ethernet
}

// Function to receive a message from Ethernet, verify the CRC32, and decrypt it
void ProtocolLayer_receive(uint8_t* buffer, size_t length) {
    // Receive the message from Ethernet
    // Verify CRC32
    // Decrypt the message
}

/*! @brief Build Frame for transmit. */
void ENET_BuildBroadCastFrame(void)
{
    uint32_t count  = 0;
    uint32_t length = ENET_DATA_LENGTH - 14;
uint8_t destMacAddr[] = DEST_MAC_ADDRESS;  // Use the defined destination MAC address

    for (count = 0; count < 6U; count++)
    {
        g_frame[count] = destMacAddr[count];  // Set the destination MAC address
    }
    memcpy(&g_frame[6], &g_macAddr[0], 6U);  // Source MAC address
    g_frame[12] = (length >> 8) & 0xFFU;
    g_frame[13] = length & 0xFFU;

    for (count = 0; count < length; count++)
    {
        g_frame[count + 14] = count % 0xFFU;
    }
}

#if (defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
void PHY_LinkStatusChange(void)
{
#if (EXAMPLE_USES_LOOPBACK_CABLE)
    linkChange = true;
#endif
}
#endif
