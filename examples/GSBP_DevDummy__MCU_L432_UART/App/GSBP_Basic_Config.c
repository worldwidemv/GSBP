/*
 * # GeneralSerialByteProtocol -> MCU Basic Project Implementations #
 *   a communication protocol and software module for microcontroller,
 *   suitable for various hardware interfaces (UART, USB, BT, ...)
 *
 *   Copyright (C) 2015-2020 Markus Valtin <os@markus.valtin.net>
 *
 *   Author:  Markus Valtin
 *   File:    GSBP_Basic_Config.c -> source file with the project specific implementation
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

#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "string.h"
#include "main.h"
#include "GSBP_Basic_Config.h"
#include "GSBP_Basic.h"

/*
 * ### Project specific global Variables ###
 * TODO
 * Add task/board specific implementation for each available interface
 */
#if (GSBP_SETUP__INTERFACE_UART_USED)
#include "usart.h"
GSBP_Handle_t GSBP_UART;			// UART interface
#endif
#if (GSBP_SETUP__INTERFACE_USB_USED)
GSBP_Handle_t GSBP_USB;			// real MCU USB interface
#endif

/*
 * ### Project specific Functions
 */

/*
 * initialize all handle variables and start the interfaces
 */
void GSBP_Init(void)
{
	// initialize the global handle list
	memset(&gCOM, 0, sizeof(gCOM));
	gCOM.DefaultHandle = 0;
	gCOM.NextCallbackTimer = HAL_GetTick() +GSBP_SETUP__CALLBACK_PERIOD_IN_MS;

	/*
	 * TODO
	 * Add task/board specific implementation for each available interface
	 */

	// clear the GSBP handles and configure them
#if (GSBP_SETUP__INTERFACE_UART_USED)
	memset(&GSBP_UART, 0, sizeof(GSBP_Handle_t));
	GSBP_UART.InterfaceType = GSBP_InterfaceUART;
	GSBP_UART.State |=  GSBP_HandleState__HandleDisabled;
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
	memset(&GSBP_USB, 0, sizeof(GSBP_Handle_t));
	GSBP_USB.InterfaceType = GSBP_InterfaceUSB;
	GSBP_USB.State |= GSBP_HandleState__HandleDisabled;
#endif

	// configure the default handle
	GSBP_UART.State |= GSBP_HandleState__DefaultHandle;

	// enable the handles if you do not use the GSBP_Management functions
#if (GSBP_SETUP__INTERFACE_UART_USED)
	GSBP_InitHandle(&GSBP_UART, &huart2);
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
	GSBP_InitHandle(&GSBP_USB, NULL);
#endif
}


/*
 * Check the interface/handle if any new data was received and if so,
 * check if the package is complete and execute the commands
 * Input: GSBP interface handle
 * Output: process another package directly afterwards (true = yes, false = no)
 */
bool GSBP_EvaluatePackage(gsbp_PackageRX_t *CMD, GSBP_Handle_t *Handle)
{
	gsbpDebugMSG(6, "EvalPackage: CMD:%d; Req# %d; Nbytes:%d (%ld)\n", CMD->CommandID, CMD->RequestID, CMD->DataSize, HAL_GetTick());

	switch (CMD->CommandID) {
	// universal commands
	case NodeInfoCMD:
		GSBP_SendNodeInfo(Handle, CMD->RequestID);
		break;
	case StatusCMD:
		GSBP_SendStatus(Handle, CMD->RequestID);
		break;
	case ResetCMD:
		GSBP_SendUniversalACK(Handle, ResetCMD, true);
		// TODO: implement project specific MCU/project reset
		//MCU_StateChange(MCU_Reset);
		return false;
		break;

	// task / board specific implementation
	case InitCMD:
		// start the initialization -> got to DoInit state
		//MCU_StateChange(MCU_DoInit);
		mcuState = doInit;
		return false; // do not process another package afterwards, because the initialization is done i the main loop
		break;
	case StartApplicationCMD:
		// start the measurement -> got to MeasurementDoStart state
		//MCU_StateChange(MCU_MeasurementDoStart);
		mcuState = startMeasurement;
		return false;
		break;
	case StopApplicationCMD:
		// stop the measurement -> got to MeasurementDoStop state
		//MCU_StateChange(MCU_MeasurementDoStop);
		mcuState = stopMeasurement;
		return false;
		break;
	case DeInitCMD:
		// reset -> got to MCU_DoDeInit state
		//MCU_StateChange(MCU_DoDeInit);
		mcuState = doDeInit;
		return false;
		break;

	default:
		GSBP_SendMSG(Handle, MsgCriticalError, GSBP_GetMcuState(), E_UnknownCMD, "GSBP: unknown CMD %d", CMD->CommandID);
	}

	// default: process another package directly afterwards
	return true;
}

/*
 * Project specific reimplementation of 'GSBP_SendNodeInfo()'.
 * Update here things like deviceClass, boardID, serialNumber, and firmwareVersion ....
 * 
 * The basic structure of the NodeInfoACK should not be changed, so this universal package is always valid.
 */
void GSBP_SendNodeInfo(GSBP_Handle_t *Handle, uint8_t RequestID)
{
    gCOM.ACK.CommandID = NodeInfoACK;
    gCOM.ACK.RequestID = RequestID;
    gsbp_ACK_nodeInfo_t* ACK = (gsbp_ACK_nodeInfo_t*)gCOM.ACK.Data;
    // boardID number, use as you like
    ACK->boardID = 0x00;
    // the deviceClass as defind in the GSBP_Setup
    ACK->deviceClass = GSBP_SETUP__DEVICE_CLASS_ID;
    // the serial number of the current device
    ACK->serialNumber = 1904010001; // getSerialNumber();
    // the version of the GSBP protocol/package definitions
    ACK->versionProtocol[0] = 0;
    ACK->versionProtocol[1] = 1;
    // the current firmware version
    ACK->versionFirmware[0] = 0;
    ACK->versionFirmware[1] = 1;

    // ACK->msg is a string or any other custom data
    sprintf((char *)ACK->msg, "GSBP: NODE DESCRIPTION -> TODO");
    gCOM.ACK.DataSize = GSBP__COMMAND_SIZE_NODEINFO + strlen((const char*)ACK->msg);
    if (gCOM.ACK.DataSize > GSBP_SETUP__MAX_PAYLOAD_SIZE_TX){
        gCOM.ACK.DataSize = GSBP_SETUP__MAX_PAYLOAD_SIZE_TX;
        gCOM.ACK.Data[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -1] = 0x00;
    }

    GSBP_SendPackageAll(Handle, &gCOM.ACK, true);
}

/*
 * Project specific reimplementation of 'GSBP_SendStatus()'.
 * Update/Add things your project needs for the status ACK.
 * 
 * The basic structure of the ACK can be adopted to the project needs.
 */
void GSBP_SendStatus(GSBP_Handle_t *Handle, uint8_t RequestID)
{
    gCOM.ACK.CommandID = StatusACK;
    gCOM.ACK.RequestID = RequestID;
    gsbp_ACK_status_t* ACK = (gsbp_ACK_status_t*)gCOM.ACK.Data;
    ACK->errorCode = 0;
    ACK->state = GSBP_GetMcuState();

    // add data or string ...
    sprintf((char *)ACK->msg, "TODO or set to 0");
    gCOM.ACK.DataSize = GSBP__COMMAND_SIZE_STATE + strlen((const char*)ACK->msg);
    if (gCOM.ACK.DataSize > GSBP_SETUP__MAX_PAYLOAD_SIZE_TX){
        gCOM.ACK.DataSize = GSBP_SETUP__MAX_PAYLOAD_SIZE_TX;
        gCOM.ACK.Data[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX -1] = 0x00;
    }

    GSBP_SendPackageAll(Handle, &gCOM.ACK, true);
}

uint8_t GSBP_GetMcuState()
{
	// TODO: return project specific MCU StateMachine state
	//return MCU_State_CurrentState;
	return 0;
}
