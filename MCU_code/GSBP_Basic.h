/*
 * # GeneralSerialByteProtocol -> MCU Basic Definitions #
 *   a communication protocol and software module for microcontroller,
 *   suitable for various hardware interfaces (UART, USB, BT, ...)
 *
 *   Copyright (C) 2015-2020 Markus Valtin <os@markus.valtin.net>
 *
 *   Author:  Markus Valtin
 *   File:    GSBP_Basic.h -> header file with general definitions and API definition
 *   Version: 2 (09.2020)
 *
 *   This file is part of GeneralSerialByteProtocol (GSBP).
 *
 *   GSBP is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GSBP is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Copyright Header.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef APP_GSBP_BASIC_H_
#define APP_GSBP_BASIC_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "stdbool.h"
#include "GSBP_Basic_Config.h"
#ifdef APP_GSBP_BASIC_CONFIG_H_

// Version
#define GSBP_VERSION_MAJOR						2
#define GSBP_VERSION_MINOR						0
// Sizes
#define GSBP_RX_TEMP_BUFFER_OVERSIZE            30
#define GSBP_RX_TEMP_BUFFER_SIZE                (GSBP_SETUP__RX_BUFFER_SIZE+GSBP_RX_TEMP_BUFFER_OVERSIZE)

// Commands
#define GSBP__PACKAGE_START_BYTE                0x7E
#define GSBP__PACKAGE_START_BYTE_SIZE           1
#define GSBP__PACKAGE_END_BYTE                  0x81
#define GSBP__PACKAGE_END_BYTE_SIZE             1
#define GSBP__PACKAGE_HEADER_CHECKSUM_START     GSBP__PACKAGE_START_BYTE
#if (GSBP_SETUP__USE_DESTINATION)
 #define GSBP__PACKAGE_N_BYTES_DESTINATION 		1
#else
 #define GSBP__PACKAGE_N_BYTES_DESTINATION 		0
#endif
#if (GSBP_SETUP__USE_CHECKSUMMES)
 #define GSBP__PACKAGE_N_BYTES_HEADER_CHECKSUM  1
 #define GSBP__PACKAGE_N_BYTES_DATA_CHECKSUM    4
#else
 #define GSBP__PACKAGE_N_BYTES_HEADER_CHECKSUM  0
 #define GSBP__PACKAGE_N_BYTES_DATA_CHECKSUM    0
#endif

#define GSBP__PACKAGE_SIZE_HEADER_FIXED_PART    (GSBP__PACKAGE_START_BYTE_SIZE +1) // start byte + package counter
#define GSBP__PACKAGE_SIZE_HEADER				(GSBP__PACKAGE_SIZE_HEADER_FIXED_PART + GSBP__PACKAGE_N_BYTES_DESTINATION + GSBP_SETUP__N_BYTES_CMD + GSBP_SETUP__N_BYTES_DATA_SIZE + GSBP__PACKAGE_N_BYTES_HEADER_CHECKSUM)
#define GSBP__PACKAGE_SIZE_MIN					(GSBP__PACKAGE_SIZE_HEADER + 1) // + stop byte
#define GSBP__PACKAGE_SIZE_TAIL            		(GSBP__PACKAGE_N_BYTES_DATA_CHECKSUM + 1)  // x + stop byte
#define GSBP__PACKAGE_OVERHEAD					(GSBP__PACKAGE_SIZE_HEADER + GSBP__PACKAGE_SIZE_TAIL)
#define GSBP__REQUEST_ID_NONE    				0
#define GSBP__REQUEST_ID_MEASUREMENT_DATA    	255

#define GSBP__UART_TX_TIMEOUT					10

// STM32 HAL Extensions for UART
#define HAL_UART_STATE__TX_ACTIVE				0x01U
#define HAL_UART_STATE__RX_ACTIVE				0x02U
#define HAL_UART_STATE__TX_RX_ACTIVE			0x03U


#ifndef APP_MCU_ERRORHANDLER_H_
	// empty defines, so the debug code can stay inside of the code
	#define MCU_ErrorHandler(X)
	typedef enum {
		EH_GSBP_InitError,
		EH_GSBP_HandleError,
	} gsbp_MCUErrorStates_t;

#else
	#include "MCU_ErrorHandler.h"
#endif


typedef enum {
	GSBP_InterfaceUSB,
	GSBP_InterfaceUART
} gsbp_InterfaceType_t;

typedef enum {
	GSBP_HandleState__HandleDisabled		= 0b00000001,
	GSBP_HandleState__HandleEnabled			= 0b00000010,

	GSBP_HandleState__ReceiveError			= 0b00001000,
	GSBP_HandleState__USBResetBuffer		= 0b00010000,
	GSBP_HandleState__WaitForData			= 0b00100000,

	GSBP_HandleState__DefaultHandle			= 0b10000000,
} gsbp_HandleState_t;

typedef struct {
	volatile uint8_t   		State;
	#ifdef APP_GSBP_MANAGEMENT_CONFIG_H_
		gsbp_Interface_t    	Interface;
	#endif
	uint8_t					HandleListIndex;
	gsbp_InterfaceType_t 	InterfaceType;
	UART_HandleTypeDef 		*UART_Handle; /** Pointer to the UART/BT/USB handle */

	// UART Receive Buffer
	uint8_t 			LastRxRequestID;
	uint8_t 			RxBuffer[GSBP_SETUP__RX_BUFFER_SIZE];
	int16_t				RxBufferSize;			// used by USB to track the buffer size
	uint16_t 			RxBufferIndex1;			// used by saveBuffer UART DMA to track the new memory portion
	uint16_t 			RxBufferIndex2;			// used by saveBuffer UART DMA to track the new memory portion
	uint8_t 			RxTempBuffer[GSBP_RX_TEMP_BUFFER_SIZE];
	uint16_t 			RxTempSize;				// used the function saveBuffer and buildPackage to track the size of the TempBuffer

	// UART Transmit Buffer
	uint8_t 			TxBuffer[GSBP_SETUP__TX_BUFFER_SIZE];
	uint16_t 			TxBufferSize;
} GSBP_Handle_t;

typedef struct __packed {
	#if (GSBP_SETUP__N_BYTES_CMD ==1)
    	uint8_t    CommandID;              	// CMD/ACK ID; MANDATORY
	#elif (GSBP_SETUP__N_BYTES_CMD == 2)
    	uint16_t    CommandID;              // CMD/ACK ID; MANDATORY
	#else
    	error("CMD must be 1 or 2 bytes!");
	#endif
	#if GSBP_SETUP__USE_DESTINATION
    	uint8_t     Destination;         	// package destination
	#endif
    uint8_t     RequestID;         			// raw request id (8 bit); info
	#if (GSBP_SETUP__N_BYTES_DATA_SIZE ==1)
    	uint8_t    DataSize;                // user payload size; MANDATORY
	#elif (GSBP_SETUP__N_BYTES_DATA_SIZE == 2)
    	uint16_t    DataSize;               // user payload size; MANDATORY
	#else
    	error("DATA SIZE must be 1 or 2 bytes!");
	#endif
	#if GSBP_SETUP__USE_CHECKSUMMES
    	uint8_t     ChecksumHeader;         // header checksum; info
	#endif
    uint8_t     Data[GSBP_SETUP__MAX_PAYLOAD_SIZE_RX]; // user payload; MANDATORY
	#if GSBP_SETUP__USE_CHECKSUMMES
    	uint32_t    ChecksumData;			// data checksum; info
	#endif
    	GSBP_Handle_t *HandleOfThisCMD;
} gsbp_PackageRX_t;

typedef struct __packed {
	uint8_t    StartByte;					// StartByte; MANDATORY
	#if (GSBP_SETUP__N_BYTES_CMD ==1)
    	uint8_t    CommandID;              	// CMD/ACK ID; MANDATORY
	#elif (GSBP_SETUP__N_BYTES_CMD == 2)
    	uint16_t    CommandID;              // CMD/ACK ID; MANDATORY
	#else
    	error("CMD must be 1 or 2 bytes!");
	#endif
	#if GSBP_SETUP__USE_DESTINATION
    	uint8_t     Destination;         	// package destination
	#endif
   	uint8_t    RequestID;         			// raw request id (8 bit); info
	#if (GSBP_SETUP__N_BYTES_DATA_SIZE ==1)
    	uint8_t    DataSize;                // user payload size; MANDATORY
	#elif (GSBP_SETUP__N_BYTES_DATA_SIZE == 2)
    	uint16_t    DataSize;               // user payload size; MANDATORY
	#else
    	error("DATA SIZE must be 1 or 2 bytes!");
	#endif
	#if GSBP_SETUP__USE_CHECKSUMMES
    	uint8_t     ChecksumHeader;         // header checksum; info
	#endif
    uint8_t     Data[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX]; // user payload; MANDATORY
	#if GSBP_SETUP__USE_CHECKSUMMES
    	uint32_t    ChecksumData;			// data checksum; info
	#endif
    uint8_t     Tail[GSBP__PACKAGE_SIZE_TAIL];
} gsbp_PackageTX_t;


typedef struct {
	uint32_t			NextCallbackTimer;
	uint32_t			SendTimer;
	#ifdef APP_GSBP_MANAGEMENT_CONFIG_H_
		// Management Variables
		volatile uint8_t 	DoEnableInterface;
		volatile uint8_t 	DoDisableInterface;
		volatile uint8_t 	InterfacesEnabled;
	#endif

	GSBP_Handle_t		*Handles[GSBP_SETUP__NUMBER_OF_HANDLES +1];
	GSBP_Handle_t		*DefaultHandle;
	uint8_t				nActiveHandles;

	gsbp_PackageRX_t	CMD;
	gsbp_PackageTX_t	ACK;

#if GSBP_SETUP__DEBUG_LEVEL >= 1
	#ifdef GSBP_DEBUG_UART
		char debMSG[100];
	#endif
#endif
} GSBP_HandlesList_t;


/*
 * ### GSBP general global variables ###
 */
extern GSBP_HandlesList_t gCOM;


/*
 * ### GSBP "public" API definitions ###
 *
 */

#define  GSBP__COMMAND_SIZE_NODEINFO		18
typedef struct __packed {
	uint64_t boardID;
	uint16_t deviceClass;
	uint32_t serialNumber;
	uint8_t  versionProtocol[2];
	uint8_t  versionFirmware[2];
	uint8_t  msg[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -GSBP__COMMAND_SIZE_NODEINFO];
} gsbp_ACK_nodeInfo_t;

#ifndef GSBP__COMMAND_SIZE_STATE
	//#define  GSBP__COMMAND_SIZE_STATE			3
	typedef struct __packed {
		uint16_t errorCode;
		uint8_t  state;
		uint8_t  msg[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -GSBP__COMMAND_SIZE_STATE];
	} gsbp_ACK_status_t;
#endif

typedef struct __packed {
	uint16_t cmd;
	uint8_t  success;
} gsbp_ACK_universalACK_t;


typedef enum {
	MsgInvalid			= 0,
	MsgCriticalError 	= 1,
	MsgError 			= 2,
	MsgWarning 			= 3,
	MsgInfo 			= 4,
	MsgDebug 			= 5
} gsbp_MsgTypes_t;

#define  GSBP__COMMAND_SIZE_MSG			4
typedef struct __packed {
	uint8_t  msgType;
	uint8_t  state;
	uint16_t errorCode;
	uint8_t  msg[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -GSBP__COMMAND_SIZE_MSG];
} gsbp_ACK_messageACK_t;

/*
 * ### GSBP "public" API functions ###
 *
 * general functions
 */
void    GSBP_InitHandle(GSBP_Handle_t *Handle, UART_HandleTypeDef *huart);
void 	GSBP_DeInitHandle(GSBP_Handle_t *Handle);
bool	GSBP_SetDefaultHandle(GSBP_Handle_t *Handle);

/*
 * ### GSBP "public" API functions ###
 *
 * functions for receiving packages
 */
void    GSBP_ClearBuffer(GSBP_Handle_t *Handle);
uint8_t GSBP_SaveBuffer(GSBP_Handle_t *Handle);
bool 	GSBP_BuildPackageAll(GSBP_Handle_t *Handle, gsbp_PackageRX_t *Package, bool CopyData);
static inline bool GSBP_BuildPackage(GSBP_Handle_t *Handle, gsbp_PackageRX_t *Package){
	return GSBP_BuildPackageAll(Handle, Package, true);
}

bool GSBP_CheckForPackagesAndEvaluateThem(GSBP_Handle_t *Handle, uint8_t *PackagesEvaluedCounterToIncrease);
uint8_t GSBP_CheckAndEvaluatePackages(void);

/*
 * ### GSBP "public" API functions ###
 *
 * functions for sending packages
 */
bool 	GSBP_SendPackageAll(GSBP_Handle_t *Handle, gsbp_PackageTX_t *Package, bool CopyData);
static inline bool GSBP_SendPackage(GSBP_Handle_t *Handle, gsbp_PackageTX_t *Package){
	return GSBP_SendPackageAll(Handle, Package, true);
}

void 	GSBP_SendUniversalACK(GSBP_Handle_t *Handle, uint16_t ResponceToCMD, bool Success);
void 	GSBP_SendUniversalACKext(GSBP_Handle_t *Handle, uint16_t ResponceToCMD, uint8_t RequestID, bool Success);
void 	GSBP_SendMSG(GSBP_Handle_t *Handle, gsbp_MsgTypes_t Type, uint8_t State, uint16_t ErrorCode, const char* Message, ...);

void 	GSBP_SendNodeInfo(GSBP_Handle_t *Handle, uint8_t RequestID);
void    GSBP_SendStatus(GSBP_Handle_t *Handle, uint8_t RequestID);

//bool 	GSBP_ReSendPackage(GSBP_Handle_t *Handle);
//bool 	GSBP_ReSendPackages(void);

void GSBP_BuildPackageCallback(GSBP_Handle_t *Handle);

/*
 * ### GSBP "public" API functions ###
 *
 * functions for debugging
 */

void gsbpDebugMSG(uint8_t level, const char* message, ...);

/*
 * ### GSBP "private" functions ###
 *
 * helper functions for GSBP
 */
uint8_t GSBP_GetRequestID(GSBP_Handle_t *Handle, uint8_t RequestIDToUse);
uint8_t GSBP_GetHeaderChecksum(uint8_t *Buffer);
uint8_t GSBP_GetDataChecksum(uint8_t *Buffer, uint8_t length); // TODO: richtig machen ....

#ifdef __cplusplus
 }
#endif

#endif /* APP_GSBP_BASIC_CONFIG_H_ */
#endif /* APP_GSBP_BASIC_H_ */
