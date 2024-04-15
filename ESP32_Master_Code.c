/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <esp_system.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "NRF24L01.h"

#define TX_MODE 1
#define RX_MODE 0

// Define the transmission pipe address
uint64_t SlaveAddrs = 0x11223344AA;

// Define an array to store transmission data with an initial value "SEND"
char myTxData[32] = "SEND To My Data";

// Define an array to store received data
char myRxData[32];

// Define an acknowledgment payload with an initial value "Ack by Master !!"
char myAckPayload[32] = "Ack by ESP32S3 !!";

// Define an array to store acknowledgment payload
char AckPayload[32];

// Counter for number of data recieved
int DataCount = 0;

// count waiting time
int waitCount = 0;

// Flag used to indicate errors in communication
int C_ERROR_COUNT = 0;
bool errorFlag = true;

/**
 * @brief nrf24_config_mode - Configures the NRF24 module for transmit or receive mode.
 *
 * This function takes a boolean parameter `transmit_mode` to determine the desired mode
 * for the NRF24 radio module. It configures the module based on the provided mode:
 *
 *  - **Transmit mode (transmit_mode = true):**
 *      - Stops listening for incoming data (NRF24_stopListening)
 *      - Opens the writing pipe for the specified slave address (NRF24_openWritingPipe)
 *  - **Receive mode (transmit_mode = false):**
 *      - Starts listening for incoming data on the NRF24 module (NRF24_startListening)
 *      - Opens the reading pipe number 1 for the specified slave address (NRF24_openReadingPipe)
 *
 * Additionally, the function applies common settings regardless of the mode:
 *  - Enables automatic acknowledgments (NRF24_set_AutoAck)
 *  - Sets the communication channel to 52 (NRF24_set_Channel)
 *  - Sets the payload size to 32 bytes (NRF24_set_PayloadSize)
 *  - Enables dynamic payloads (NRF24_enableDynamicPayloads)
 *  - Enables acknowledgement payloads (NRF24_enable_AckPayload)
 *
 * These common settings ensure proper data transmission and reception with acknowledgments.
 *
 * @param transmit_mode: Boolean flag indicating the desired mode (true for transmit, false for receive).
 */
void nrf24_config_mode(bool transmit_mode)
{
    if (transmit_mode)
    {
        // Transmit mode settings
        NRF24_stopListening();
        NRF24_openWritingPipe(SlaveAddrs);
    }
    else
    {
        // Receive mode settings
        NRF24_startListening();
        NRF24_openReadingPipe(1, SlaveAddrs);
    }

    // Common settings for both modes
    NRF24_set_AutoAck(true);
    NRF24_set_Channel(52);
    NRF24_set_PayloadSize(32);
    NRF24_enableDynamicPayloads();
    NRF24_enable_AckPayload();
}

/*
 * Function: ShootTheMessage (with courtesy!)
 * Description: Blasts a message through the NRF24 radio and waits for a thumbs up (acknowledgment) from the receiver (Node 1).
 * Returns: true if the message was delivered successfully, false otherwise.
 */
bool ShootTheMessage(void)
{
    // Flag to track message delivery success
    bool messageDelivered = false;

    // Load the message into the NRF24's launch bay (call to NRF24_write)
    if (NRF24_write(myTxData, 32))
    {
        // Message launched successfully! Now, wait for a thumbs up from the receiver.
        NRF24_read(AckPayload, 32);
        // printf("Received thumbs up: %s \n\r", AckPayload);
        messageDelivered = true;
    }

    return messageDelivered;
}

/*
 * Function: ReceiveAndAcknowledgeData
 * Description: Listens for incoming data on the NRF24 module, reads it if available,
 *              and sends an acknowledgement payload to Node 1 (data pipe 1).
 * Parameters: None
 * Returns: bool - Indicates whether data was received successfully or not
 */
bool ReceiveAndAcknowledgeData(void)
{
    // Flag to track data reception success
    bool dataReceived = false;

    // Check if data is available on the NRF24 module
    if (NRF24_available())
    {
        // Read the received data into the buffer
        NRF24_read(myRxData, 32);

        // Immediately send an acknowledgement payload on pipe 1
        NRF24_write_AckPayload(1, myAckPayload, 32);

        // Increment received data counter
        DataCount++;

        dataReceived = true;
    }
    return dataReceived;
}

/**
 * @brief AnnounceAndRestartIfNecessary - Monitors for connection errors and triggers a restart if necessary.
 *
 * This function keeps track of connection errors (`errorFlag`) and the number of times this function
 * has been called within a specific wait period (`waitCount`). It checks if the wait limit has been reached
 * (typically after 5 calls) and if there's an error flag set. If both conditions are met, it increments a
 * cumulative error counter (`C_ERROR_COUNT`, assumed to be defined elsewhere).
 *
 * The function then resets the `errorFlag` for the next wait cycle (assuming a typo and meant to be reset to false).
 * If the cumulative error count reaches a threshold (e.g., 10), it announces a connection error with a playful message
 * and initiates a dramatic countdown before restarting the system using `esp_restart()`.
 *
 * If the wait limit hasn't been reached, the function resets the cumulative error counter to zero.
 *
 * @param errorFlag: Boolean flag indicating a connection error.
 * @param waitCount: Number of times this function has been called within a wait period.
 */
void AnnounceAndRestartIfNecessary(bool errorFlag, int waitCount)
{
    // Track cumulative errors if we've reached the wait limit
    if (waitCount == 5)
    {
        if (errorFlag)
        {
            C_ERROR_COUNT++; // Increment error counter (assuming C_ERROR_COUNT is defined elsewhere)
        }

        // Reset error flag for next wait cycle
        errorFlag = false; // Assuming typo here, reset to false

        if (C_ERROR_COUNT >= 10)
        {
            printf("Uh oh! We're lost in connection space!  Reconnecting...\n\r");
            // Countdown with a dramatic flair!
            for (int i = 5; i >= 0; i--)
            {
                printf("Warp drive initiating in %d seconds...\n\r", i);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            printf("Engaging warp drive...\n\r"); // Or "Initiating system reboot..."
            fflush(stdout);
            esp_restart();
        }
    }
    else
    {
        // Reset error counter if we're within the wait limit
        C_ERROR_COUNT = 0;
    }
}

/**
 * @brief Main loop that handles data reception and transmission.
 */
void loop(void)
{
    // Attempt to transmit a message
    if (ShootTheMessage())
    {
        printf("Message sent successfully! Preparing to receive response...\n\r");

        // Switch NRF24 to receive mode and listen for acknowledgement
        nrf24_config_mode(RX_MODE);

        // Track the number of waiting attempts
        waitCount = 0;

        // Loop for a limited time waiting for acknowledgement
        while (waitCount < 5)
        {
            waitCount++;

            // Check if data is received successfully
            if (ReceiveAndAcknowledgeData())
            {
                // Data received! Print it for debugging (optional)
                printf("Received data: %s (Attempt %d)\n\r", myRxData, waitCount);

                // Switch NRF24 back to transmit mode
                nrf24_config_mode(TX_MODE);

                // Reset connection error flag
                errorFlag = false;

                // Short delay before next transmission
                vTaskDelay(200 / portTICK_PERIOD_MS);

                // Exit the inner loop (acknowledgement received)
                break;
            }
            else
            {
                // Acknowledgement not received yet, but haven't reached limit
                printf("Waiting for acknowledgement... (Attempt %d)\n\r", waitCount);

                // Short delay before retrying
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }

            // Reached the wait limit without acknowledgement
            if (waitCount == 5)
            {
                errorFlag = true;
                printf("**Communication error! No acknowledgement received.**\n\r");
            }
        }
    }
    else
    {
        // Message transmission failed
        printf("Message launch unsuccessful! Retrying...\n\r");

        // Set connection error flag
        errorFlag = true;
        waitCount == 5;

        // Short delay before retrying
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    // Print debug information (optional)
    printf("DataCount = %d | Wait_Count = %d | C_ERROR_COUNT = %d | C_ERROR_FLAG = %d  \n\r", DataCount, waitCount, C_ERROR_COUNT, errorFlag);
    printf("//*_______________________________________________________*//\n\r");
    // Handle potential connection errors after the waiting loop
    AnnounceAndRestartIfNecessary(errorFlag, waitCount);
}

/**
 * @brief Initializes the NRF24 module, enters transmit mode, and prints radio settings.
 */
void setup(void)
{

    // Initialize the NRF24 radio module
    NRF24_Init();

    // Announce the start with a dramatic flair (optional)
    printf("                          //**** TRANSMIT - ACK ****//                     \n\r");
    printf("________________________Engaging communication channels...________________________ \n\r");

    // Configure NRF24 for transmit mode with acknowledgment
    nrf24_config_mode(TX_MODE);

    // Print current radio settings (optional)
    printRadioSettings();
}





/**
 * @brief Main application entry point
 *
 * This function initializes the NRF24 radio module, transmits a message,
 * attempts to receive an acknowledgement, and handles potential errors.
 * It enters an infinite loop, continuously trying to transmit and receive data.
 */
void app_main(void)
{

    setup();

    // Loop indefinitely for communication
    while (1)
    {
        // Transmit & Receive data, handle any errors that occur during transmission or reception
        loop();
    }
}
