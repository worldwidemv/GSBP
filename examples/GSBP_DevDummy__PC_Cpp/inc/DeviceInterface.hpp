/*
 * # Specific Device Interface #
 *   C++ Interface class for the GSBP_DevDummy device,
 *   utilizing the GSBP communication interface class.
 *
 *   Copyright (C) 2015-2020 Markus Valtin <os@markus.valtin.net>
 *
 *   Author:  Markus Valtin
 *   File:    DeviceInterface.hpp -> Header file for the device interface class
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

#ifndef DUMMYDEVICE_INTERFACE_H
#define DUMMYDEVICE_INTERFACE_H

#if defined(__APPLE__) || defined(__unix__)
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#else
// Windows is not supported at the moment
//#include <windows.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cstring>
#include <math.h>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>

#include "GSBP_DevDummy.hpp"
#include <boost/thread/mutex.hpp>

using namespace ns_GSBP_DD_01;
#define DUMMYDEVICE_STATUS_MSG_SIZE_MAX			1024
#define DUMMYDEVICE_PAYLOAD_SIZE_MAX			512

/*
 * DummyDevice name space
 */
namespace nsDUMMYDEVICE_01 {

// GSBP Command IDs used
typedef enum {
	// GSBP universal commands
	NodeInfoCMD						= 1,
	NodeInfoACK						= 2,
	UniversalACK                	= 3,
	MessageACK                  	= 4,
	StatusCMD                   	= 5,
	StatusACK                   	= 6,
	ResetCMD                    	= 9,
	/*
	 * TODO
	 * Add task/board specific commands
	 */
	InitCMD							= 200,
	InitACK							= 201,
	StartApplicationCMD				= 210,
	ApplicationDataACK				= 216,
	StopApplicationCMD				= 220,
	DeInitCMD						= 230,
} mcu_Command_t;

// GSBP Error codes used with GSBP_SendError()
typedef enum {
	// GSBP standard errors
	E_NoError                       = 0,
	E_UnknownCMD                	= 1,
	E_ChecksumMissmatch             = 2,
	E_EndByteMissmatch              = 3,
	E_UARTSizeMissmatch             = 4,
	E_BufferToSmall					= 5,
	E_DeviceClass_InValid           = 9,
	E_CMD_NotValidNow				= 11,
	E_CMD_NotExpected				= 12,
	E_State_UnknowState				= 15,
	/*
	 * TODO
	 * Add task/board specific error codes
	 */
	E_NoNewData						= 20
} mcu_ErrorCode_t;

/*
 * ### Project specific definitions ###
 */
typedef struct __packed {
	uint16_t errorCode;
	uint8_t  state;
	uint8_t  msg[DUMMYDEVICE_STATUS_MSG_SIZE_MAX];
} mcuStatus_t;

typedef struct __packed {
	uint16_t dataSize;
	uint16_t dataPeriodMS;
	uint8_t  increment;
} initCMD_t;

typedef struct __packed {
	bool     success;
	uint16_t dataSize;
	uint16_t dataPeriodMS;
	uint8_t  increment;
} initACK_t;

typedef struct __packed {
	uint16_t numberOfValues;
	int16_t data[DUMMYDEVICE_PAYLOAD_SIZE_MAX];
} measurementDataACK_t;


class DummyDevice {
public:
	DummyDevice(const char *DeviceID, const char *SerialDeviceFile);
	~DummyDevice(void);

    bool    IsDeviceConnected(void);
    bool 	GetNodeInfo(GSBP_DD::gsbp_ACK_nodeInfo_t *NodeInfo, bool PrintNodeInfo);
    bool 	GetNodeInfo(GSBP_DD::gsbp_ACK_nodeInfo_t *NodeInfo, bool PrintNodeInfo, uint16_t *ErrorCode);
    bool 	GetStatus(mcuStatus_t *Status);
    bool 	GetStatus(mcuStatus_t *Status, uint16_t *ErrorCode);
	bool 	InitialiseMCU(initCMD_t *InitConfig, initACK_t *InitResponce);
	bool 	InitialiseMCU(initCMD_t *InitConfig, initACK_t *InitResponce, uint16_t* ErrorCode);
	bool	StartMCUApplication(void);
	bool 	StartMCUApplication(uint16_t* ErrorCode);
	uint16_t GetData(int16_t Data[], uint16_t NumberOfValuesMax);
	bool	StopMCUApplication(void);
	bool 	StopMCUApplication(uint16_t* ErrorCode);
	bool 	DeinitialiseMCU(void);
	bool 	DeinitialiseMCU(uint16_t* ErrorCode);

private:
    //global parameter
	GSBP_DD *Interface;
	char DeviceIDClass[100];
	char DeviceFileName[255];
	bool DeviceInitialised;
	uint16_t NumberOfDataValues;
	int16_t DataValues[50000];
	boost::mutex DataMutex;

	/*
	 * private functions
	 */

	// GSBP callback functions
	bool 	 	PackageHandler(GSBP_DD::rxPackage_t *Package, uint64_t RequestID);
	const char*	GetCommandIdString(uint16_t Cmd);
	const char*	GetErrorString(uint16_t ErrorCode);
};

} // namespace

#endif /* DUMMYDEVICE_INTERFACE_H */
