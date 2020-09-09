/*
 * # GeneralSerialByteProtocol #
 *   a communication interface for inclusion in upper device classes,
 *   e.g. for communication with the GSBP MCU counterpart.
 *
 *   Copyright (C) 2015-2020 Markus Valtin <os@markus.valtin.net>
 *
 *   Author:  Markus Valtin
 *   File:    GSBP_XXX.hpp -> Header file for the communication interface class
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

#ifndef GSBP_XXX_H
#define GSBP_XXX_H

// TODO: cleanup C/C++ includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <math.h>

#include <functional>
#include <iostream>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/circular_buffer.hpp>
//#define  BOOST_CB_ENABLE_DEBUG							1
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>


// ### GSBP SETUP ###

// Debug Support
#define GSBP__DEBUG_SENDING_COMMANDS                    	0
#define GSBP__DEBUG_RECEIVING_COMMANDS                  	0
#define GSBP__DEBUG_RECEIVING_COMMANDS_EXEPT_MEAS_ACKS  	0
#define GSBP__DEBUG_MCU_AND_INCREASE_TIMEOUTS				1
#define GSBP__DEBUG_GSBP_STATS								1
#define GSBP__DEBUG_SERIAL_ACTIONS							0
#define GSBP__DEBUG_REQUEST_AND_RESPONSE_BUFFER				0
#define GSBP__DEBUG_REQUEST_AND_RESPONSE_BUFFER_ALL			0
#define GSBP__DEBUG_REQUEST_AND_RESPONSE_BUFFER_PACKAGES 	0

// Communication Payload
const uint32_t gsbp_TxMaxUserDataSize						= 3000; // max amount of data (bytes) send in one package
const uint32_t gsbp_RxMaxUserDataSize						= 3000; // max amount of data (bytes) received in one package

// Serial Interface	- ignored by the USB virtual com port
#define GSBP__UART_BAUTRATE                             	B1000000 //B115200 // B921600 //B2000000 //B3500000
#define GSBP__UART_USE_UART_FLOW_CONTROL                	0

// Package Structure
#define GSBP__ACTIVATE_DESTINATION_FEATURE              	0
#define GSBP__ACTIVATE_SOURCE_FEATURE                   	0
#define GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE       	0
#define GSBP__ACTIVATE_16BIT_CMD_FEATURE                	0
#define GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE     	1

#define GSBP__USE_CHECKSUMS									0
#define GSBP__ACTIVATE_32BIT_CRC_DATA_CHECKSUM          	0
#define GSBP__ACTIVATE_16BIT_CRC_DATA_CHECKSUM          	0
#define GSBP__ACTIVATE_8BIT_CRC_HEADER_CHECKSUM         	0
#define GSBP__ACTIVATE_8BIT_XOR_HEADER_CHECKSUM         	0

// Misc
#define	__packed											__attribute__((__packed__))
const uint32_t gsbp_AdditionalTimeOutForMcuDebuging			= 500;
const uint32_t gsbp_PackageReadTimoutUs             		= 11000; // important timeout to detect  a if not every byte of a package is received and to check if the ReadPackage function should terminate

const uint32_t gsbp_ErrorStringSize     					= 100;
const uint32_t gsbp_RequestResponceBufferSize				= 500;

const uint32_t gsbp_MaxGsbpHeaderSize						= 50; //max 50 byte for the package overhead
const uint32_t gsbp_TxMaxPackageSize						= (gsbp_TxMaxUserDataSize + gsbp_MaxGsbpHeaderSize);
const uint32_t gsbp_RxMaxPackageSize						= (gsbp_RxMaxUserDataSize + gsbp_MaxGsbpHeaderSize);
const uint32_t gsbp_MaxErrorCodeNumber     					= 32;

namespace ns_GSBP_XXX_01 {

    class GSBP_XXX
    {
    public:
        /* Public Definitions */

        // GSBP errors -> first gsbp_MaxErrorCodeNumber numbers
        enum  error_t {
            NoError                   			= 0,
			GSBP_NotConnectedToDevice			= 1,
			GSBP_InvalidCMD				        = 2,
			GSBP_NoRequestFound		        	= 3,
			GSBP_GetResponseTimeout		        = 4,
			GSBP_OpeningTheDeviceFailed			= 5,
			GSBP_NodeInfoWasNotReceived			= 6,
			GSBP_DeviceClassDoesNotMatch		= 7,
            UnknownCMDError                     = 11,
            ChecksumMissmatchError              = 12,
            EndByteMissmatchError               = 13,
            UARTSizeMissmatchError              = 14,
            GSBP_Error__CMD_NotValidNow         = 20,
            MaxGSBP_ErrorEnumIDs                = gsbp_MaxErrorCodeNumber
        };

        // Misc definitions
        enum misc_t {
        	InvalidRequestID					= 0
        };

    	// RX package -> receiving
        struct rxPackage_t {
            uint16_t        CommandID;                      // CMD/ACK ID
            uint8_t			State;                          // packet status
            uint8_t  		RequestID;
            // additional features, see GSBP configuration
            #if GSBP__ACTIVATE_DESTINATION_FEATURE || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE
            uint8_t         Destination;                    // destination
            #endif
            #if GSBP__ACTIVATE_SOURCE_FEATURE || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE
            uint8_t         Source;                         // source
            #endif
            uint32_t        DataSize;                     	// user payload size
            unsigned char   Data[gsbp_RxMaxUserDataSize];   // user payload
        };

        // TX package -> sending
        struct txPackage_t {
            uint16_t        CommandID; 						// CMD/ACK ID
            // additional features, see GSBP configuration
            #if GSBP__ACTIVATE_DESTINATION_FEATURE || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE
            uint8_t         Destination;                    // destination
            #endif
            #if GSBP__ACTIVATE_SOURCE_FEATURE || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE
            uint8_t         Source;                         // source
            #endif
            uint32_t        DataSize;                       // user payload size
            unsigned char   Data[gsbp_TxMaxUserDataSize];	// user payload
        };

        // node info data
        struct __packed gsbp_ACK_nodeInfo_t {
        	uint64_t boardID;
        	uint16_t deviceClass;
        	uint32_t serialNumber;
        	uint8_t  versionProtocol[2];
        	uint8_t  versionFirmware[2];
        	uint8_t  msg[gsbp_RxMaxUserDataSize];
        };

        // configuration and callback's, which need to be implemented by the upper device class
        struct gsbpConfiguration_t {
        	bool UpdateDeviceID;
        	char DeviceID[100];
        	bool UseThreadToRead;
        	std::function<bool(GSBP_XXX::rxPackage_t*, uint64_t)> PackageHandler;
        	std::function<const char*(uint16_t)> GetErrorString;
        	std::function<const char*(uint16_t)> GetCmdString;
        	uint16_t NodeInfoCMD_ID;
        	uint16_t NodeInfoACK_ID;
        	uint16_t MessageACK_ID;
        	uint16_t ApplicationDataACK_ID;
        	bool DisplayWarnings;
        	bool DisplayErrors;
        };

        /* Public Functions */
        GSBP_XXX(void);
        GSBP_XXX(char* DeviceID);
        GSBP_XXX(char* DeviceID, char* DeviceFileName, uint16_t DeviceClass, gsbpConfiguration_t Config);
        ~GSBP_XXX(void);

        bool      IsDeviceConnected(void);
        bool      ConnectToDevice(char* DeviceFileName, uint16_t DeviceClass, bool UseThreadToRead, uint16_t* ErrorCode);
        bool	  UpdateConfiguration(gsbpConfiguration_t Config, uint16_t* ErrorCode);
        bool	  GetNodeInfo(gsbp_ACK_nodeInfo_t* NodeInfo, bool PrintNodeInfo, uint16_t* ErrorCode);
        uint64_t  SendPackage(txPackage_t* P, uint16_t* ErrorCode);
    	bool 	  GetResponse(uint64_t RequestId, uint16_t AckId, rxPackage_t* ACK, int MilliSecondsToWait, uint32_t* NumberOfOpenRequests, uint16_t* ErrorCode);
    	bool      DisconnectFromDevice(uint16_t* ErrorCode);

    	const char* GetGsbpErrorString(uint16_t ErrorCode);
        void      PrintPackage(txPackage_t* Package);
        void      PrintPackage(rxPackage_t* Package);
        void 	  PrintRequestResponse(uint64_t RequestId, bool PrintPackageContent);
        void      PrintRequestResponseBuffer(bool ShowAllEntries);
        void      PrintStatsGSBP(void);

    private:
        /* Private Definitions */

    	// package state
    	enum packageState_t {
    		PackageIsBroken                      = 0,
    		PackageIsBroken_IncompleteTimout     = 1,
    		PackageIsBroken_IncompleteData       = 2,
    		PackageIsBroken_InvalidCommandID     = 3,
    		PackageIsBroken_StartByteError       = 10,
    		PackageIsBroken_EndByteError         = 11,
    		PackageIsOk                          = 128
    	};

    	// request/response element
    	struct RequestResponse_t {
            uint8_t  RequestIdLocal;      		// raw request ID (8 bit); info
            uint64_t RequestIdGlobal;       	// absolute request ID (32 bit); info
            uint8_t  RequestIdLocal_Debug;
            uint64_t RequestIdGlobal_Debug;
            bool IsDummyCopy;
    		bool WaitForResponce;
    		bool WaitTimedOut;
    		bool ResponseReceived;
    		txPackage_t Cmd;
    		rxPackage_t Ack;
    		bool Error;
    		uint16_t ErrorCode;
    		char ErrorDescription[gsbp_ErrorStringSize];
    		uint8_t  txChecksumHeader;          // header checksum; info
            uint32_t txChecksumData;            // data checksum; info
    		uint8_t  rxChecksumHeader;          // header checksum; info
            uint32_t rxChecksumData;            // data checksum; info
#if GSBP__DEBUG_SENDING_COMMANDS
            boost::posix_time::ptime CmdTime;
#endif
#if GSBP__DEBUG_RECEIVING_COMMANDS
            boost::posix_time::ptime AckTime;
            uint32_t AckWaitTimeout;
#endif
    	};

        // transmission statistics
        struct statsGSBP_t {
        	boost::posix_time::ptime StartTime;
            uint64_t NumberOfRxPackages;
            uint64_t NumberOfRxPackages_Missing;
            uint64_t NumberOfRxPackages_BrokenStructur;
            uint64_t NumberOfRxPackages_BrokenChecksum;
            uint64_t BytesDiscarded;

            uint8_t  LocalTxRequestID;
            uint64_t GlobalTxRequestID;
        } StatsGSBP;


        /* Private Variables */
        char     ID[255];
        char     DeviceFileName[255];
        uint16_t DeviceClass;
        int      fd;
        bool     DeviceConnected;
        bool     RunReceiverThread;
        bool     ReceiverThreatRunning;

        uint8_t  TxBuffer[gsbp_TxMaxPackageSize];
        uint32_t TxBufferSize;

        // external configuration
        gsbpConfiguration_t	ExtConfig;

        // receiver thread
        boost::thread* Receiver_thread;
        boost::mutex   ReadPackage_mutex;

        // request/response buffer
    	boost::circular_buffer<RequestResponse_t> RequestResponseBuffer;
        boost::mutex RequestResponseLock_mutex;
        uint32_t     UnclaimedRequestResponces;


        /* Private Functions */
        void      InitialiseVariables(void);
        void	  SetDefaultExtConfiguration(void);
        int       OpenDevice(void);

        void 	  AddRequest(RequestResponse_t item);
        bool      ReadPackages(bool doReturnAfterTimeout);
        void      BuildPackage(uint8_t* RxBuffer, uint32_t RxBufferSize, packageState_t State, uint8_t ChecksumHeader);
        uint64_t  AddResponse(rxPackage_t* Response);

        bool      ExtPackageHandler(rxPackage_t* Package, uint64_t RequestId);
        void      PackageHandler_Debug(rxPackage_t* Package);
        void 	  PackageHandler_Warning(rxPackage_t* Package);
        uint16_t  PackageHandler_Error(rxPackage_t* Package);

        const char* GetCmdString(uint8_t Id);
        const char* GetErrorString(uint16_t ErrorCode);

        uint8_t   GetNextRequestIdLocal(void);
        uint8_t   GetCurrentRequestIdLocal(void);
        uint64_t  GetCurrentRequestIdGlobal(void);

		void      DoPrintNodeInfo(gsbp_ACK_nodeInfo_t* NodeInfo);
        void      DoPrintPackageContent(txPackage_t* Package, uint8_t RequestIdLocal);
        void      DoPrintPackageContent(rxPackage_t* Package);
        void      DoPrintPackage(rxPackage_t* Package, bool IsACK);
        void      DoPrintRequestResponse(RequestResponse_t* Request, bool AddOnlyAck, bool PrintPackageContent);
        void	  DoPrintBits(void const*  const ptr, size_t const size);
    };

} // end namespace
#endif

