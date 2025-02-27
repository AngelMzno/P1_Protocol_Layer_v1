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
 * Definitions
 ******************************************************************************/
#define ENET_RXBD_NUM          (4)
#define ENET_TXBD_NUM          (4)
#define ENET_RXBUFF_SIZE       (ENET_FRAME_MAX_FRAMELEN)
#define ENET_TXBUFF_SIZE       (ENET_FRAME_MAX_FRAMELEN)
#define ENET_DATA_LENGTH       (1000)
#define ENET_TRANSMIT_DATA_NUM (20)

#define CRC32_DATA_SIZE        (4)
#define MAC_DATA_SIZE          (6)
#define HEADER_MSG_SIZE        (14 + CRC32_DATA_SIZE)

#define DATA_LENGTH_INDEX      (12)
#define DATA_BUFFER_INDEX      (14)

#define APP_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT
#define PHY_AUTONEGO_TIMEOUT_COUNT (300000)

#define SWAP16(value) (((value >> 8) & 0x00FF) | ((value << 8) & 0xFF00))

/*******************************************************************************
 * Data Types
 ******************************************************************************/
typedef struct
{
    uint8_t MACdst[MAC_DATA_SIZE];
    uint8_t MACsrc[MAC_DATA_SIZE];
    uint16_t DataLength;
    uint8_t DataBuffer[ENET_DATA_LENGTH];
} tstEthMsg;

/*******************************************************************************
 * Variables
 ******************************************************************************/
AT_NONCACHEABLE_SECTION_ALIGN(enet_rx_bd_struct_t g_rxBuffDescrip[ENET_RXBD_NUM], ENET_BUFF_ALIGNMENT);
AT_NONCACHEABLE_SECTION_ALIGN(enet_tx_bd_struct_t g_txBuffDescrip[ENET_TXBD_NUM], ENET_BUFF_ALIGNMENT);
SDK_ALIGN(uint8_t g_rxDataBuff[ENET_RXBD_NUM][SDK_SIZEALIGN(ENET_RXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT)],
          APP_ENET_BUFF_ALIGNMENT);
SDK_ALIGN(uint8_t g_txDataBuff[ENET_TXBD_NUM][SDK_SIZEALIGN(ENET_TXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT)],
          APP_ENET_BUFF_ALIGNMENT);

enet_handle_t g_handle;
phy_handle_t phyHandle;
static CRC_Type *CRC_base = CRC_ENGINE;

static uint8_t key[16] = AES_KEY;
static uint8_t iv[16] = AES_IV;

uint8_t g_frame[ENET_DATA_LENGTH + 14]; 
uint8_t g_macAddr[6] = SRC_MAC_ADDRESS;

/*******************************************************************************
 * Private functions
 ******************************************************************************/
/*! @brief Initialize PHY. */
static void ProtocolLayer_initPHY(void)
{
    volatile uint32_t count = 0;
    phy_config_t phyConfig = {0};
    status_t status;

    bool link = false;
    bool autonego = false;

    phyConfig.phyAddr = EXAMPLE_PHY_ADDRESS;
    phyConfig.autoNeg = true;
    phyConfig.ops = EXAMPLE_PHY_OPS;
    phyConfig.resource = EXAMPLE_PHY_RESOURCE;

    do
    {
        status = PHY_Init(&phyHandle, &phyConfig);
        if (status == kStatus_Success)
        {
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
}

/*! @brief Apply padding to the data. */
static void ApplyPadding(uint8_t* data, size_t length, uint8_t* paddedData, size_t* paddedLength)
{
    uint16_t padSize = AES_BLOCKLEN - (length % AES_BLOCKLEN);
    *paddedLength = length + padSize;

    memcpy(paddedData, data, length);
    memset(paddedData + length, padSize, padSize);
}

/*! @brief Remove padding from the data. */
static void RemovePadding(uint8_t* data, size_t length, size_t* dataLength)
{
    uint8_t padValue = data[length - 1];
    if (padValue > AES_BLOCKLEN)
    {
        PRINTF("Incorrect padding.\r\n");
        *dataLength = 0;
        return;
    }

    for (size_t i = 0; i < padValue; i++)
    {
        if (data[length - 1 - i] != padValue)
        {
            PRINTF("Incorrect padding.\r\n");
            *dataLength = 0;
            return;
        }
    }

    *dataLength = length - padValue;
}

/*! @brief Initialize CRC32 configuration. */
void ProtocolLayer_initCRC32(void)
{
    crc_config_t config;

    config.polynomial = kCRC_Polynomial_CRC_32;
    config.reverseIn = true;
    config.complementIn = false;
    config.reverseOut = true;
    config.complementOut = true;
    config.seed = 0xFFFFFFFFU;

    CRC_Init(CRC_base, &config);
}

/*! @brief Check the CRC32 of the received message. */
static bool CheckCRC(const uint8_t* buffer, uint16_t length, CRC_Type* crcBase)
{
    uint32_t receivedCRC = 0;
    uint32_t calculatedCRC = 0;

    memcpy(&receivedCRC, &buffer[length], CRC32_DATA_SIZE);

    ProtocolLayer_initCRC32();
    CRC_WriteData(crcBase, buffer, length);
    calculatedCRC = CRC_Get32bitResult(crcBase);

    return (receivedCRC == calculatedCRC);
}

/*******************************************************************************
 * Global functions
 ******************************************************************************/
/*! @brief Initialize the protocol layer, including the Ethernet interface. */
void ProtocolLayer_init(void)
{
    enet_config_t config;
    phy_speed_t speed;
    phy_duplex_t duplex;
    uint8_t g_macAddr[MAC_DATA_SIZE] = SRC_MAC_ADDRESS;

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

    ENET_GetDefaultConfig(&config);

    config.miiMode = kENET_RmiiMode;

    ProtocolLayer_initPHY();

    PHY_GetLinkSpeedDuplex(&phyHandle, &speed, &duplex);
    config.miiSpeed = (enet_mii_speed_t)speed;
    config.miiDuplex = (enet_mii_duplex_t)duplex;

    config.macSpecialConfig = kENET_ControlRxBroadCastRejectEnable;

    ENET_Init(EXAMPLE_ENET, &g_handle, &config, &buffConfig[0], &g_macAddr[0], EXAMPLE_CLOCK_FREQ);
    ENET_ActiveRead(EXAMPLE_ENET);

    // Initialize CRC32
    crc_config_t crcConfig;
    crcConfig.polynomial    = kCRC_Polynomial_CRC_32;
    crcConfig.reverseIn     = true;
    crcConfig.complementIn  = false;
    crcConfig.reverseOut    = true;
    crcConfig.complementOut = true;
    crcConfig.seed          = 0xFFFFFFFFU;
    CRC_Init(CRC_base, &crcConfig);
}

/*! @brief Send an encrypted message with CRC32 over Ethernet. */
void ProtocolLayer_send(const uint8_t* message, size_t length)
{
    struct AES_ctx ctx;
    uint32_t u32CRC = 0;
    size_t u16MsgLength = 0;
    bool link = false;

    tstEthMsg stMsgInfo = {
        .MACdst = DEST_MAC_ADDRESS,
        .MACsrc = SRC_MAC_ADDRESS,
    };

    // Apply padding and encrypt the data
    ApplyPadding((uint8_t*)message, length, stMsgInfo.DataBuffer, &u16MsgLength);
    AES_init_ctx_iv(&ctx, aes_key, aes_iv);
    AES_CBC_encrypt_buffer(&ctx, stMsgInfo.DataBuffer, u16MsgLength);

    // Calculate CRC32
    ProtocolLayer_initCRC32();
    CRC_WriteData(CRC_base, stMsgInfo.DataBuffer, u16MsgLength);
    u32CRC = CRC_Get32bitResult(CRC_base);

    // Append CRC32 to the DataBuffer
    memcpy(&stMsgInfo.DataBuffer[u16MsgLength], (uint8_t*)&u32CRC, CRC32_DATA_SIZE);

    // Set the length of the data
    stMsgInfo.DataLength = SWAP16(u16MsgLength + CRC32_DATA_SIZE);

    // Ensure the payload is at least 48 bytes and at most 1488 bytes
    size_t totalLength = HEADER_MSG_SIZE + u16MsgLength + CRC32_DATA_SIZE;
    if (totalLength < 48)
    {
        memset(stMsgInfo.DataBuffer + u16MsgLength + CRC32_DATA_SIZE, 0, 48 - totalLength);
        totalLength = 48;
    }
    else if (totalLength > 1488)
    {
        totalLength = 1488;
    }

    // Send the frame over Ethernet
    PHY_GetLinkStatus(&phyHandle, &link);
    if (link)
    {
        ENET_SendFrame(EXAMPLE_ENET, &g_handle, (uint8_t*)&stMsgInfo, totalLength, 0, false, NULL);
    }
}

/*! @brief Receive a message from Ethernet, verify the CRC32, and decrypt it. */
uint16_t ProtocolLayer_receive(uint8_t* msgBuffer)
{
    enet_data_error_stats_t eErrStatic;
    uint32_t length = 0;
    status_t status;
    uint16_t msgLength = 0;
    size_t unpadLength = 0;
    bool CRC_check = false;
    struct AES_ctx ctx;

    status = ENET_GetRxFrameSize(&g_handle, &length, 0);
    if (length != 0)
    {
        uint8_t *data = (uint8_t *)malloc(length);
        status = ENET_ReadFrame(EXAMPLE_ENET, &g_handle, data, length, 0, NULL);
        if (status == kStatus_Success)
        {
            memcpy((uint8_t*)&msgLength, &data[DATA_LENGTH_INDEX], sizeof(msgLength));
            msgLength = SWAP16(msgLength) - CRC32_DATA_SIZE;

            CRC_check = CheckCRC(&data[DATA_BUFFER_INDEX], (uint16_t)msgLength, CRC_base);
            if (CRC_check == true)
            {
                AES_init_ctx_iv(&ctx, aes_key, aes_iv);
                AES_CBC_decrypt_buffer(&ctx, &data[DATA_BUFFER_INDEX], msgLength);

                RemovePadding(&data[DATA_BUFFER_INDEX], msgLength, &unpadLength);
                if (unpadLength > 0)
                {
                    memcpy(msgBuffer, &data[DATA_BUFFER_INDEX], unpadLength);
                }
            }
            else
            {
                PRINTF("CRC incorrecto.\r\n");
            }
        }

        free(data);
    }
    else if (status == kStatus_ENET_RxFrameError)
    {
        ENET_GetRxErrBeforeReadFrame(&g_handle, &eErrStatic, 0);
        ENET_ReadFrame(EXAMPLE_ENET, &g_handle, NULL, 0, 0, NULL);
    }

    return unpadLength;
}

#if (defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
void PHY_LinkStatusChange(void)
{
#if (EXAMPLE_USES_LOOPBACK_CABLE)
    linkChange = true;
#endif
}

static void InitCrc32(CRC_Type *base, uint32_t seed) {
    crc_config_t config;

    config.polynomial         = 0x04C11DB7U;
    config.seed               = seed;
    config.reflectIn          = true;
    config.reflectOut         = true;
    config.complementChecksum = true;
    config.crcBits            = kCrcBits32;
    config.crcResult          = kCrcFinalChecksum;

    CRC_Init(base, &config);
}

uint32_t calculateCRC32(uint8_t* toCalculate, size_t length) {
    CRC_Type *base = CRC0;
    uint32_t crc32 = 0;

    InitCrc32(base, 0xFFFFFFFFU);
    CRC_WriteData(base, (uint8_t *)&toCalculate[0], length);
    crc32 = CRC_Get32bitResult(base);

    return crc32;
}
#endif