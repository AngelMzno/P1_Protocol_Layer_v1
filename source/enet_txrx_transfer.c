/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "fsl_silicon_id.h"
#include "protocol_layer.h"
#include "board.h"
#include "app.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
int main(void)
{
    /* Hardware Initialization. */
    BOARD_InitHardware();
    ProtocolLayer_init();

    /* Build broadcast for sending. */
    ENET_BuildBroadCastFrame();

    while (1)
    {
#if EXAMPLE_USES_LOOPBACK_CABLE
        /* PHY link status update. */
#if (defined(EXAMPLE_PHY_LINK_INTR_SUPPORT) && (EXAMPLE_PHY_LINK_INTR_SUPPORT))
        if (linkChange)
        {
            linkChange = false;
            PHY_ClearInterrupt(&phyHandle);
            PHY_GetLinkStatus(&phyHandle, &link);
            GPIO_EnableLinkIntr();
        }
#else
        PHY_GetLinkStatus(&phyHandle, &link);
#endif
        if (tempLink != link)
        {
            PRINTF("PHY link changed, link status = %u\r\n", link);
            tempLink = link;
        }
#endif /*EXAMPLE_USES_LOOPBACK_CABLE*/
        /* Get the Frame size */
        status = ENET_GetRxFrameSize(&g_handle, &length, 0);
        /* Call ENET_ReadFrame when there is a received frame. */
        if (length != 0)
        {
            /* Received valid frame. Deliver the rx buffer with the size equal to length. */
            uint8_t *data = (uint8_t *)malloc(length);
            status        = ENET_ReadFrame(EXAMPLE_ENET, &g_handle, data, length, 0, NULL);
            if (status == kStatus_Success)
            {
                PRINTF(" A frame received. the length %d ", length);
                PRINTF(" Dest Address %02x:%02x:%02x:%02x:%02x:%02x Src Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",
                       data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9],
                       data[10], data[11]);
            }
            free(data);
        }
        else if (status == kStatus_ENET_RxFrameError)
        {
            /* Update the received buffer when error happened. */
            /* Get the error information of the received g_frame. */
            ENET_GetRxErrBeforeReadFrame(&g_handle, &eErrStatic, 0);
            /* update the receive buffer. */
            ENET_ReadFrame(EXAMPLE_ENET, &g_handle, NULL, 0, 0, NULL);
        }

        if (testTxNum < ENET_TRANSMIT_DATA_NUM)
        {
            /* Send a multicast frame when the PHY is link up. */
#if EXAMPLE_USES_LOOPBACK_CABLE
            if (link)
#endif
            {
                testTxNum++;
                if (kStatus_Success ==
                    ENET_SendFrame(EXAMPLE_ENET, &g_handle, &g_frame[0], ENET_DATA_LENGTH, 0, false, NULL))
                {
                    PRINTF("The %d frame transmitted success!\r\n", testTxNum);
                }
                else
                {
                    PRINTF(" \r\nTransmit frame failed!\r\n");
                }
            }
        }
    }
}
