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
#include "fsl_common.h"  

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

    // Test the ProtocolLayer_send function
    test_ProtocolLayer_send();

    while (1)
    {
        // Receive a message
        ProtocolLayer_receive();
    }
}

// Function to test ProtocolLayer_send
void test_ProtocolLayer_send(void)
{
    const char* messages[] = {
        "No todo lo que es oro reluce...",
        "Aún en la oscuridad...",
        "¿Qué es la vida?",
        "No temas a la oscuridad...",
        "Hasta los más pequeños...",
        "No digas que el sol se ha puesto...",
        "El coraje se encuentra...",
        "No todos los tesoros...",
        "Es peligroso...",
        "Un mago nunca llega tarde...",
        "Aún hay esperanza...",
        "El mundo está cambiando...",
        "Las raíces profundas...",
        "No se puede...",
        "Y sobre todo...",
        "De las cenizas, un fuego..."
    };

    size_t num_messages = sizeof(messages) / sizeof(messages[0]);

    for (size_t i = 0; i < num_messages; i++) {
        PRINTF("Sending test message: %s\r\n", messages[i]);
        ProtocolLayer_send((const uint8_t*)messages[i], strlen(messages[i]));
        SDK_DelayAtLeastUs(2000000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);  // Wait 2 seconds between each message
        ProtocolLayer_receive();
        SDK_DelayAtLeastUs(2000000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    }
}
