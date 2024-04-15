
#include "MY_NRF24.h"
#include "stdio.h"
#include <string.h>


// Define the transmission pipe address for the slave device
uint64_t pipeAddr = 0x11223344AA;

// Define an array to store transmission data with an initial value "SEND"
char myTxData[32] = "Hello World 1 !!";

// Define an array to store acknowledgment payload
char AckPayload[32];

// Define an array to store received data
char myRxData[32];

// Define an acknowledgment payload with an initial value "Ack by Master !!"
char myAckPayload[32] = "Ack by Node 1";

int count = 0;

// Function to configure NRF24 module for transmit mode without acknowledgment
void Tx_Mode(void) {
	// Print information about entering transmit mode without acknowledgment
	//printf("________________________Tx Mode________________________ \n\r");

	// Stop listening for incoming data
	NRF24_stopListening();

	// Set writing pipe address to TxpipeAddrs
	NRF24_openWritingPipe(pipeAddr);

	// Enable auto acknowledgment
	NRF24_setAutoAck(true);

	// Set channel to 52
	NRF24_setChannel(52);

	// Set payload size to 32 bytes
	NRF24_setPayloadSize(32);

	// Enable dynamic payloads
	NRF24_enableDynamicPayloads();

	// Enable acknowledgment payloads
	NRF24_enableAckPayload();

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, RESET);
}

/*
 * Function: Send_Data
 * Description: Sends data using NRF24 module and waits for acknowledgment from Node 1.
 * Parameters: None
 * Returns: bool - Indicates whether data was sent successfully or not
 */
bool Send_Data(void) {
	// Variable to track the status of data transmission
	bool send_stat = false;

	// Attempt to write data to NRF24 module
	if (NRF24_write(myTxData, 32)) {
		// If data is successfully written, read acknowledgment payload
		NRF24_read(AckPayload, 32);
		printf("%s \r\n", AckPayload);
		send_stat = true;
	}

	// Return the status of data transmission
	return send_stat;
}

// Function to configure NRF24 module for receiving mode without acknowledgment
void Rx_Mode(void) {
	// Print information about changing settings to receiver mode with acknowledgment
	//printf("________________________Rx Mode________________________ \n\r");

	// Enable auto acknowledgment
	NRF24_setAutoAck(true);

	// Set channel to 52
	NRF24_setChannel(52);

	// Set payload size to 32 bytes
	NRF24_setPayloadSize(32);

	// Open reading pipe with address RxpipeAddrs
	NRF24_openReadingPipe(1, pipeAddr);

	// Enable dynamic payloads
	NRF24_enableDynamicPayloads();

	// Enable acknowledgment payloads
	NRF24_enableAckPayload();

	// Start listening for incoming data
	NRF24_startListening();

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, SET);
}

/*
 * Function: Receive_Data
 * Description: Receives data from Node 1 using NRF24 module.
 * Parameters: None
 * Returns: bool - Indicates whether data was received successfully or not
 */
bool Receive_Data(void) {
	// Variable to track the status of data reception
	bool receive_stat = false;

	// Check if there is data available to read
	if (NRF24_available()) {
		// Read data from NRF24 module
		NRF24_read(myRxData, 32);

		// Send acknowledgment payload to Node 1
		NRF24_writeAckPayload(1, myAckPayload, 32);

		// Print the received data
		count++;
		printf("%s : %d \r\n", myRxData, count);

		// Set receive_stat to true to indicate successful data reception
		receive_stat = true;
	}
	// Return the status of data reception
	return receive_stat;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 2 */
	/*
	 * Description: This block of code initializes the NRF24 module, enters receive mode,
	 *              and prints current radio settings.
	 */

	// Initialize NRF24 module
	NRF24_Init();

	// Print information about entering receive mode with acknowledgment
	printf("//**** RECEIVE - ACK ****//   \n\r");
	printf(
			"________________________After change Setting________________________ \n\r");

	// Switch NRF24 module to receive mode
	Rx_Mode();

	// Print current radio settings
	printRadioSettings();

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/*
		 * Description: This block of code manages the data reception and transmission process.
		 *              It checks if data is received using the Receive_Data function.
		 *              If data is received successfully, it switches to transmit mode,
		 *              sends the acknowledgment, and switches back to receive mode.
		 *              If no data is received, it prints a message indicating no request from the master.
		 */

		// Check if data is received successfully
		if (Receive_Data()) {
			// Switch NRF24 module to transmit mode

			Tx_Mode();
			
			// Loop indefinitely until acknowledgment is sent or a timeout occurs
			int coun = 0;
			while (coun < 5) {
				// Check if acknowledgment is sent successfully
				if (Send_Data()) {
					// Switch NRF24 module back to receive mode
					Rx_Mode();

					// Delay for 100 milliseconds before breaking out of the loop
					HAL_Delay(200);

					// Exit the loop
					break;

				} else if (coun == 5) {
					// Switch NRF24 module back to receive mode
					Rx_Mode();
				}

				// If acknowledgment transmission fails, wait for a short period before retrying
				HAL_Delay(200);

				coun++;

			}

			Rx_Mode();

		} else {
			// Delay for 100 milliseconds before continuing
			HAL_Delay(200);
		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}
