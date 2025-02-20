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

    // Send a custom message
    const char* message = "Test number one :D";
    ProtocolLayer_send((const uint8_t*)message, strlen(message));

    while (1)
    {
        // Receive a message
        ProtocolLayer_receive();

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
