/*
 * # GeneralSerialByteProtocol -> MCU Basic API Implementation #
 *   a communication protocol and software module for microcontroller,
 *   suitable for various hardware interfaces (UART, USB, BT, ...)
 *
 *   Copyright (C) 2015-2020 Markus Valtin <os@markus.valtin.net>
 *
 *   Author:  Markus Valtin
 *   File:    GSBP_Basic.h -> source file with general API implementation
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

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "GSBP_Basic_Config.h"
#include "GSBP_Basic.h"


#if (GSBP_SETUP__INTERFACE_UART_USED)
	#include "usart.h"
#endif
#if (GSBP_SETUP__INTERFACE_USB_USED)
	#include "usbd_cdc_if.h"
	extern USBD_HandleTypeDef hUsbDeviceFS;
#endif


/***
 * ### GSBP general global variables ###
 ***/
GSBP_HandlesList_t gCOM;



/***
 * ### GSBP "public" API functions ###
 *
 * general functions
 ***/

// Initialize a handle variable and start the interface
void GSBP_InitHandle(GSBP_Handle_t *Handle, UART_HandleTypeDef *huart)
{
	// initial checks
	if (Handle->State  & GSBP_HandleState__HandleEnabled){
		// handle already active -> quit
		return;
	}
	if (gCOM.nActiveHandles  >= GSBP_SETUP__NUMBER_OF_HANDLES){
		// all handles are already active and assigned -> quit / error?
		return;
	}

	// add handle to the GSBP handle list
	Handle->HandleListIndex = gCOM.nActiveHandles;
	gCOM.Handles[gCOM.nActiveHandles] = Handle;
	gCOM.nActiveHandles++;

	// update the state
	Handle->State |=  GSBP_HandleState__HandleEnabled;
	Handle->State &= ~GSBP_HandleState__HandleDisabled;

#if (GSBP_SETUP__INTERFACE_UART_USED)
	// UART interface
	if (Handle->InterfaceType == GSBP_InterfaceUART){
		// set the UART handle pointer
		Handle->UART_Handle = huart;
		// start Receiving
		#if (GSBP_SETUP__UART_RX_METHOD == 0)	// POLLING
			// noting to do here
		#elif (GSBP_SETUP__UART_RX_METHOD == 1)	// IT
		if (HAL_UART_Receive_IT(Handle->UART_Handle, Handle->RxBuffer, GSBP_SETUP__RX_BUFFER_SIZE) != HAL_OK){
			MCU_ErrorHandler(EH_UART_InitError);
		}
		#elif (GSBP_SETUP__UART_RX_METHOD == 2)	// DMA
		if (HAL_UART_Receive_DMA(Handle->UART_Handle, Handle->RxBuffer, GSBP_SETUP__RX_BUFFER_SIZE) != HAL_OK){
			MCU_ErrorHandler(EH_UART_InitError);
		}
		#else
		#error "Unsupported GSBP_SETUP__UART_RX_METHOD"
		#endif
	}
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
	// USB interface
	if (Handle->InterfaceType == GSBP_InterfaceUSB){
	    // set the UART handle pointer
		Handle->UART_Handle = NULL;
	}
#endif

	// handle the default handle
	if (Handle->State & GSBP_HandleState__DefaultHandle){
		GSBP_SetDefaultHandle(Handle);
	}
}

// remove the given handle and make sure the handle list is in order
void GSBP_DeInitHandle(GSBP_Handle_t *Handle)
{
	if (Handle->State & GSBP_HandleState__DefaultHandle){
		gCOM.DefaultHandle = NULL;
	}
	if (gCOM.nActiveHandles > 0) {
		for (uint8_t i=Handle->HandleListIndex; i< gCOM.nActiveHandles; i++){
			if (gCOM.Handles[i]->UART_Handle != NULL){
				// move all handles up in case this handle is not the last
				gCOM.Handles[i] = gCOM.Handles[i+1];
			}
		}
		// set the last handle to NULL even it should already be NULL
		gCOM.Handles[gCOM.nActiveHandles -1] = NULL;
		// count down the active handles
		gCOM.nActiveHandles--;
	}
	// update the state
	Handle->State &= ~GSBP_HandleState__HandleEnabled;
	Handle->State |=  GSBP_HandleState__HandleDisabled;

#if (GSBP_SETUP__INTERFACE_UART_USED)
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
#endif
}

// update the default handle to the given handle
bool GSBP_SetDefaultHandle(GSBP_Handle_t *Handle)
{
	// initial checks
	if (!(Handle->State  & GSBP_HandleState__HandleEnabled)){
		// error: the handle is not active
		return false;
	}

	// search for the current default handle
	if (gCOM.nActiveHandles > 0) {
		for (uint8_t i=0; i< gCOM.nActiveHandles; i++){
			if (gCOM.Handles[i]->State & GSBP_HandleState__DefaultHandle){
				gCOM.Handles[i]->State &= ~GSBP_HandleState__DefaultHandle;
			}
		}
	} else {
		// error: there should be active handles
		return false;
	}

	gCOM.DefaultHandle = Handle;
	Handle->State |= GSBP_HandleState__DefaultHandle;
	return true;
}



/***
 * ### GSBP "public" API functions ###
 *
 * functions for receiving packages
 ***/

// GSBP receive function for polling -> works probably not reliable
uint16_t GSBP_GetNBytes(GSBP_Handle_t *Handle, uint16_t N_Bytes, uint16_t TimeOut)
{
	memset(Handle->RxBuffer, 0, sizeof(Handle->RxBuffer));
	HAL_UART_Receive(Handle->UART_Handle, Handle->RxBuffer, N_Bytes, (uint32_t)TimeOut);
	memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], Handle->RxBuffer, strlen((char*)Handle->RxBuffer));
	Handle->RxTempSize += strlen((char*)Handle->RxBuffer);
	return Handle->RxTempSize;

	//TODO: check if timeout
}

// clear the RX buffer of that handle
void GSBP_ClearBuffer(GSBP_Handle_t *Handle)
{
    memset(Handle->RxBuffer, 0, GSBP_SETUP__RX_BUFFER_SIZE);
    Handle->RxBufferSize = 0;
    memset(Handle->RxTempBuffer, 0, GSBP_RX_TEMP_BUFFER_SIZE);
    Handle->RxTempSize = 0;
}

// copy the bytes received by that handle to a second buffer for later inspection
uint8_t GSBP_SaveBuffer(GSBP_Handle_t *Handle)
{
	// check the handle
	if (Handle == NULL){
		return 0;
	}
	// run only if the handle is enabled
	if (Handle->State & GSBP_HandleState__HandleDisabled){
		return 0;
	}

	int16_t NumberOfBytesToTransfer = 0;

    // TODO: prüfen, was passiert, wenn daten ankommen, während diese funktion ausgeführt wird  -> HAL_Delay einfügen ...
	// TODO: bei packeten von mehreren Interfaces, muss sichergestellt sien, dass die Packete vollständig sind, da sonnst die Bytereihenfole in RxBufferTemp nicht mehr stimmt....
    #if (GSBP_SETUP__DEBUG_LEVEL >= 4)
		gsbpDebugToggle_D1();
    #endif

#if (GSBP_SETUP__INTERFACE_UART_USED)
    // UART Interface
    if (Handle->InterfaceType == GSBP_InterfaceUART){
#if (GSBP_SETUP__UART_RX_METHOD == 0)	// Polling
        // get the header Bytes from the interface and then the payload
    	// works, but not really reliable
    	uint16_t *sizePayload;
    	// todo: use get_n_byte mit Timeout -> add timeout
    	HAL_StatusTypeDef ret = HAL_UART_Receive(Handle->UART_Handle, &Handle->RxTempBuffer[Handle->RxTempSize], (uint16_t)GSBP__PACKAGE_SIZE_HEADER, (uint32_t)GSBP_SETUP__UART_RX_POLLING_TIMEOUT);
    	if (ret == HAL_OK){
    		sizePayload = Handle->RxTempBuffer[Handle->RxTempSize +3];
    		ret = HAL_UART_Receive(Handle->UART_Handle, &Handle->RxTempBuffer[Handle->RxTempSize +GSBP__PACKAGE_SIZE_HEADER], (*sizePayload +GSBP__PACKAGE_TAIL_SIZE -1), (uint32_t)GSBP_SETUP__UART_RX_POLLING_TIMEOUT);
    		if (ret == HAL_OK){
    			Handle->RxTempSize += *sizePayload +GSBP__PACKAGE_SIZE_HEADER +GSBP__PACKAGE_TAIL_SIZE;
    		}
    	}
#elif (GSBP_SETUP__UART_RX_METHOD == 1)	// IT
    	if (Handle->UART_Handle->RxXferSize != Handle->UART_Handle->RxXferCount){
        	__HAL_LOCK(Handle->UART_Handle);
        	NumberOfBytesToTransfer = Handle->UART_Handle->RxXferSize - Handle->UART_Handle->RxXferCount;
        	// copy N bytes
        	memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], Handle->RxBuffer, NumberOfBytesToTransfer );
        	Handle->RxTempSize += NumberOfBytesToTransfer;
        	// Reset the buffer
    		Handle->UART_Handle->pRxBuffPtr = Handle->RxBuffer;
        	Handle->UART_Handle->RxXferCount = Handle->UART_Handle->RxXferSize;
        	__HAL_UNLOCK(Handle->UART_Handle);
    	}
#elif (GSBP_SETUP__UART_RX_METHOD == 2)	// DMA
#ifdef STM32L432xx
    	if ((Handle->RxBufferIndex2 = Handle->UART_Handle->RxXferSize -Handle->UART_Handle->hdmarx->Instance->CNDTR) != Handle->RxBufferIndex1){
#else
    	if ((Handle->RxBufferIndex2 = Handle->UART_Handle->RxXferSize -Handle->UART_Handle->hdmarx->Instance->NDTR) != Handle->RxBufferIndex1){
#endif
    		// save buffer
    		if (Handle->RxBufferIndex2 < Handle->RxBufferIndex1){
    			// the circular buffer was reseted -> 2 copy operations
    			// copy the data from position x to the end of the buffer
    			NumberOfBytesToTransfer = Handle->UART_Handle->RxXferSize -Handle->RxBufferIndex1;
    			memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], &Handle->RxBuffer[Handle->RxBufferIndex1], NumberOfBytesToTransfer);
    			Handle->RxTempSize += NumberOfBytesToTransfer;
    			// copy the data from the start of the buffer until position y
    			memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], &Handle->RxBuffer[0], Handle->RxBufferIndex2);
    			Handle->RxTempSize += Handle->RxBufferIndex2;
    		} else {
    			// the circular buffer was NOT reseted -> 1 copy operations
    			// copy the data from position x to position y
    			NumberOfBytesToTransfer = Handle->RxBufferIndex2 -Handle->RxBufferIndex1;
    			memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], &Handle->RxBuffer[Handle->RxBufferIndex1], NumberOfBytesToTransfer);
    			Handle->RxTempSize += NumberOfBytesToTransfer;
    		}
    		Handle->RxBufferIndex1 = Handle->RxBufferIndex2;
    		// resets are UNNECESSARY because the DMA has to be configured in circular mode -> so it will go back automatically
    	}
#else
#error "Unsupported GSBP_SETUP__UART_RX_METHOD"
#endif
    }
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
    // USB Interface
    if (Handle->InterfaceType == GSBP_InterfaceUSB) {
    	if (Handle->RxBufferSize != 0){
    		NumberOfBytesToTransfer = Handle->RxBufferSize;
    		if ((Handle->RxTempSize +NumberOfBytesToTransfer) > GSBP_RX_TEMP_BUFFER_SIZE){
    			// error: buffer to small
    			gsbpDebugMSG(1, "GSBP SaveBuffer ERROR: RxTempBuffer to small (%d + %d > %d)\n",
    					Handle->RxTempSize, NumberOfBytesToTransfer, GSBP_RX_TEMP_BUFFER_SIZE);
    			GSBP_SendMSG(Handle, MsgError, GSBP_GetMcuState(), E_BufferToSmall, "GSBP SaveBuffer: RxTempBuffer to small (%d + %d > %d)",
    					Handle->RxTempSize, NumberOfBytesToTransfer, GSBP_RX_TEMP_BUFFER_SIZE);
    			// fill the RX temp buffer
    			NumberOfBytesToTransfer = GSBP_RX_TEMP_BUFFER_SIZE -Handle->RxTempSize;
    		}
    		// copy N bytes
    		memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], Handle->RxBuffer, NumberOfBytesToTransfer );
    		Handle->RxTempSize += NumberOfBytesToTransfer;
    		// Reset the buffer
    		Handle->RxBufferSize = 0;
    		if (Handle->State & GSBP_HandleState__USBResetBuffer){
    			USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &GSBP_USB.RxBuffer[0]);
    			USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    			Handle->State &= ~GSBP_HandleState__USBResetBuffer;
    		}
    	}
    }
#endif

#if (GSBP_SETUP__DEBUG_LEVEL >= 4)
	gsbpDebugToggle_D1();
#endif
	if (Handle->RxTempSize > 0){
		gsbpDebugMSG(6, "GSBP SaveBuffer: +%d->%d: (0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)\n",
			Handle->RxBufferSize, Handle->RxTempSize, Handle->RxTempBuffer[0], Handle->RxTempBuffer[1], Handle->RxTempBuffer[2], Handle->RxTempBuffer[3], Handle->RxTempBuffer[4]);
	}

    return Handle->RxTempSize;
}

// build the package struct from a byte stream saved in the handles temp buffer
bool GSBP_BuildPackageAll(GSBP_Handle_t *Handle, gsbp_PackageRX_t *Package, bool CopyData)
{
#if (GSBP_SETUP__DEBUG_LEVEL >= 4)
		gsbpDebugToggle_D2();
#endif

	// check the handle
	if (Handle == NULL || Package == NULL){
		return false;
	}
	if (Handle->RxTempSize == 0){
		return false;
	}

	uint16_t    RxPackageStartIndex = 0, RxPackageEndIndex = 0;
	GSBP_BuildPackageCallback(Handle);

	// search the start byte
	for (uint16_t i = 0; i < Handle->RxTempSize; i++){
		if (Handle->RxTempBuffer[i] == GSBP__PACKAGE_START_BYTE){
			RxPackageStartIndex = i + 1; // the byte after the start byte is the first header byte
			break;
		}
	}

	if (RxPackageStartIndex == 0){
		// no start byte found
		Handle->RxTempSize = 0;
		return false;
	}

	gsbp_PackageRX_t *temp = (gsbp_PackageRX_t *)&Handle->RxTempBuffer[RxPackageStartIndex];

#if (GSBP_SETUP__USE_CHECKSUMMES)
	// check the header checksum
	if (temp->ChecksumHeader != GSBP_GetHeaderChecksum(&Handle->RxTempBuffer[RxPackageStartIndex]) ){
		// checksum does NOT match
		GSBP_SendMSG(Handle, MsgError, GSBP_GetMcuState(), E_ChecksumMissmatch, "GSBP: HeaderChecksum is %d!=%d for package #%d",
				GSBP_GetHeaderChecksum(&Handle->RxTempBuffer[RxPackageStartIndex]), temp->ChecksumHeader, temp->RequestID);
		Handle->RxTempBuffer[RxPackageStartIndex -1] = 0x00; // clear the GSBP start byte -> masking this package
		return false;
	}
#endif

	// check the sizes
	if ( ((Handle->RxTempSize -RxPackageStartIndex +GSBP__PACKAGE_START_BYTE_SIZE) < GSBP__PACKAGE_SIZE_MIN) ||
	     (temp->DataSize != 0 && ((Handle->RxTempSize -RxPackageStartIndex +GSBP__PACKAGE_START_BYTE_SIZE) < (temp->DataSize +GSBP__PACKAGE_SIZE_MIN +GSBP__PACKAGE_N_BYTES_DATA_CHECKSUM))) ){
		if (Handle->State & GSBP_HandleState__WaitForData){
			// send error about not enough data....
			Handle->State &= ~GSBP_HandleState__WaitForData;
			gsbpDebugMSG(1, "BuildPackage: ERROR not enough data: BS %d; DS %d; SI %d\n", Handle->RxTempSize, temp->DataSize, RxPackageStartIndex);
			GSBP_SendMSG(Handle, MsgError, GSBP_GetMcuState(), E_NoNewData, "BuildPackage: ERROR not enough data: BS %d; DS %d; SI %d\n",
					Handle->RxTempSize, temp->DataSize, RxPackageStartIndex);
			// reset the buffer
			memset(Handle->RxTempBuffer, 0, sizeof(Handle->RxTempBuffer));
			Handle->RxTempSize = 0;
			return false;
		} else {
			// wait until the next call of this function, maybe the data has arrived then
			Handle->State |= GSBP_HandleState__WaitForData;
			return false;
		}
	}

	// TODO Check the request ID ???
	// TODO Checksum data

	// check the END byte
	if (temp->DataSize > 0){
		RxPackageEndIndex = RxPackageStartIndex +GSBP__PACKAGE_SIZE_HEADER -GSBP__PACKAGE_START_BYTE_SIZE +temp->DataSize +GSBP__PACKAGE_N_BYTES_DATA_CHECKSUM;
	} else {
		RxPackageEndIndex = RxPackageStartIndex +GSBP__PACKAGE_SIZE_HEADER -GSBP__PACKAGE_START_BYTE_SIZE;
	}
	if (Handle->RxTempBuffer[RxPackageEndIndex] != GSBP__PACKAGE_END_BYTE) {
		// size does not match -> ERROR
		gsbpDebugMSG(1, "BuildPackage: ERROR end byte is wrong BS %d; DS %d; SI %d -> EB %d ([%d])\n",
				Handle->RxTempSize, temp->DataSize, RxPackageStartIndex, Handle->RxTempBuffer[RxPackageEndIndex], RxPackageEndIndex);
		GSBP_SendMSG(Handle, MsgError, GSBP_GetMcuState(), E_EndByteMissmatch, "GSBP: EndByte is %d!=%d for requestID %d",
				Handle->RxTempBuffer[RxPackageEndIndex], GSBP__PACKAGE_END_BYTE, temp->RequestID);
		// clear the header in the buffer
		memset(&Handle->RxTempBuffer[RxPackageStartIndex -GSBP__PACKAGE_START_BYTE_SIZE], 0, GSBP__PACKAGE_SIZE_HEADER);
		return false;
	}
	RxPackageEndIndex++; // add EndByte

	// all check passed -> the data is valid
	if (CopyData){
		// copy the data into the package
		memcpy(Package, &Handle->RxTempBuffer[RxPackageStartIndex], (size_t)(GSBP__PACKAGE_SIZE_HEADER -GSBP__PACKAGE_START_BYTE_SIZE +temp->DataSize));
	} else {
		// update the package pointer to the handle's RxBuffer
		Package = (gsbp_PackageRX_t *)&Handle->RxTempBuffer[RxPackageStartIndex];
	}
    // update the pointer to the GSBP handle the CMD came from
	Package->HandleOfThisCMD = Handle;

	// reset some counters
	if (RxPackageEndIndex >= Handle->RxTempSize){
		// the buffer holds only this one package -> reset the counter
		if (CopyData){
			Handle->RxTempSize = 0;
		}
	} else {
		// the buffer holds more data -> set the package data to 0
		memset(&Handle->RxTempBuffer[RxPackageStartIndex -GSBP__PACKAGE_START_BYTE_SIZE], 0, (RxPackageEndIndex -RxPackageStartIndex));
		gsbpDebugMSG(6, "BuildPackage: the buffer holds more data -> set this package (%d bytes) to 0\n", (RxPackageEndIndex -RxPackageStartIndex));
	}

	// set the lastRxRequestID
	Handle->LastRxRequestID = Package->RequestID;

	gsbpDebugMSG(6, "BuildPackage: CMD %d; Req# %d; Nbytes %d; SI %d\n", Package->CommandID, Package->RequestID, Package->DataSize, RxPackageStartIndex);

	//TODO better error handling
	// maybe the package was not yet completely received ...
	//Handle->RxTempBuffer[RxPackageStartIndex -1] = 0x00; // clear the GSBP start byte -> masking this package
	//Handle->RxTempSize = 0;

#if (GSBP_SETUP__DEBUG_LEVEL >= 4)
		gsbpDebugToggle_D2();
#endif

	return true;
}


bool GSBP_CheckForPackagesAndEvaluateThem(GSBP_Handle_t *Handle, uint8_t *PackagesEvaluedCounterToIncrease)
{
	// check if new commands were received
	if ( GSBP_SaveBuffer(Handle) >= GSBP__PACKAGE_SIZE_MIN ) {
		// get one package and process it's payload
		while (GSBP_BuildPackage(Handle, &gCOM.CMD)){
			if (!GSBP_EvaluatePackage(&gCOM.CMD, Handle)){
				(*PackagesEvaluedCounterToIncrease)++;
				return false;
			}
			(*PackagesEvaluedCounterToIncrease)++;
		}
	}
	return true;
}

uint8_t GSBP_CheckAndEvaluatePackages(void)
{
	// check all enabled GSBP handles
	uint8_t i = 0, PackagesEvaluated = 0;

	for (i=0; i< gCOM.nActiveHandles; i++){
		if (gCOM.Handles[i]->State & GSBP_HandleState__HandleEnabled){
			if (!GSBP_CheckForPackagesAndEvaluateThem(gCOM.Handles[i], &PackagesEvaluated)){
				return PackagesEvaluated;
			}
		}
	}
	return PackagesEvaluated;
}



/***
 * ### GSBP "public" API functions ###
 *
 * functions for sending packages
 ***/

// Sends in format:
//Start, command, Pack Num, data size high, data size low, checksum, data ... , 0 byte (checksum 2?), End. Total Bytes = DataSize + 8

bool GSBP_SendPackageAll(GSBP_Handle_t *Handle, gsbp_PackageTX_t *Package, bool CopyData)
{
#if (GSBP_SETUP__DEBUG_LEVEL >= 4)
	gsbpDebugToggle_D3();
#endif

	if (Handle == NULL || Package == NULL){
		return false;
	}
	if (Handle->State & GSBP_HandleState__HandleDisabled){
		return false;
	}

	if (Package->DataSize > GSBP_SETUP__MAX_PAYLOAD_SIZE_TX){
		GSBP_SendMSG(Handle, MsgCriticalError, GSBP_GetMcuState(), E_BufferToSmall,
				"data=%d bytes but TxBuffer only %d bytes", Package->DataSize, Handle->TxBufferSize);
		return false;
	}

	// wait until the last  transfer is done before we override the buffer content
#if (GSBP_SETUP__INTERFACE_UART_USED)
	// UART interface
	uint32_t uartTimeout = 0;
	if (Handle->InterfaceType == GSBP_InterfaceUART){
		uartTimeout = HAL_GetTick() +GSBP_SETUP__UART_TX_SEND_TIMEOUT;
		while (Handle->UART_Handle->gState & HAL_UART_STATE__TX_ACTIVE) {  // TODO ERROR, falls RX = DMA !!!! Flag richtig?
			// Timeout after N ms
			if (HAL_GetTick() >= uartTimeout) {
				break;
			}
		}
	}
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
	// USB interface
	if (Handle->InterfaceType == GSBP_InterfaceUSB){
		if (CDC_GSBP_WaitUntilReadyToSend(HAL_GetTick() +GSBP_SETUP__UART_TX_SEND_TIMEOUT)) {
			return false;
		}
	}
#endif

	// Debug the send commands
	gsbpDebugMSG(6, "SendACK: CMD %d (%ld) -> ", Package->CommandID, HAL_GetTick());

	/*
	 * build the TxBuffer
	 */
	gsbp_PackageTX_t *TxPackage = Package;

	if (CopyData){
		if (GSBP__PACKAGE_SIZE_HEADER +Package->DataSize +Handle->TxBufferSize > GSBP_SETUP__TX_BUFFER_SIZE ){
			gsbpDebugMSG(1, "SendACK ERROR: reseting TxBufferSize to 0 from %d", Handle->TxBufferSize);
			Handle->TxBufferSize = 0;
		}
		memcpy(&Handle->TxBuffer[Handle->TxBufferSize], Package, (size_t)(GSBP__PACKAGE_SIZE_HEADER +Package->DataSize));
		TxPackage = (gsbp_PackageTX_t*)&Handle->TxBuffer[Handle->TxBufferSize];
	}
	// TxBuffersize is increased depending on the features
	Handle->TxBufferSize += GSBP__PACKAGE_SIZE_HEADER +Package->DataSize;
	TxPackage->RequestID = GSBP_GetRequestID(Handle, Package->RequestID);

#if (GSBP_SETUP__USE_CHECKSUMMES)
	// generate the header checksum
	TxPackage->ChecksumHeader = GSBP_GetHeaderChecksum(&(TxPackage->CommandID));
	if (TxPackage->DataSize > 0){
		// generate the header checksum and add the data checksum
		uint32_t *ChecksumData = (uint32_t*) &TxPackage->Data[TxPackage->DataSize];
		*ChecksumData = GSBP_GetDataChecksum(TxPackage->Data, TxPackage->DataSize);
		Handle->TxBufferSize += sizeof(uint32_t);
		// add end byte
		TxPackage->Data[TxPackage->DataSize +sizeof(uint32_t)] = GSBP__PACKAGE_END_BYTE;
	} else {
		// add end byte
		TxPackage->Data[0] = GSBP__PACKAGE_END_BYTE;
	}
#else
	// add end byte
	TxPackage->Data[TxPackage->DataSize] = GSBP__PACKAGE_END_BYTE;
#endif

	Handle->TxBufferSize += GSBP__PACKAGE_END_BYTE_SIZE;
	TxPackage->StartByte = GSBP__PACKAGE_START_BYTE;

	gsbpDebugMSG(6, "%d bytes (#%d)\n", Handle->TxBufferSize, TxPackage->RequestID);
#if (GSBP_SETUP__DEBUG_LEVEL >=7)
	gsbpDebugMSG(7, "  -> ");
	for (uint16_t i = 0; i<Handle->TxBufferSize; i++){
		gsbpDebugMSG(7, "0x%02X ", Handle->TxBuffer[i]);
	}
	gsbpDebugMSG(7, "\n");
#endif

	/*
	 * send the command
	 */
#if (GSBP_SETUP__INTERFACE_UART_USED)
	// UART Interface
	if (Handle->InterfaceType == GSBP_InterfaceUART){
#if (GSBP_SETUP__UART_TX_METHOD == 0)	// Polling
		if (HAL_UART_Transmit(Handle->UART_Handle, (uint8_t *) TxPackage, (uint16_t) Handle->TxBufferSize, GSBP__UART_TX_TIMEOUT) != HAL_OK) {
#endif

#if (GSBP_SETUP__UART_TX_METHOD == 1)	// IT
			if (HAL_UART_Transmit_IT(Handle->UART_Handle, (uint8_t *) TxPackage, (uint16_t) Handle->TxBufferSize) != HAL_OK) {
#endif
#if (GSBP_SETUP__UART_TX_METHOD == 2)	// DMA
				if (HAL_UART_Transmit_DMA(Handle->UART_Handle, (uint8_t *) TxPackage, (uint16_t) Handle->TxBufferSize) != HAL_OK) {
#endif
					// buffer NOT send
					return false;
				} else {
					// buffer send -> set the buffer size to zero
					Handle->TxBufferSize = 0;
					return true;
				}
			}
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
	// USB Interface
	if (Handle->InterfaceType == GSBP_InterfaceUSB) {
		if (CDC_Transmit_FS( (uint8_t *) TxPackage, (uint16_t) (Handle->TxBufferSize)) != USBD_OK) {
			return false;
		} else {
			// buffer send -> set the buffer size to zero
			Handle->TxBufferSize = 0;
			return true;
		}
	}
#endif

#if (GSBP_SETUP__DEBUG_LEVEL >= 4)
	gsbpDebugToggle_D3();
#endif

	return false;
}

void GSBP_SendUniversalACK(GSBP_Handle_t *Handle, uint16_t ResponceToCMD, bool Success)
{
	GSBP_SendUniversalACKext(Handle, ResponceToCMD, 0, Success);
}
void GSBP_SendUniversalACKext(GSBP_Handle_t *Handle, uint16_t ResponceToCMD, uint8_t RequestID, bool Success)
{
	gCOM.ACK.CommandID = UniversalACK;
	gCOM.ACK.RequestID = RequestID;
	gsbp_ACK_universalACK_t* ACK = (gsbp_ACK_universalACK_t*)gCOM.ACK.Data;
	ACK->cmd = ResponceToCMD;
	ACK->cmd |= GSBP_SETUP__DEVICE_CLASS_ID;
	ACK->success = (uint8_t)Success;

	gCOM.ACK.DataSize = sizeof(gsbp_ACK_universalACK_t);

	GSBP_SendPackageAll(Handle, &gCOM.ACK, true);
}

void GSBP_SendMSG(GSBP_Handle_t *Handle, gsbp_MsgTypes_t Type, uint8_t State, uint16_t ErrorCode, const char* Message, ...)
{
	gCOM.ACK.CommandID = MessageACK;
	gCOM.ACK.RequestID = 0;
	gsbp_ACK_messageACK_t* ACK = (gsbp_ACK_messageACK_t*)gCOM.ACK.Data;
	ACK->msgType = (uint8_t)Type;
	ACK->state = State;
	ACK->errorCode = ErrorCode;
	va_list args;
	va_start(args, Message);
	vsnprintf((char *)ACK->msg, (GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -GSBP__COMMAND_SIZE_MSG -GSBP__PACKAGE_SIZE_TAIL), Message, args);
	va_end(args);

	gCOM.ACK.DataSize = GSBP__COMMAND_SIZE_MSG + strlen((const char*)ACK->msg);
	if (gCOM.ACK.DataSize > (GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -GSBP__PACKAGE_SIZE_TAIL)){
		gCOM.ACK.DataSize = GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -GSBP__PACKAGE_SIZE_TAIL;
		gCOM.ACK.Data[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -GSBP__PACKAGE_SIZE_TAIL -1] = 0x00;
	}

	if (Handle != NULL){
		GSBP_SendPackageAll(Handle, &gCOM.ACK, true);
	} else {
		gsbpDebugMSG(3, (const char*)gCOM.ACK.Data);
	}
}


__weak void GSBP_SendNodeInfo(GSBP_Handle_t *Handle, uint8_t RequestID)
{
    gCOM.ACK.CommandID = NodeInfoACK;
    gCOM.ACK.RequestID = RequestID;
    gsbp_ACK_nodeInfo_t* ACK = (gsbp_ACK_nodeInfo_t*)gCOM.ACK.Data;
    ACK->boardID = 0x00;
    ACK->deviceClass = GSBP_SETUP__DEVICE_CLASS_ID;
    ACK->serialNumber = 1904010001;
    ACK->versionProtocol[0] = 0;
    ACK->versionProtocol[1] = 1;
    ACK->versionFirmware[0] = 0;
    ACK->versionFirmware[1] = 1;

    sprintf((char *)ACK->msg, "GSBP: NODE DESCRIPTION -> TODO");
    gCOM.ACK.DataSize = GSBP__COMMAND_SIZE_NODEINFO + strlen((const char*)ACK->msg);
    if (gCOM.ACK.DataSize > GSBP_SETUP__MAX_PAYLOAD_SIZE_TX){
        gCOM.ACK.DataSize = GSBP_SETUP__MAX_PAYLOAD_SIZE_TX;
        gCOM.ACK.Data[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -1] = 0x00;
    }

    GSBP_SendPackageAll(Handle, &gCOM.ACK, true);
}

__weak void GSBP_SendStatus(GSBP_Handle_t *Handle, uint8_t RequestID)
{
	gCOM.ACK.CommandID = StatusACK;
	gCOM.ACK.RequestID = RequestID;
	gsbp_ACK_status_t* ACK = (gsbp_ACK_status_t*)gCOM.ACK.Data;
	ACK->errorCode = 0;
	ACK->state = 0;
	uint8_t sizeACK = (uint8_t)sizeof(gsbp_ACK_status_t) -(uint8_t)sizeof(uint8_t*);
	ACK->msg[0] = 0x00;
	gCOM.ACK.DataSize = sizeACK + strnlen((const char*)ACK->msg, (GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -sizeACK));
	GSBP_SendPackageAll(Handle, &gCOM.ACK, true);
}

void gsbpDebugMSG(uint8_t level, const char* message, ...)
{
#if GSBP_SETUP__DEBUG_LEVEL >= 1
	#ifdef GSBP_DEBUG_UART
		if (level > GSBP_SETUP__DEBUG_LEVEL){
			return;
		}
		// wait until the UART is free (mostly if DMA is use)
		uint32_t counter = 200000;
		while((GSBP_DEBUG_UART.gState & HAL_UART_STATE__TX_ACTIVE) && counter ){
			counter--;
		}
		va_list args;
		va_start(args, message);
		vsnprintf((char *)gCOM.debMSG, sizeof(gCOM.debMSG), message, args);
		va_end(args);
		#if GSBP_DEBUG_UART_USES_DMA ==1
			HAL_UART_Transmit_DMA(&GSBP_DEBUG_UART, (uint8_t*)gCOM.debMSG, strlen((const char *)gCOM.debMSG));
		#else
			HAL_UART_Transmit(&GSBP_DEBUG_UART, (uint8_t*)gCOM.debMSG, strlen((const char *)gCOM.debMSG), GSBP_SETUP__UART_TX_SEND_TIMEOUT);
		#endif
	#endif
#endif
}


		/*
 * ReSend Command
 */
bool GSBP_ReSendPackage(GSBP_Handle_t *Handle)
{
    if (Handle->TxBufferSize == 0) {
        return 1; // TRUE
    }

#if (GSBP_SETUP__INTERFACE_UART_USED)
    // UART Interface
    if (Handle->InterfaceType == GSBP_InterfaceUART){
#if (GSBP_SETUP__UART_TX_METHOD == 0)	// Polling
    	if (HAL_UART_Transmit(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) Handle->TxBufferSize, GSBP__UART_TX_TIMEOUT) == HAL_OK) {
#endif
#if (GSBP_SETUP__UART_TX_METHOD == 1)	// IT
 		if (HAL_UART_Transmit_IT(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) Handle->TxBufferSize) == HAL_OK) {
#endif
#if (GSBP_SETUP__UART_TX_METHOD == 2)	// DMA
 		if (HAL_UART_Transmit_DMA(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) Handle->TxBufferSize) == HAL_OK) {
#endif
 			// buffer send -> set the buffer size to zero
 			Handle->TxBufferSize = 0;
 			return 1; // TRUE
 		}
    }
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
 	// USB Interface
 	if (Handle->InterfaceType == GSBP_InterfaceUSB) {
 		if (CDC_Transmit_FS( (uint8_t *) Handle->TxBuffer, (uint16_t) (Handle->TxBufferSize)) == USBD_OK) {
 			// buffer send -> set the buffer size to zero
 			Handle->TxBufferSize = 0;
 		    return 1; // TRUE
 		}
 	}
#endif
    return 0; // FALSE
}

bool GSBP_ReSendPackages(void){
	// check all enabled GSBP handles
    uint8_t i = 0, PackagesReSent = 0;

	for (i=0; i< gCOM.nActiveHandles; i++){
		if (gCOM.Handles[i]->UART_Handle != NULL){
			// check if a buffer needs to be resent
    		if (gCOM.Handles[i]->TxBufferSize != 0) {
    			PackagesReSent += GSBP_ReSendPackage(gCOM.Handles[i]);
    		}
    	}
    	if (gCOM.Handles[i]->InterfaceType == GSBP_InterfaceUSB){
    		// check if a buffer needs to be resent
    		if (gCOM.Handles[i]->TxBufferSize != 0) {
    			PackagesReSent += GSBP_ReSendPackage(gCOM.Handles[i]);
    		}
    	}
    }
    return PackagesReSent;
}

__weak void GSBP_BuildPackageCallback(GSBP_Handle_t *Handle)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(Handle);
}

/***
 * ### GSBP "private" functions ###
 *
 * helper functions for GSBP
 ***/

// get the correct request ID
uint8_t GSBP_GetRequestID(GSBP_Handle_t *Handle, uint8_t RequestIDToUse)
{
    if (RequestIDToUse == 0){
        // use the last request ID received
    	return Handle->LastRxRequestID;
    } else {
    	return RequestIDToUse;
    }
}

// get the header checksum for the given buffer -> TODO better checksum's!!!
uint8_t GSBP_GetHeaderChecksum(uint8_t *Buffer)
{
    uint8_t i, ChecksumHeaderTemp = GSBP__PACKAGE_HEADER_CHECKSUM_START;
    // TODO andere Paketstrukturen einbeziehen
    //gsbpDebugMSG(4, "%\n HC: ");
    for (i = 0; i < 4; i++){
        ChecksumHeaderTemp ^= Buffer[i];
        //gsbpDebugMSG(4, "0x%02X ", Buffer[i]);
        //TODO Checksumme ist nicht gut -> lieber one's sum ....
        //http://betterembsw.blogspot.de/2010/05/which-error-detection-code-should-you.html
    }
    //gsbpDebugMSG(4, "%\n HC: ");
    return ChecksumHeaderTemp;
}

// get the data checksum for the given buffer
uint8_t GSBP_GetDataChecksum(uint8_t *Buffer, uint8_t length)
{
	if (length > 50){
		return 0;
	}
	uint8_t i, ChecksumDataTemp = 0x00;
    // TODO andere Paketstrukturen einbeziehen
    for (i = 0; i < length; i++){
        ChecksumDataTemp ^= Buffer[i];
        //TODO Checksumme ist nicht gut -> lieber one's sum ....
        //http://betterembsw.blogspot.de/2010/05/which-error-detection-code-should-you.html
    }
    return ChecksumDataTemp;
}



/***
 *  GSBP HAL ReImplementations
 *  ############################################################################
 ***/

/***
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of IT Rx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
#if (GSBP_SETUP__DEBUG_LEVEL >= 4)
	gsbpDebugToggle_D4();
#endif

	// IT
#if (GSBP_SETUP__UART_RX_METHOD == 1)	// IT
	//search the right GSBP handle
	GSBP_Handle_t *Handle = NULL;
	uint8_t i = 0;
	for (i=0; i< gCOM.nActiveHandles; i++){
		if ( (gCOM.Handles[i]->UART_Handle - huart) == 0 ){
			Handle = gCOM.Handles[i];
			break;
		}
	}

	if (Handle != NULL) {
		// save the full buffer
		GSBP_SaveBuffer(Handle);
		if (Handle->InterfaceType == GSBP_InterfaceUART){
			// restart the RX interrupt read in
			if (HAL_UART_Receive_IT(Handle->UART_Handle, Handle->RxBuffer, GSBP_SETUP__RX_BUFFER_SIZE) != HAL_OK){
				MCU_ErrorHandler(EH_GSBP_InitError);
			}
		}
	} else {
		MCU_ErrorHandler(EH_GSBP_HandleError);
	}
#endif

	// DMA
	// DMA should be configured in circular mode -> no implementation needed
	// The callback is still called if the the buffer resets

#if (GSBP_SETUP__DEBUG_LEVEL >= 4)
	gsbpDebugToggle_D4();
#endif
}

/**
  * @brief  Tx Transfer completed callbacks.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
/*void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	UNUSED(huart);
}*/

/***
  * @brief  UART error callbacks.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{

	gsbpDebugMSG(1, "UART Error: errorCode= %ld\n", huart->ErrorCode);

	//search the right GSBP handle
	GSBP_Handle_t *Handle = NULL;
	uint8_t i = 0;
	for (i=0; i< gCOM.nActiveHandles; i++){
		if ( (gCOM.Handles[i]->UART_Handle - huart) == 0 ){
			Handle = gCOM.Handles[i];
			break;
		}
	}

	if (huart->ErrorCode == HAL_UART_ERROR_ORE){
		// remove the error condition
		huart->ErrorCode = HAL_UART_ERROR_NONE;
		// set the correct state, so that the UART_RX_IT works correctly
		huart->gState = HAL_UART_STATE_BUSY_RX;
	}

	// TODO: handle all errors ...
	if ((void*)Handle != NULL) {
		// set the flag
		Handle->State |= GSBP_HandleState__ReceiveError;
	} else {
		MCU_ErrorHandler(EH_GSBP_HandleError);
	}
}

