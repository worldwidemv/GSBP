/*
 * # GeneralSerialByteProtocol #
 *   a communication interface for inclusion in device classes,
 *   e.g. for communication with the GSBP MCU counterpart.
 *
 *   Copyright (C) 2015-2020 Markus Valtin <os@markus.valtin.net>
 *
 *   Author:  Markus Valtin
 *   File:    GSBP_DevDummy.cpp -> Source file for the communication interface class
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

#include "GSBP_DevDummy.hpp"

//
// GSBP defines
// #############################################################################

#define GSBP__UART_START_BYTE                           0x7E
#define GSBP__UART_END_BYTE                             0x81
#define GSBP__UART_HEADER_CHECKSUM_START                GSBP__UART_START_BYTE

// TODO: support more package structures
#if GSBP__USE_CHECKSUMS
#define GSBP__UART_PACKAGE_HEADER_SIZE                  6 // TODO determine this via preprocessor
#define GSBP__UART_DATA_CHECKSUM_SIZE                   4
#else
#define GSBP__UART_PACKAGE_HEADER_SIZE                  5 // TODO determine this via preprocessor
#define GSBP__UART_DATA_CHECKSUM_SIZE                   0
#endif
#define GSBP__UART_PACKAGE_TAIL_SIZE                    (GSBP__UART_DATA_CHECKSUM_SIZE + 1)  // x + stop byte   // TODO determine this via preprocessor
#define GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE     3 // TODO determine this via preprocessor

#define GSBP__DEBUG__PP_N_DATA_BYTES_PER_LINE           32 // how many bytes of package payload data should be shown in debug mode (PrintPackage(); format: 0xXX )

#define __MakeUChar(X)  	(unsigned char)(X & 0x0000FF)

const uint32_t gsbp_DefaultGetResponceTimeout     		= 300;

namespace ns_GSBP_DD_01 {

//TODO: remove
union convert16_t {
    uint8_t  c_data[2];
    uint16_t data;
};

typedef union {
    uint8_t  c_data[2];
    int16_t data;
} convert16s_t;

typedef union {
    uint8_t  c_data[4];
    uint32_t data;
} convert32_t;

	//
	enum gsbp_PackageTypes_t {
		NoCmdAck_or_Invalid	= 0
	};

	// GSBP message package
	enum gsbp_MsgTypes_t {
		MsgInvalid			= 0,
		MsgCriticalError 	= 1,
		MsgError 			= 2,
		MsgWarning 			= 3,
		MsgInfo 			= 4,
		MsgDebug 			= 5
	};
	struct __packed gsbp_ACK_messageACK_t {
		uint8_t  msgType;
		uint8_t  state;
		uint16_t errorCode;
		uint8_t  msg[1000];
	};

    /* ### #########################################################################
     * Public Functions
     * ### #########################################################################
     */

    /*
     * Constructors
     */
    GSBP_DD::GSBP_DD() : GSBP_DD((char*)"GSBP") {}
/*		RequestResponseBuffer(gsbp_RequestResponceBufferSize)
    {
    	// initialise the variables
    	GSBP_DD::InitialiseVariables();

    	// set ID
    	sprintf(this->ID,"GSBP");
    	// set start time
    	this->StatsGSBP.StartTime = boost::posix_time::microsec_clock::local_time();
    	// set the external configuration
    	GSBP_DD::SetDefaultExtConfiguration();
    }
*/
    GSBP_DD::GSBP_DD(char* DeviceID)
      : // initialisation
		RequestResponseBuffer(gsbp_RequestResponceBufferSize)
    {
        // initialise the variables
    	GSBP_DD::InitialiseVariables();

        // set ID
        sprintf(this->ID,"%s",DeviceID);
        // set start time
        this->StatsGSBP.StartTime = boost::posix_time::microsec_clock::local_time();
        // set the external configuration
        GSBP_DD::SetDefaultExtConfiguration();
    }

    GSBP_DD::GSBP_DD(char* DeviceID, char* DeviceFileName, uint16_t DeviceClass, gsbpConfiguration_t Config)
      : // initialisation
		RequestResponseBuffer(gsbp_RequestResponceBufferSize)
    {
        // initialise the variables
    	GSBP_DD::InitialiseVariables();

        // set ID
        sprintf(this->ID,"%s",DeviceID);
        // set start time
        this->StatsGSBP.StartTime = boost::posix_time::microsec_clock::local_time();
        // set the external configuration
        this->ExtConfig = Config;

        // open the character device and use it as tty/virtual com port
        uint16_t ErrorCode = NoError;
        GSBP_DD::ConnectToDevice(DeviceFileName, DeviceClass, this->ExtConfig.UseThreadToRead, &ErrorCode);
    }

    /*
     * Destructor
     */
    GSBP_DD::~GSBP_DD(void)
    {
        // print some statistics
#if GSBP__DEBUG_REQUEST_AND_RESPONSE_BUFFER
        GSBP_DD::PrintRequestResponseBuffer(GSBP__DEBUG_REQUEST_AND_RESPONSE_BUFFER_ALL);
#endif
#if GSBP__DEBUG_GSBP_STATS
        GSBP_DD::PrintStatsGSBP();
#endif

        // clear the queue
        this->RequestResponseBuffer.clear();

        // close the device
        uint16_t ErrorCode;
        GSBP_DD::DisconnectFromDevice(&ErrorCode);
    }

    bool GSBP_DD::IsDeviceConnected(void){
        return this->DeviceConnected;
    }

    bool GSBP_DD::ConnectToDevice(char* DeviceFileName, uint16_t DeviceClass, bool UseThreadToRead, uint16_t* ErrorCode)
    {
    	*ErrorCode = NoError;

    	// check if already connected
    	if (this->DeviceConnected){
    		GSBP_DD::DisconnectFromDevice(ErrorCode);
    	}

        // update device file
        memset(this->DeviceFileName, 0, sizeof(this->DeviceFileName));
        sprintf(this->DeviceFileName,"%s",DeviceFileName);
        this->ExtConfig.UseThreadToRead = UseThreadToRead;
        this->DeviceClass = DeviceClass;

        // Open the character device and use it as tty/virtual com port
        this->fd = GSBP_DD::OpenDevice();
        if (this->fd < 0){
            // the device could not be opened
            this->DeviceConnected = false;
            *ErrorCode = GSBP_OpeningTheDeviceFailed;
            std::cout << this->ID << "\e[1m\e[91m ERROR:\e[0m Can't open device " << this->DeviceFileName << "!!!" << std::endl;
            std::cout << "   " << strerror(errno) << " (" << errno << ")" << std::endl << std::endl;
            return false;
        } else {
            // file / socket is open
            this->DeviceConnected = true;
#if GSBP__DEBUG_SERIAL_ACTIONS
            std::cout << this->ID << ": Opening device " << this->DeviceFileName << " was successful!" << std::endl;
#endif
            // flush the serial data stream
            tcflush(fd, TCIOFLUSH);

            if (this->ExtConfig.UseThreadToRead) {
            	// start receiving packages
            	this->RunReceiverThread = true;
            	this->ReceiverThreatRunning = true;
            	this->Receiver_thread = new boost::thread(&GSBP_DD::ReadPackages, this, false);
            	this->ReceiverThreatRunning = true;
#if GSBP__DEBUG_SERIAL_ACTIONS
            	std::cout << this->ID << ": Receiver threat started" << std::endl;
#endif
        	}

            // get the NodeInfo
            gsbp_ACK_nodeInfo_t NodeInfo = {0};
            if (!GSBP_DD::GetNodeInfo(&NodeInfo, false, ErrorCode)){
            	// get NodeInfo failed
            	GSBP_DD::DisconnectFromDevice(ErrorCode);
            	*ErrorCode = GSBP_NodeInfoWasNotReceived;
            	return false;
            } else {
            	if (NodeInfo.deviceClass != this->DeviceClass){
            		// get NodeInfo failed
            		std::cout << this->ID << "\e[1m\e[91m ERROR:\e[0m The device class do not match (" << this->DeviceClass << "!=" << NodeInfo.deviceClass << ")!" << std::endl;
            		GSBP_DD::DoPrintNodeInfo(&NodeInfo);
            		GSBP_DD::DisconnectFromDevice(ErrorCode);
            		*ErrorCode = GSBP_DeviceClassDoesNotMatch;
            		return false;
            	}
            }
        }

        return true;
    }

    bool GSBP_DD::UpdateConfiguration(gsbpConfiguration_t Config, uint16_t* ErrorCode)
    {
    	this->ExtConfig.UpdateDeviceID = false;
    	if (Config.UpdateDeviceID){
    		sprintf(this->ExtConfig.DeviceID,"%s",Config.DeviceID);
    		sprintf(this->ID,"%s",Config.DeviceID);
    	}
    	this->ExtConfig.UseThreadToRead = Config.UseThreadToRead;
    	this->ExtConfig.PackageHandler = Config.PackageHandler;
    	this->ExtConfig.GetErrorString = Config.GetErrorString;
    	this->ExtConfig.GetCmdString = Config.GetCmdString;
    	this->ExtConfig.MessageACK_ID = Config.MessageACK_ID;
    	this->ExtConfig.ApplicationDataACK_ID = Config.ApplicationDataACK_ID;
    	this->ExtConfig.DisplayWarnings = Config.DisplayWarnings;
    	this->ExtConfig.DisplayErrors = Config.DisplayErrors;
    	return true;
    }

    bool GSBP_DD::GetNodeInfo(gsbp_ACK_nodeInfo_t* NodeInfo, bool PrintNodeInfo, uint16_t* ErrorCode)
    {
    	// send the NodeInfoCMD
    	txPackage_t P = {0};
    	P.CommandID = this->ExtConfig.NodeInfoCMD_ID;
    	P.DataSize = 0;
    	uint64_t RequestID = GSBP_DD::SendPackage(&P, ErrorCode);
    	if (RequestID == 0){
    		return false;
    	}

    	// wait for the response
    	rxPackage_t Ack = {0};
    	uint32_t NOR = 0;
    	if (GSBP_DD::GetResponse(RequestID, this->ExtConfig.NodeInfoACK_ID, &Ack, gsbp_DefaultGetResponceTimeout, &NOR, ErrorCode)){
    		// NodeInfo received
    		memcpy(NodeInfo, Ack.Data, Ack.DataSize);
    		if (NOR != 0){
    			// unexpected result received
    			std::cout << this->ID << " Get NodeInfo: received " << NOR << " unexpected responses" << std::endl;
    			for (uint16_t i = 0; i<NOR; i++){
    				if (GSBP_DD::GetResponse(RequestID, NoCmdAck_or_Invalid, &Ack, 0, &NOR, ErrorCode)){
    					GSBP_DD::PrintPackage(&Ack);
    				} else {
    					std::cout << "   -> ACK not matching the request ID" << std::endl;
    				}
    			}
    			std::cout << std::endl;
    		}
    		if (PrintNodeInfo){
    			GSBP_DD::DoPrintNodeInfo(NodeInfo);
    		}
    		return true;
    	} else {
    		// did not receive the NodeInfoAck TODO: better handling of NO ACKs
    		if (NOR > 1){
    			// unexpected result received
    			std::cout << this->ID << " GetNodeInfo: FAILED but received " << NOR << " unexpected responses" << std::endl;
    			for (uint16_t i = 0; i< NOR; i++){
    				if (GSBP_DD::GetResponse(RequestID, NoCmdAck_or_Invalid, &Ack, 0, &NOR, ErrorCode)){
    					GSBP_DD::PrintPackage(&Ack);
    				} else {
    					std::cout << "   -> ACK not matching the request ID" << std::endl;
    				}
    			}
    			std::cout << std::endl;
    		} else {
    			std::cout << this->ID << " GetNodeInfo: did NOT receive a response from the device" << std::endl;
    		}
    		return false;
    	}
    }

    /*
     * Send Command
     */
    uint64_t GSBP_DD::SendPackage(txPackage_t* P, uint16_t* ErrorCode)
    {
    	// check if connected to a device
    	if (!this->DeviceConnected){
    		*ErrorCode = GSBP_NotConnectedToDevice;
    		return 0;
    	}
    	// check if the CMD is valid
    	if (P->CommandID == 0){
    		*ErrorCode = GSBP_InvalidCMD;
    		return 0;
    	}
    	RequestResponse_t R;
    	R.Cmd = *P;
    	R.RequestIdLocal = GetNextRequestIdLocal();
    	R.RequestIdLocal_Debug = GetCurrentRequestIdLocal();
    	R.RequestIdGlobal = GetCurrentRequestIdGlobal();
    	R.RequestIdGlobal_Debug = GetCurrentRequestIdGlobal();
    	R.IsDummyCopy = false;
    	R.ResponseReceived = false;
    	R.WaitForResponce = false;
    	R.WaitTimedOut = false;
    	R.Error = false;
    	R.ErrorCode = NoError;

        // TODO check package
        //GSBP_DD::CheckPackage(pakage_t* Package)

        // ### build TxBuffer ###
        memset(this->TxBuffer, 0, sizeof(this->TxBuffer));
        TxBufferSize = 0;
        // SET HEADER
        TxBuffer[TxBufferSize++] = GSBP__UART_START_BYTE;
        #if GSBP__ACTIVATE_DESTINATION_FEATURE
        TxBuffer[TxBufferSize++] = __MakeUChar(Command->Destination);
        #endif
        #if GSBP__ACTIVATE_SOURCE_FEATURE
        TxBuffer[TxBufferSize++] = __MakeUChar(Command->Source);
        #endif
        #if GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE
        TxBuffer[TxBufferSize++] = //TODO
        #endif
        #if GSBP__ACTIVATE_16BIT_CMD_FEATURE
        TxBuffer[TxBufferSize++] = __MakeUChar((Command->CommandID >> 8));
        #endif
        TxBuffer[TxBufferSize++] = __MakeUChar(P->CommandID);
        #if GSBP__ACTIVATE_CONTROL_FEATURE
        TxBuffer[TxBufferSize++] = __MakeUChar(Command->Control);
        #endif
        TxBuffer[TxBufferSize++] = __MakeUChar(R.RequestIdLocal);
        #if GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE
        TxBuffer[TxBufferSize++] = __MakeUChar(P->DataSize);
        #endif
        TxBuffer[TxBufferSize++] = __MakeUChar((P->DataSize >>8));

#if GSBP__USE_CHECKSUMS
        // header checksum
        uint8_t	ChecksumHeaderTemp;
        ChecksumHeaderTemp = GSBP__UART_HEADER_CHECKSUM_START;
        for(uint32_t i=1; i<TxBufferSize; i++){
            ChecksumHeaderTemp ^= TxBuffer[i];
            //TODO Checksumme ist nicht gut -> lieber one's sum ....
            //http://betterembsw.blogspot.de/2010/05/which-error-detection-code-should-you.html
        }
        TxBuffer[TxBufferSize++] = ChecksumHeaderTemp;
        Command->ChecksumHeader = ChecksumHeaderTemp;
#endif
        if (P->DataSize > 0){
            // SET DATA
            memcpy( &TxBuffer[TxBufferSize], P->Data, P->DataSize);
            TxBufferSize += P->DataSize;

#if GSBP__USE_CHECKSUMS
            // SET Checksum Data
            TxBuffer[TxBufferSize++] = 0x00;      // TODO Checksum
            TxBuffer[TxBufferSize++] = 0x00;      // TODO Checksum
            TxBuffer[TxBufferSize++] = 0x00;      // TODO Checksum
            TxBuffer[TxBufferSize++] = 0x00;      // TODO Checksum
#endif
 //           Command->ChecksumData = 0x00;
        }
        // SET TAIL
        TxBuffer[TxBufferSize++] = GSBP__UART_END_BYTE;


        // ### send command ###
        if (write(this->fd, TxBuffer, TxBufferSize) != (int)TxBufferSize){
            printf("\e[1m\e[91m%s ERROR:\e[0m Can't write to %s: %s (%d)\n", this->ID, this->DeviceFileName, strerror(errno), errno);
            return false;
        }

        // add the request to the buffer
        GSBP_DD::AddRequest(R);

        //Debug
        #if GSBP__DEBUG_SENDING_COMMANDS
        GSBP_DD::PrintPackage(P);
        printf("\033[2A   Send: ");
        for(uint32_t i=0; i<TxBufferSize; i++){
            printf("0x%02X ", (uint8_t)TxBuffer[i]);
        }
        printf("\n\n\n");
        fflush(stdout);
        #endif

        return R.RequestIdGlobal;
    }


	bool GSBP_DD::GetResponse(uint64_t RequestId, uint16_t AckId, rxPackage_t* ACK, int MilliSecondsToWait, uint32_t* NumberOfOpenRequests, uint16_t* ErrorCode)
	{
		// check if connected to a device
    	if (!this->DeviceConnected){
    		*NumberOfOpenRequests = 0;
    		*ErrorCode = GSBP_NotConnectedToDevice;
    		return false;
    	}

        // get the packages if the receiver threat is not running
        if (!this->ReceiverThreatRunning){
            GSBP_DD::ReadPackages(true);
        }

        // default returns
        ACK->CommandID = 0;
        ACK->RequestID = 0;
        ACK->DataSize = 0;
        ACK->State = PackageIsBroken;
		*NumberOfOpenRequests = 0;
		*ErrorCode = NoError;

		// lock the queue
		boost::mutex::scoped_lock lock(this->RequestResponseLock_mutex);

		// wait for the expected response to arrive?
		bool WaitForResponce = false;
		if (MilliSecondsToWait <= 0){
			MilliSecondsToWait = 0;
		} else {
			WaitForResponce = true;
			MilliSecondsToWait++; // add one millisecond because of the loop...
#if GSBP__DEBUG_MCU_AND_INCREASE_TIMEOUTS
			MilliSecondsToWait += gsbp_AdditionalTimeOutForMcuDebuging;
#endif
		}

		// wait for the response ...
		// get the request(s)
		boost::circular_buffer<RequestResponse_t>::iterator Item = this->RequestResponseBuffer.begin();
		bool ResponceFound = false;
		do {
			// check the request and response buffer
			*NumberOfOpenRequests = 0;
			for (Item = this->RequestResponseBuffer.begin();
					Item != this->RequestResponseBuffer.end(); Item++){
				// is the local request ID identical?
				if (Item->RequestIdGlobal == RequestId){
					// this is the request
					(*NumberOfOpenRequests)++;
					Item->WaitForResponce = WaitForResponce;
					if (Item->ResponseReceived){
						// is the response valid?
						if (AckId == 0 || Item->Ack.CommandID == AckId){
							// yes -> return this ACK
							*ACK = Item->Ack;
							Item->RequestIdLocal = 0;
							Item->RequestIdGlobal = 0;
							ResponceFound = true;
						}
					}
				}
			}
			if (ResponceFound){
				this->UnclaimedRequestResponces--;
				--(*NumberOfOpenRequests);
				return true;
			}

			// wait
			if (MilliSecondsToWait > 0){
				lock.unlock();
				boost::this_thread::sleep(boost::posix_time::milliseconds(1));
				MilliSecondsToWait --;
				if (!this->ReceiverThreatRunning){
					GSBP_DD::ReadPackages(true);
				}
				lock.lock();
			}
		} while (MilliSecondsToWait >0);

		if (WaitForResponce){
			// check the request and response buffer again and mark the timeout,
			// because everything, that is there now was received after the timeout
			*ErrorCode = GSBP_GetResponseTimeout;
			*NumberOfOpenRequests = 0;
			for (Item = this->RequestResponseBuffer.begin();
					Item != this->RequestResponseBuffer.end(); Item++){
				// is the local request ID identical?
				if (Item->RequestIdGlobal == RequestId){
					// this is the request
					Item->WaitForResponce = true;
					Item->WaitTimedOut = true;
					(*NumberOfOpenRequests)++;
				}
			}
			this->StatsGSBP.NumberOfRxPackages_Missing++;

			if (*NumberOfOpenRequests == 0){
				*ErrorCode = GSBP_NoRequestFound;
			}
		}

		return false;
	}

	bool GSBP_DD::DisconnectFromDevice(uint16_t* ErrorCode)
	{
		*ErrorCode = NoError;
        // close the device
        if (this->DeviceConnected){
            // stop receiving packages
            if (this->ReceiverThreatRunning){
                // stop receiving packages
                this->RunReceiverThread = false;
                this->Receiver_thread->join(); // TODO: use signals to stop the thread
                this->ReceiverThreatRunning = false;
            }

            // flush the serial data stream
            int retval;
            tcflush(this->fd, TCIOFLUSH);
            while (retval = close(this->fd), retval == -1 && errno == EINTR) ;
#if GSBP__DEBUG_SERIAL_ACTIONS
            std::cout << this->ID << ": Device " << this->DeviceFileName << " closed" << std::endl;
#endif
            this->DeviceConnected = false;
        }
        return true;
	}

    const char* GSBP_DD::GetGsbpErrorString(uint16_t ErrorCode)
    {
    	if (ErrorCode < gsbp_MaxErrorCodeNumber){
            switch ( (error_t)ErrorCode ) { // TODO update
                case NoError:      			  		return "NoError";

                case UnknownCMDError:         		return "UnknownCMDError";
                case ChecksumMissmatchError:  		return "ChecksumMissmatchError";
                case EndByteMissmatchError:   		return "EndByteMissmatchError";
                case UARTSizeMissmatchError:  		return "UARTSizeMissmatchError";

                case GSBP_Error__CMD_NotValidNow: 	return "CMD_NotValidNow";

                case MaxGSBP_ErrorEnumIDs:         	return "Max GSBP ErrorCode";
                default:                      		return "Unknown Error Code";
            }
    	}
    	return "Not a GSBP ErrorCode";
    }

    void GSBP_DD::PrintPackage(txPackage_t* Package)
    {
    	rxPackage_t P;
    	P.CommandID = Package->CommandID;
    	memcpy(P.Data, Package->Data, Package->DataSize);
    	P.DataSize = Package->DataSize;
    	P.RequestID = GetCurrentRequestIdLocal();
    	P.State = PackageIsOk;
    	this->DoPrintPackage(&P, false);
    }

    void GSBP_DD::PrintPackage(rxPackage_t* Package)
    {
    	this->DoPrintPackage(Package, true);
    }


    void GSBP_DD::PrintRequestResponse(uint64_t RequestId, bool PrintPackageContent)
    {
    	std::cout << std::endl << this->ID << " Request/Response:" << std::endl;

    	// lock the queue
        boost::mutex::scoped_lock lock(this->RequestResponseLock_mutex);
    	for (auto Request = this->RequestResponseBuffer.rbegin();
    			Request != this->RequestResponseBuffer.rend(); ++Request){
    		if (Request->RequestIdGlobal_Debug == RequestId){
    			if (!Request->IsDummyCopy){
    				GSBP_DD::DoPrintRequestResponse(&(*Request), false, PrintPackageContent);
    			} else {
    				GSBP_DD::DoPrintRequestResponse(&(*Request), true, PrintPackageContent);
    			}
    		}
    	}
    	lock.unlock();
    	std::cout << std::endl;
    }

    void GSBP_DD::PrintRequestResponseBuffer(bool ShowAllEntries)
    {
    	std::cout << std::endl << this->ID << " Request/Response Buffer Debugging: Size=" << this->RequestResponseBuffer.size();
    	std::cout << " with " << this->UnclaimedRequestResponces << " unclaimed requests" << std::endl;

    	// lock the queue
        boost::mutex::scoped_lock lock(this->RequestResponseLock_mutex);
    	for (auto Request = this->RequestResponseBuffer.rbegin();
    			Request != this->RequestResponseBuffer.rend(); ++Request){
    		if (ShowAllEntries || Request->RequestIdLocal != 0){
    			if (!Request->IsDummyCopy){
    				GSBP_DD::DoPrintRequestResponse(&(*Request), false, GSBP__DEBUG_REQUEST_AND_RESPONSE_BUFFER_PACKAGES);
    			} else {
    				GSBP_DD::DoPrintRequestResponse(&(*Request), true, GSBP__DEBUG_REQUEST_AND_RESPONSE_BUFFER_PACKAGES);
    			}
    		}
    	}
    	lock.unlock();
    	std::cout << std::endl;
    }

    void GSBP_DD::PrintStatsGSBP()
    {
        // print statistics
        printf("\n%s GSBP Statistics:\n   Packages received = %lu (missing: %lu | broken checksum: %lu | broken structure: %lu | bytes discarded: %lu)\n   Packages send = %lu\n\n",
               this->ID, this->StatsGSBP.NumberOfRxPackages, this->StatsGSBP.NumberOfRxPackages_Missing, (long unsigned int)this->StatsGSBP.NumberOfRxPackages_BrokenChecksum, this->StatsGSBP.NumberOfRxPackages_BrokenStructur, this->StatsGSBP.BytesDiscarded,
               this->StatsGSBP.GlobalTxRequestID
        );
        fflush(stdout);
    }

    /* ### #########################################################################
     * Private Functions
     * ### #########################################################################
     */

    void GSBP_DD::InitialiseVariables(void)
    {
        // Initialise the variables
        memset(this->ID, 0, sizeof(this->ID));
        memset(this->DeviceFileName, 0, sizeof(this->DeviceFileName));

        this->DeviceClass = 0;
        this->fd = 0;
        // bool's
        this->DeviceConnected = false;
        this->RunReceiverThread= false;
        this->ReceiverThreatRunning = false;
        // StatsGSBP
        memset(&(this->StatsGSBP), 0, sizeof(this->StatsGSBP));

        // buffer
        memset(this->TxBuffer, 0, sizeof(this->TxBuffer));
        this->TxBufferSize = 0;
    	this->UnclaimedRequestResponces = 0;
    }

    void GSBP_DD::SetDefaultExtConfiguration(void)
    {
    	this->ExtConfig.UpdateDeviceID = false;
    	sprintf(this->ExtConfig.DeviceID, "GSBP");
    	this->ExtConfig.UseThreadToRead = true;
    	this->ExtConfig.PackageHandler = NULL;
    	this->ExtConfig.GetErrorString = NULL;
    	this->ExtConfig.GetCmdString = NULL;
    	this->ExtConfig.NodeInfoCMD_ID = 1;
    	this->ExtConfig.NodeInfoACK_ID = 2;
    	this->ExtConfig.MessageACK_ID = 4;
    	this->ExtConfig.ApplicationDataACK_ID = 216;
    	this->ExtConfig.DisplayWarnings = true;
    	this->ExtConfig.DisplayErrors = true;
    }

    int GSBP_DD::OpenDevice()
    {
        struct termios ti;
        int fd, ret;

        if (access(this->DeviceFileName, R_OK|W_OK ) < 0 ) {
            // error with the serial interface -> exit
            return -1;
        }

        fd = open(this->DeviceFileName, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) {
            return -1;
        }
        // discard all not send or not read bytes
        tcflush(fd, TCIOFLUSH);

        /* Test if we can read the serial port device */
        ret = tcgetattr(fd, &ti);
        if (ret < 0)
        {
            close(fd);
            return -1;
        }

        /* Switch serial port to RAW mode */
        // Raw mode
        //
        // cfmakeraw() sets the terminal to something like the "raw" mode of the old Version 7 terminal driver:
        // input is available character by character, echoing is disabled, and all special processing of terminal input and output characters is disabled.
        // The terminal attributes are set as follows:
        // termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
        // termios_p->c_oflag &= ~OPOST;
        // termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        // termios_p->c_cflag &= ~(CSIZE | PARENB);
        // termios_p->c_cflag |= CS8;
        //memset(&ti, 0, sizeof(ti));
        cfmakeraw(&ti);
        ret = tcsetattr(fd, TCSANOW, &ti);
        if (ret < 0) {
            close(fd);
            return -1;
        }

        // ### http://linux.die.net/man/3/termios ###
        ret = cfsetospeed(&ti,(speed_t)GSBP__UART_BAUTRATE);
        ret = cfsetispeed(&ti,(speed_t)GSBP__UART_BAUTRATE);
        ret = cfsetspeed(&ti,GSBP__UART_BAUTRATE);
        ti.c_cflag |=  CS8 | CLOCAL | CREAD;     // Parity=0; Only one StopBit
        #if GSBP__UART_USE_UART_FLOW_CONTROL
        ti.c_cflag |= CRTSCTS;       // HW HandShake ON
        #else
        ti.c_cflag &= ~CRTSCTS;      // HW HandShake OFF
        #endif
        #if GSBP__USE_UART_TWO_STOP_BITS
        ti.c_cflag |= CSTOPB;        // 2 Stopbits   ON
        #else
        ti.c_cflag &= ~CSTOPB;       // 2 Stopbits   OFF
        #endif
        ti.c_iflag |= IGNBRK;
        ti.c_cc[VTIME] = 0;
        ti.c_cc[VMIN]  = 0;

        tcflush(fd, TCIOFLUSH);

        /* Set the new configuration */
        ret = tcsetattr(fd, TCSANOW, &ti);
        if (ret < 0) {
            close(fd);
            return -1;
        }
        return fd;
    }

    void GSBP_DD::AddRequest(RequestResponse_t item)
    {
#if GSBP__DEBUG_SENDING_COMMANDS
        item.CmdTime = boost::posix_time::microsec_clock::local_time();
#endif
#if GSBP__DEBUG_RECEIVING_COMMANDS
        item.AckTime = boost::posix_time::ptime();
#endif
        boost::mutex::scoped_lock lock(this->RequestResponseLock_mutex);
        GSBP_DD::RequestResponseBuffer.push_front(item);
        ++this->UnclaimedRequestResponces;
        lock.unlock();
    }

    bool GSBP_DD::ReadPackages(bool doReturnAfterTimeout)
    {
        // look this function
        boost::mutex::scoped_lock lock(this->ReadPackage_mutex);

        fd_set rfd;
        struct timeval TimeTimeout;
        unsigned char       RxBuffer[gsbp_RxMaxUserDataSize];
        size_t              RxBufferSize = 0;
        size_t              BytesToRead = 0;
        int                 BytesRead = 0;
        int sel;
        uint8_t ChecksumHeaderTemp = 0x00;

        bool                SearchStartByte = true;
        bool                ReadHeader = false;
        bool                ReadData   = false;
        bool                NewPackage = false;

        memset(RxBuffer, 0, sizeof(RxBuffer));
        FD_ZERO(&rfd);
        FD_SET(this->fd, &rfd);
        TimeTimeout.tv_sec = 0;
        TimeTimeout.tv_usec = gsbp_PackageReadTimoutUs;

        #if GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE
        convert16_t temp_datasize;
        #endif

        //   struct timeval TimeTimeout, time_start, time_stop;
        //gettimeofday(&time_start,NULL);
        //gettimeofday(&time_stop,NULL);
        //   u1 = time_start.tv_sec*1000 + time_start.tv_usec/1000;
        //   u2 = time_stop.tv_sec*1000 + time_stop.tv_usec/1000;
        //   *time_needed = (int)u2-u1;

        while(this->RunReceiverThread || doReturnAfterTimeout)
        {
            // TODO Watchdog??? TODO abbruch nach dem Einlesen aller Bytes von der Schnittstelle (don't use read threat)

            // wait that something is received and check if the TimeTimeout was reached
            FD_SET(this->fd, &rfd);
            TimeTimeout.tv_usec = gsbp_PackageReadTimoutUs;
            if ( (sel = select(this->fd+1, &rfd, NULL, NULL, &TimeTimeout)) == 0) {
                // timeout triggered -> check if the package was complete;
                if (!SearchStartByte){
                    // the package is incomplete; this should never happen
                    // build package from what we have so far
                    GSBP_DD::BuildPackage(RxBuffer, RxBufferSize, PackageIsBroken_IncompleteTimout, 0x00);
                    // reset the buffer for the next command
                    RxBufferSize = 0;
                    BytesToRead = 0;
                    SearchStartByte = true;
                    ReadHeader = false;
                    ReadData = false;
                }
                // wait for a byte or exit -> start at the beginning of the loop
                if (doReturnAfterTimeout){
                    // do not continuously read the serial interface; abort if there are no more bytes to read -> break
                    break;
                }
                continue;
            } else {
                // the timeout did not occur -> did an error occur?
                if (sel < 0){
                    printf("\e[1m\e[91m%s ERROR during package read:\e[0m Wait for a byte: %s (%d)\n", this->ID, strerror(errno), errno);
                    // wait for one or more bytes or exit -> start at the beginning of the loop
                    continue;
                }
            }

            // read the x bytes available
            if (SearchStartByte){
                // a new package starts -> read only one byte to search for the start byte
                BytesToRead = 1;
            }
            // check if the RxBuffer is large enough
            if ((RxBufferSize+BytesToRead) > gsbp_RxMaxPackageSize){
                // the RxBuffer is NOT large enough; this should never happen!
                printf("\e[1m\e[91m%s ERROR during package read:\e[0m RxBuffer is NOT large enough!\n   Requested buffer = %d bytes; available buffer = %d\n", this->ID, (int)(RxBufferSize+BytesToRead), gsbp_RxMaxPackageSize);
                // fill the buffer and let the BuildPackage function decide if the package is still usable
                BytesToRead = gsbp_RxMaxPackageSize - RxBufferSize;
                SearchStartByte = false;
                ReadHeader = false;
                ReadData = true;
            }
            // read n bytes
            BytesRead = read(this->fd, (void* ) &RxBuffer[RxBufferSize], BytesToRead);
            if (BytesRead < 0) {
                printf("\e[1m\e[91m%s ERROR during package read:\e[0m Read n bytes < 0: %s (%d)\n", this->ID, strerror(errno), errno);
                continue;
            }
            // we did read n bytes; set RxBufferSize to the next free value
            RxBufferSize += BytesRead;
            // degrease the BytesToRead counter
            BytesToRead -= BytesRead;
            // see if we are done for now?
            if (BytesToRead == 0) {
                // done to read a specific section -> start or header or data?
                if (SearchStartByte){
                    if (RxBuffer[0] == GSBP__UART_START_BYTE){
                        // a new package starts -> read the header first - the one start byte
                        BytesToRead = GSBP__UART_PACKAGE_HEADER_SIZE -1 ;
                        SearchStartByte = false;
                        ReadHeader = true;
                        ReadData = false;
                    } else {
                        // this was not the start byte, something went wrong -> at least log the error
                        this->StatsGSBP.BytesDiscarded++;
                        RxBufferSize = 0;
                    }
                }
                else if (ReadHeader) {
                    // reading the header is done -> make the checksum and get how many data bytes to read
                    // make the checksum without the start byte and the checksum (RxBufferSize-1)
#if GSBP__USE_CHECKSUMS
                    ChecksumHeaderTemp = GSBP__UART_HEADER_CHECKSUM_START;
                    for(uint32_t i=1; i<(RxBufferSize-1); i++){
                        ChecksumHeaderTemp ^= RxBuffer[i];
                        //TODO Checksumme ist nicht gut -> lieber one's sum ....
                        //http://betterembsw.blogspot.de/2010/05/which-error-detection-code-should-you.html
                    }
                    // check the checksum
                    if (RxBuffer[RxBufferSize-1] == ChecksumHeaderTemp) {
#endif
                        // checksum matches -> get the number of bytes to read next
                        #if GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE
                        //temp_datasize.data = 0;
                        //temp_datasize.c_data[1] = RxBuffer[GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE];
                        //temp_datasize.c_data[0] = RxBuffer[GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE+1];
                        //BytesToRead = temp_datasize.data;
						BytesToRead = ((uint16_t)RxBuffer[GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE+1] <<8) | (uint16_t)RxBuffer[GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE];
                        #else
                        BytesToRead = (size_t)RxBuffer[GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE];
                        #endif
                        if (BytesToRead > 0){
                            BytesToRead += GSBP__UART_PACKAGE_TAIL_SIZE;
                        } else {
                            // TODO TAIL Size unabhäning vom den Daten machen bzw. Checksumsize einführen
                            BytesToRead += 1;
                        }
                        SearchStartByte = false;
                        ReadHeader = false;
                        ReadData = true;
                        continue;
#if GSBP__USE_CHECKSUMS
                    } else {
                        // checksum does not match -> TODO what now? wait 10 us and flush the buffer?
                        printf("\e[1m\e[91m%s ERROR during package read:\e[0m Header checksum failed for ACK number %lu (%s (ID = 0x%02X)) (is: 0x%02X; should be: 0x%02X)\n", this->ID, (long unsigned int)GSBP_DD::GetCurrentRequestIdGlobal(isACK), GSBP_DD::ext_GetCommandString((command_t)RxBuffer[1]), RxBuffer[1], ChecksumHeaderTemp, RxBuffer[RxBufferSize-1]);
                        // update the statistics
                        this->StatsGSBP.NumberOfRxPackages_BrokenChecksum++;
                        // update the package counter because we receive a package, even if it is invalid
//                        GSBP_DD::GetRequestIdLocal(isACK, true);
                        // TODO: send "repeat command" command
                        //  wait 10 us and flush the buffer? there are data bytes availabe, which could contain a start_byte
                        RxBufferSize = 0;
                        BytesToRead = 0;
                        SearchStartByte = true;
                        ReadHeader = false;
                        ReadData = false;
                        continue;
                    }
#endif
                }
                else if (ReadData) {
                    // reading the data section is done -> build the package (check for the end byte later)
                    GSBP_DD::BuildPackage(RxBuffer, RxBufferSize-1, PackageIsOk, ChecksumHeaderTemp);
                    // reset the buffer for the next command
                    RxBufferSize = 0;
                    BytesToRead = 0;
                    SearchStartByte = true;
                    ReadHeader = false;
                    ReadData = false;
                    NewPackage = true;
                    continue;
                }
                else {
                    printf("\e[1m\e[91m%s ERROR during package read:\e[0m Neither ReadHeader / ReadData active\n", this->ID);
                }
            } // endif (BytesToRead == 0)
        }

        // UnLock the function
        lock.unlock();

        return NewPackage;  // return if called from same thread
    }


    void GSBP_DD::BuildPackage(uint8_t* RxBuffer, uint32_t RxBufferSize, packageState_t State, uint8_t ChecksumHeader)
    {
        size_t       RxBufferSizeCounter = 0;
        uint32_t     ChecksumDataTemp = 0;

        if (State != PackageIsOk) {
            // the package is broken -> see if we can salvage anything

            // check if measurment ack and if not send "repeate last package" command
            printf("\e[1m\e[91m%s ERROR during package build:\e[0m Package is broken (State = %d)...\n", this->ID, (int)State);
            return;
        }

        // create a new package for the queue -> allocate memory
        //package_t* Package = (package_t*) malloc(sizeof(package_t));               //TODO: remove
        rxPackage_t Package;

        // set default state
        Package.State = PackageIsBroken;
        do {
            // the header was ok -> get the header data
            if (RxBuffer[RxBufferSizeCounter++] != GSBP__UART_START_BYTE) {
                // TODO
                Package.State = PackageIsBroken_StartByteError;
                printf("\e[1m\e[91m%s ERROR during package build:\e[0m Startbyte does not match (0x%02X)\n", this->ID, RxBuffer[RxBufferSizeCounter-1]);
                // update the statistics
                this->StatsGSBP.NumberOfRxPackages_BrokenStructur++;
                // update the package counter because we receive a package, even if it is invalid
                break;
            }
            #if GSBP__ACTIVATE_DESTINATION_FEATURE
            Package->Destination = RxBuffer[RxBufferSizeCounter++];
            #endif
            #if GSBP__ACTIVATE_SOURCE_FEATURE
            Package->Source = RxBuffer[RxBufferSizeCounter++];
            #endif
            #if GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE
            RxBuffer[RxBufferSizeCounter++] = //TODO
            #endif
            #if GSBP__ACTIVATE_16BIT_CMD_FEATURE
            convert16_t temp_cmd;
            temp_cmd.data = 0;
            temp_cmd.c_data[1] = RxBuffer[RxBufferSizeCounter++];
            temp_cmd.c_data[0] = RxBuffer[RxBufferSizeCounter++];
            Package->CommandID = (uint16_t)temp_cmd.data;
            #else
            Package.CommandID = (uint16_t)RxBuffer[RxBufferSizeCounter++];
            #endif
            // check if CommandID is valid
/*            if (!GSBP_DD::ext_IsCommandIDValid((command_t)Package->CommandID)){
                Package->State = PackageIsBroken_InvalidCommandID;
                printf("\e[1m\e[91m%s ERROR during package build:\e[0m Invalid CommandID!!! (%02d|0x%02X|%s)\n",
                       this->ID, Package->CommandID, Package->CommandID, GSBP_DD::ext_GetCommandString((command_t)Package->CommandID));
                // update the statistics
                this->StatsGSBP.NumberOfRxPackages_BrokenStructur++;
                // update the package counter because we receive a package, even if it is invalid
                GSBP_DD::GetPackageNumberLocal(isACK, true);
                break;
            }
*/
            #if GSBP__ACTIVATE_CONTROL_FEATURE
            Package->Control = RxBuffer[RxBufferSizeCounter++];
            #endif
            Package.RequestID = RxBuffer[RxBufferSizeCounter++];

            #if GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE
            Package.DataSize = (uint16_t)RxBuffer[RxBufferSizeCounter] | ((uint16_t)RxBuffer[RxBufferSizeCounter +1] <<8);
            RxBufferSizeCounter += 2;
            #else
            Package->DataSize = (size_t)RxBuffer[RxBufferSizeCounter++];
            if (Package->DataSize > GSBP__UART_MAX_PACKAGE_SIZE){
                Package->State = PackageIsBroken_IncompleteData;
                printf("\e[1m\e[91m%s ERROR during package build:\e[0m DataSize is to large!!! (DataSize = %d | max = %d)\n",
                       this->ID, Package->DataSize, GSBP__UART_MAX_PACKAGE_SIZE);
                // update the statistics
                this->StatsGSBP.NumberOfRxPackages_BrokenStructur++;
                // update the package counter because we receive a package, even if it is invalid
                GSBP_DD::GetPackageNumberLocal(isACK, true);
                break;
            }
            #endif
#if GSBP__USE_CHECKSUMS
            //TODO check checksum again
            Package->ChecksumHeader = ChecksumHeader;
            RxBufferSizeCounter++;
#endif
            // check the data length
            if (RxBufferSize < RxBufferSizeCounter) {
                // the receive buffer is smaller as it should be -> ERROR
                // TODO
                Package.State = PackageIsBroken_IncompleteData;
                printf("\e[1m\e[91m%s ERROR during package build:\e[0m RxBufferSize is to small!!! (ByteSize - ByteSizeCounter = %d)\n",
                       this->ID, ((int)RxBufferSize - (int) RxBufferSizeCounter) );
                // update the statistics
                this->StatsGSBP.NumberOfRxPackages_BrokenStructur++;
                break;
            }
            else if (RxBufferSize == RxBufferSizeCounter) {
                // no payload?
                if (Package.DataSize != 0){
                    // there is a payload defined -> ERROR
                    // TODO
                    Package.State = PackageIsBroken_IncompleteData;
                    printf("\e[1m\e[91m%s ERROR during package build:\e[0m RxBufferSize is to small and there should be data!!! (ByteSize - ByteSizeCounter = %d)\n",
                           this->ID, ((int)RxBufferSize - (int) RxBufferSizeCounter) );
                    // update the statistics
                    this->StatsGSBP.NumberOfRxPackages_BrokenStructur++;
                    break;
                } else {
                    // no payload
                    if (RxBuffer[RxBufferSizeCounter] != GSBP__UART_END_BYTE){
                        // the last byte is not the UART end byte
                        Package.State = PackageIsBroken_EndByteError;
                        printf("\e[1m\e[91m%s ERROR during package build:\e[0m The last byte for package %u (local: %u) is not the expected End byte! -> package is discarded\n   BufferCounter: %lu; BufferSize: %lu; Endbyte package: 0x%02X;  Endbyte Buffer: 0x%02X\n",
                               this->ID, Package.CommandID, (unsigned int)Package.RequestID, (long unsigned int)RxBufferSizeCounter, (long unsigned int)RxBufferSize, RxBuffer[RxBufferSizeCounter], RxBuffer[RxBufferSize]);
                        // update the statistics
                        this->StatsGSBP.NumberOfRxPackages_BrokenStructur++;
                        break;
                    } else {
                        // ###  we are done. ###
                        Package.State = PackageIsOk;
                        break;
                    }
                }
            }
            else {
                // we have a payload -> check the length
                if (RxBufferSize != (RxBufferSizeCounter + Package.DataSize + GSBP__UART_PACKAGE_TAIL_SIZE -1)) {
                    // size does not match -> ERROR
                    // TODO
                    Package.State = PackageIsBroken_IncompleteData;
                    printf("\e[1m\e[91m%s ERROR during package build:\e[0m RxBuffer to small ??? (ByteSize = %d / ByteSizeCounter+DataSize+UART_Tail = %d) (DataSize = %d)\n",
                           this->ID, (int)RxBufferSize, (int)((RxBufferSizeCounter + Package.DataSize + GSBP__UART_PACKAGE_TAIL_SIZE)), (int)Package.DataSize );
                    // update the statistics
                    this->StatsGSBP.NumberOfRxPackages_BrokenStructur++;
                    break;
                } else
                {
                    // the size match -> copy the data
                    memcpy(Package.Data, &RxBuffer[RxBufferSizeCounter], Package.DataSize);
                	Package.Data[Package.DataSize] = 0x00; // make sure strings are properly terminated
                    // get the data checksum
                    // TODO
                    RxBufferSizeCounter += Package.DataSize;
#if GSBP__USE_CHECKSUMS
                    ChecksumDataTemp = RxBuffer[RxBufferSizeCounter++];
                    RxBufferSizeCounter+=3; // 4 byte checksum
#endif
                    // Check the END byte
                    if (RxBuffer[RxBufferSizeCounter] != GSBP__UART_END_BYTE){
                        // size does not macht -> ERROR
                        // TODO
                        Package.State = PackageIsBroken_EndByteError;
                        printf("\e[1m\e[91m%s ERROR during package build:\e[0m The last byte for package %u (local: %u) is not the expected End byte! -> package is discarded\n   BufferCounter: %u; BufferSize: %u; Endbyte package: 0x%02X;  Endbyte Buffer: 0x%02X\n",
                               this->ID, (uint32_t)Package.CommandID, (uint32_t)Package.RequestID, (uint32_t)RxBufferSizeCounter, (uint32_t)RxBufferSize, RxBuffer[RxBufferSizeCounter], RxBuffer[RxBufferSize]);
                        // update the statistics
                        this->StatsGSBP.NumberOfRxPackages_BrokenStructur++;
                        break;
                    } else {
                        // ###  we are done. ###
                        Package.State = PackageIsOk;
                        break;
                    }
                }
            }
        } while(false);

        // debug output
        #if GSBP__DEBUG_RECEIVING_COMMANDS
        	GSBP_DD::PrintPackage(&Package);
        #endif

        // add the package for the RequestResponce queue
        if (Package.State == PackageIsOk) {
        	this->StatsGSBP.NumberOfRxPackages++;
        	GSBP_DD::AddResponse(&Package);
        } else {
        	GSBP_DD::PrintPackage(&Package);
        }
    }


    uint64_t GSBP_DD::AddResponse(rxPackage_t* Response)
    {
    	/*
    	 * Check queue / Add response to queue
    	 */
    	// lock the queue
        boost::mutex::scoped_lock lock(this->RequestResponseLock_mutex);

        // check if there is a request for this response in the buffer and add it
        bool RequestFound = false;
        boost::circular_buffer<RequestResponse_t>::iterator Request = this->RequestResponseBuffer.begin();
        do {
        	// search for the last request with the same local request ID
        	for (Request = this->RequestResponseBuffer.begin();
        			Request != this->RequestResponseBuffer.end(); Request++){
        		// is the local request ID identical?
        		if (Request->RequestIdLocal == Response->RequestID){
        			// this is the last package received, with this local request ID
        			RequestFound = true;
        			break;
        		}
        	}
        	if (RequestFound){
        		// did this request already have an ACK?
        		if (Request->ResponseReceived){
        			// yes -> add a new dummy request
        			RequestResponse_t DummyRequest = *Request;
        			DummyRequest.IsDummyCopy = true;
        			DummyRequest.ResponseReceived = false;
        			DummyRequest.WaitForResponce = false;
        			DummyRequest.WaitTimedOut = false;
        			DummyRequest.Error = false;
        			lock.unlock();
        			GSBP_DD::AddRequest(DummyRequest);
        			lock.lock();
        			continue;
        		} else {
        			// no -> the response was expected -> add the response to the request
        			Request->ResponseReceived = true;
        			Request->Ack = *Response;
        			Request->Error = false;
        			Request->ErrorCode = 0;
#if GSBP__DEBUG_RECEIVING_COMMANDS
        			Request->AckTime = boost::posix_time::microsec_clock::local_time();
#endif
        			break;
        		}
        	}
        } while (RequestFound && Request->ResponseReceived);

        bool RemoveRequest = false;
        // check if this response is a message
        if (Response->CommandID == this->ExtConfig.MessageACK_ID){
          	gsbp_ACK_messageACK_t* data = (gsbp_ACK_messageACK_t*)Response->Data;
          	switch (data->msgType){
          	case MsgDebug:
          		RemoveRequest = true;
          		GSBP_DD::PackageHandler_Debug(Response);
          		break;
          	case MsgInfo:
          		GSBP_DD::PackageHandler_Debug(Response);
          		break;
          	case MsgError:
          	case MsgCriticalError:
          		GSBP_DD::PackageHandler_Error(Response);
          		if (RequestFound){
          			Request->Error = true;
          			Request->ErrorCode = data->errorCode;
          			memset(Request->ErrorDescription, 0, gsbp_ErrorStringSize);
          			memcpy(Request->ErrorDescription, data->msg, strlen((const char*)data->msg));
          		}
          		break;
          	case MsgWarning:
          		GSBP_DD::PackageHandler_Warning(Response);
          		break;
          	default:
          		 printf("\e[1m\e[91m%s ERROR during package post-processing:\e[0m MSG Invalid -> type %d, state %d, error %d\n   -> %s\n\n",
          				this->ID, data->msgType, data->state, data->errorCode, data->msg);
          	}
        }

        // call the external function to see if this package should be handled by the external implementation
        uint64_t RequestIDextern = 0; // 0 = no response to a CMD
        if (RequestFound){
        	RequestIDextern = Request->RequestIdGlobal;
        }
        if (GSBP_DD::ExtPackageHandler(Response, RequestIDextern)){
        	RemoveRequest = true;
        }

    	// post-processing
    	if (RequestFound){
			if (Request->WaitForResponce){
				// the response is waited for
			    if (Request->WaitTimedOut){
			    	// the response was waited for -> the timeout was triggered and the GetResponse function returned
			    	RemoveRequest = true;
			    }
			}

			if(RemoveRequest){
				Request->RequestIdLocal  = 0;
				Request->RequestIdGlobal = 0;
				this->UnclaimedRequestResponces--;
			}
			return Request->RequestIdGlobal;
    	} else {
#if GSBP__DEBUG_REQUEST_AND_RESPONSE_BUFFER
    		std::cout << this->ID << " AddResponce: Unrequested response received!!!" << std::endl;
    		GSBP_DD::PrintPackage(Response);
#endif
    	}

    	return 0;
    }


    bool  GSBP_DD::ExtPackageHandler(rxPackage_t* Package, uint64_t RequestId)
    {
    	if (this->ExtConfig.PackageHandler != NULL){
    		return this->ExtConfig.PackageHandler(Package, RequestId);
    	} else {
    		return false;
    	}
    }
    void GSBP_DD::PackageHandler_Debug(rxPackage_t* Package)
    {
    	if (Package->DataSize > 0){
    		// print Message
    		// make sure this qualifies as a string
    		Package->Data[Package->DataSize] = 0x00;
    		Package->Data[Package->DataSize+1] = 0x00;
    		gsbp_ACK_messageACK_t* data = (gsbp_ACK_messageACK_t*)Package->Data;
            printf("%s: Debug Package received (S:%d; EC:%d)\n   Message: %s\n\n",
            		this->ID, data->state, data->errorCode, data->msg);
        }
    }

    void GSBP_DD::PackageHandler_Warning(rxPackage_t* Package)
    {
    	if (this->ExtConfig.DisplayWarnings){
    		// check the error id
    		if (Package->DataSize > 0){
    			// print Message
    			// make sure this qualifies as a string
    			Package->Data[Package->DataSize] = 0x00;
    			Package->Data[Package->DataSize+1] = 0x00;
    			gsbp_ACK_messageACK_t* P = (gsbp_ACK_messageACK_t*)Package->Data;
    			printf("%s: WARNING received (S:%d; EC:%d)\n   Message: %s\n\n",
    					this->ID, P->state, P->errorCode, P->msg);
    		}
    	}
    }
    uint16_t GSBP_DD::PackageHandler_Error(rxPackage_t* Package)
    {
    	if (this->ExtConfig.DisplayErrors){
    		if (Package->DataSize > 0) {
    			// make sure this qualifies as a string
    			Package->Data[Package->DataSize] = 0x00;
    			Package->Data[Package->DataSize+1] = 0x00;
    			gsbp_ACK_messageACK_t* P = (gsbp_ACK_messageACK_t*)Package->Data;

    			// check the error id
    			if (P->errorCode == NoError){
    				// no error id set
    				printf("\e[1m\e[91m%s: Error package received\e[0m but there was not ErrorCode!\n   -> %s\n\n", this->ID, P->msg);
    				return NoError;
    			}

    			// the error id is valid
    			printf("\e[1m\e[91m%s: Error package received\e[0m after sending ID %d|0x%02X!\n   -> ErrorCode: \e[1m\e[91m%s\e[0m (ID: %d (0x%02X))\n",
    					this->ID, (uint16_t)this->RequestResponseBuffer[0].Cmd.CommandID, (uint8_t)this->RequestResponseBuffer[0].Cmd.CommandID, GSBP_DD::GetErrorString(P->errorCode), (uint8_t)P->errorCode, (uint8_t)P->errorCode);
    			printf("   -> Back trace: State=%d;\n", P->state);
    			printf("   -> Error MSG: \"\e[4m%s\e[24m\"\n\n", P->msg);
    			return P->errorCode;
    		}
    	}
        return NoError;
    }

    const char* GSBP_DD::GetCmdString(uint8_t Id)
    {
    	if (this->ExtConfig.GetCmdString != NULL){
    		return this->ExtConfig.GetCmdString(Id);
    	} else {
    		return "-";
    	}
    }

    const char* GSBP_DD::GetErrorString(uint16_t ErrorCode)
    {
    	if (ErrorCode < gsbp_MaxErrorCodeNumber){
            return GSBP_DD::GetGsbpErrorString(ErrorCode);
    	}

    	if (this->ExtConfig.GetErrorString != NULL){
    		return this->ExtConfig.GetErrorString(ErrorCode);
    	} else {
    		return "-";
    	}
    }


    /*
     * returns the next 8bit local request ID
     */
    uint8_t GSBP_DD::GetNextRequestIdLocal(void)
    {
        ++this->StatsGSBP.LocalTxRequestID;
        ++this->StatsGSBP.GlobalTxRequestID;

        if (this->StatsGSBP.LocalTxRequestID == 0 ||
        	this->StatsGSBP.LocalTxRequestID == 255){
        	// 255 and 0 are invalid as new local request IDs
        	this->StatsGSBP.LocalTxRequestID = 1;
        }
        return this->StatsGSBP.LocalTxRequestID;
    }

    /*
     * returns the current local request ID
     */
    uint8_t GSBP_DD::GetCurrentRequestIdLocal(void)
    {
        return this->StatsGSBP.LocalTxRequestID;
    }

    /*
     * returns the current global request ID
     */
    uint64_t GSBP_DD::GetCurrentRequestIdGlobal(void)
    {
    	return this->StatsGSBP.GlobalTxRequestID;
    }


    /* ### #########################################################################
     * Debug / Info functions
     * ### #########################################################################
     */

    void GSBP_DD::DoPrintNodeInfo(gsbp_ACK_nodeInfo_t* NodeInfo)
    {
    	std::cout << this->ID << " Device Information:" << std::endl;
    	std::cout << "   Board ID: " << NodeInfo->boardID << std::endl;
    	std::cout << "   Device Class: " << NodeInfo->deviceClass << std::endl;
    	std::cout << "   Serial Number: " << NodeInfo->serialNumber << std::endl;
    	std::cout << "   Version GSBP: [" << (uint32_t)NodeInfo->versionProtocol[0] << "][" << (uint32_t)NodeInfo->versionProtocol[1] << "]" << std::endl;
    	std::cout << "   Version Firmware: [" << (uint32_t)NodeInfo->versionFirmware[0] << "][" << (uint32_t)NodeInfo->versionFirmware[1] << "]" << std::endl;
    	if (strlen((const char*)NodeInfo->msg) > 0){
    		std::cout << "   Description: " << NodeInfo->msg << std::endl;
    	}
    	std::cout << std::endl;
    }

    void GSBP_DD::DoPrintPackageContent(txPackage_t* Package, uint8_t RequestIdLocal)
    {
        std::cout << "        -> ID number Local: " << (uint32_t)RequestIdLocal << std::endl;
        if ( Package->DataSize > 0 ) {
            std::cout << "        -> Data:" << std::endl << "           ";
            for(uint32_t i=0,j=0; i<Package->DataSize; i++, j++){
                printf("0x%02X ", Package->Data[i]);
                if (j==GSBP__DEBUG__PP_N_DATA_BYTES_PER_LINE){
                    printf("\n           ");
                    j=0;
                }
            }
            printf("\n");
        }
    }

    void GSBP_DD::DoPrintPackageContent(rxPackage_t* Package)
    {
        std::cout << "        -> ID number Local: " << (uint32_t)Package->RequestID << std::endl;
        if ( Package->DataSize > 0 ) {
            std::cout << "        -> Data:" << std::endl << "           ";
        	for(uint32_t i=0,j=0; i<Package->DataSize; i++, j++){
                printf("0x%02X ", Package->Data[i]);
                if (j==GSBP__DEBUG__PP_N_DATA_BYTES_PER_LINE){
                    printf("\n           ");
                    j=0;
                }
            }
            printf("\n");
        }
    }

    void GSBP_DD::DoPrintPackage(rxPackage_t* Package, bool IsACK)
    {
    	if (GSBP__DEBUG_RECEIVING_COMMANDS_EXEPT_MEAS_ACKS && (Package->CommandID == this->ExtConfig.ApplicationDataACK_ID)){
    		return;
    	}

        char CTyp[50], CTypfull[50], CState[100], COptions[100], CChecksumData[50], CColor[50];
        // prepare what to print
        if (IsACK){
            switch (Package->State){
                case PackageIsOk:                        sprintf(CState, "Ok"); break;
                case PackageIsBroken:                    sprintf(CState, "Broken"); break;
                case PackageIsBroken_EndByteError:       sprintf(CState, "Broken, because the end byte did not match"); break;
                case PackageIsBroken_IncompleteData:     sprintf(CState, "Broken, because the Data was incomplete"); break;
                case PackageIsBroken_IncompleteTimout:   sprintf(CState, "Broken, because not all bytes were transmitted in time"); break;
                default:                                 sprintf(CState, "Unknown state!");
            }
            sprintf(CTyp, "ACK");
            sprintf(CTypfull, "ACK -> State: %s", CState);
        } else {
        	sprintf(CTyp, "CMD");
        	sprintf(CTypfull, "CMD");
        }

        COptions[0] = 0x00;
        #if GSBP__ACTIVATE_DESTINATION_FEATURE || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE
        sprintf(&COptions[strlen(COptions)], "   Destination: %d\n", Package->Destination);
        #endif
        #if GSBP__ACTIVATE_SOURCE_FEATURE || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE
        sprintf(&COptions[strlen(COptions)], "   Source: %d\n", Package->Source);
        #endif
        #if GSBP__ACTIVATE_CONTROL_BYTE_FEATURE
        sprintf(&COptions[strlen(COptions)], "   Control Byte: 0x%02X\n", Package->Control);
        #endif
        // TODO Checksum
        sprintf(CChecksumData, "TODO");


        // print package contents
        if ( !IsACK ) {
        	printf("\e[36m");	// blue 34 // cyan 36
        	sprintf(CColor, "\e[36m");
        } else {
        	printf("\e[33m");	// yellow
        	sprintf(CColor, "\e[33m");
        }
        // get current time
        boost::posix_time::ptime Now = boost::posix_time::microsec_clock::local_time();
        auto NowString = boost::posix_time::to_simple_string(Now);

        printf("%s Debug Package \e[1m%s\e[0m%s (%s)\n   CommandID: %02d|0x%02X\n   Typ: %s\n%s   ID number Local: %u\n   Data Size: %d\n     ",
               this->ID, CTyp, CColor, NowString.c_str(), Package->CommandID, Package->CommandID, CTypfull, COptions, (unsigned int)Package->RequestID, (int)Package->DataSize );
        if ( Package->DataSize > 0 ) {
            uint32_t i,j=0;
            for(i=0; i<Package->DataSize; i++){
                printf("0x%02X ", Package->Data[i]);
                j++;
                if (j==GSBP__DEBUG__PP_N_DATA_BYTES_PER_LINE){
                    printf("\n     ");
                    j=0;
                }
            }
            printf("\n");
        }

        printf("\e[0m\n\n");
        fflush(stdout);
    }

    void GSBP_DD::DoPrintRequestResponse(RequestResponse_t* Request, bool AddOnlyAck, bool PrintPackageContent)
    {
    	boost::posix_time::time_duration CompletionPeriod;
    	if (!AddOnlyAck){
    		std::cout << "  -> Request " << Request->RequestIdGlobal_Debug << " (local: " << (uint32_t)Request->RequestIdLocal << ")";
    		if (Request->ResponseReceived){
#if GSBP__DEBUG_SENDING_COMMANDS
#if GSBP__DEBUG_RECEIVING_COMMANDS
    			CompletionPeriod = Request->AckTime - Request->CmdTime;
    			std::cout << " --> completed after " << CompletionPeriod.total_milliseconds() << "ms";
#endif
#endif
    			std::cout << std::endl;
    		} else {
    			std::cout << std::endl;
    		}

    		std::cout << "      -> CMD: " << GSBP_DD::GetCmdString(Request->Cmd.CommandID) << " (" << Request->Cmd.CommandID << ") -> data size: " << Request->Cmd.DataSize;
#if GSBP__DEBUG_SENDING_COMMANDS
    		std::cout << "  -> Send at: " << boost::posix_time::to_simple_string(Request->CmdTime);
#endif
    		std::cout << std::endl;
    		if (PrintPackageContent){
    			GSBP_DD::DoPrintPackageContent(&Request->Cmd, Request->RequestIdLocal_Debug);
    		}
    	}

    	if (Request->ResponseReceived){
        	std::cout << "      -> ACK: " << GSBP_DD::GetCmdString(Request->Ack.CommandID) << " (" << Request->Ack.CommandID << ") -> data size: " << Request->Ack.DataSize;
#if GSBP__DEBUG_RECEIVING_COMMANDS
        	std::cout << "  -> Send at: " << boost::posix_time::to_simple_string(Request->AckTime);
#endif
        	std::cout << std::endl;
    		if (PrintPackageContent){
    			GSBP_DD::DoPrintPackageContent(&Request->Ack);
    		}
    	} else {
    		std::cout << "      -> ACK was not received!";
    		if (Request->WaitForResponce)
#if GSBP__DEBUG_RECEIVING_COMMANDS
    		std::cout << " -> It was waited " << Request->AckWaitTimeout << "ms for the response.";
#endif
    		std::cout << std::endl;
    	}

    	if (Request->Error){
    		std::cout << "        -> An ERROR occurred: \e[1m\e[91mErrorCode\e[0m=" << Request->ErrorCode << std::endl;
    		std::cout << "           MSG: '" << Request->ErrorDescription << "'" << std::endl;
    	}
    }

void GSBP_DD::DoPrintBits(const void*  const ptr, const size_t size) {
    	unsigned char* b = (unsigned char*) ptr;
    	unsigned char byte = 0x00;

    	for (uint16_t i = 0; i < (uint16_t) size; i++) {
    		for (int8_t j = 7; j >= 0; j--) {
    			byte = b[i] & (1 << j);
    			byte >>= j;
    			if (j == 3) {
    				printf(" ");
    			}
    			printf("%u", byte);
    		}
    		printf("  ");
    	}
    }

} // end namespace
