/*
 * # Specific Device Interface #
 *   C++ Interface class for the GSBP_DevDummy device,
 *   utilizing the GSBP communication interface class.
 *
 *   Copyright (C) 2015-2020 Markus Valtin <os@markus.valtin.net>
 *
 *   Author:  Markus Valtin
 *   File:    DeviceInterface.cpp -> Source file for the device interface class
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
#include "DeviceInterface.hpp"

const uint32_t GSBP_DeviceClass__GSBPdevel		= 1;

namespace nsDUMMYDEVICE_01 {

DummyDevice::DummyDevice(const char *DeviceID, const char *SerialDeviceFile)
{
	/*
	 * Initialise the private variables
	 */
	// Device specific initialisations
	memset(this->DeviceIDClass, 0, sizeof(this->DeviceIDClass));
	if ( snprintf(this->DeviceIDClass, sizeof(this->DeviceIDClass), "%s", DeviceID) < 0){ // C++11
		// the device id could not be added, maybe it is to long? -> reset it to "DummyDevice"
		memset(  this->DeviceIDClass, 0, sizeof(this->DeviceIDClass));
		snprintf(this->DeviceIDClass, sizeof(this->DeviceIDClass), "%s", "DummyDevice");
	}
	memset(  this->DeviceFileName, 0, sizeof(this->DeviceFileName));
	if (snprintf(this->DeviceFileName, sizeof(this->DeviceFileName), "%s", SerialDeviceFile) < 0 ){ // C++11
		// the device filename could not be set up correctly, so opening the device will probably fail anyway -> error
		throw std::invalid_argument( "Serial device path is to long!" );
	}

	// set the internal variables
	this->DeviceInitialised = false;
	this->NumberOfDataValues = 0;

	// communication interface
	GSBP_DD::gsbpConfiguration_t Config = {0};
    Config.UpdateDeviceID = false;
	sprintf(Config.DeviceID, "GSBP");
	Config.UseThreadToRead = true;
	Config.PackageHandler = std::bind(&DummyDevice::PackageHandler, this, std::placeholders::_1, std::placeholders::_2);
	Config.GetErrorString = std::bind(&DummyDevice::GetErrorString, this, std::placeholders::_1);
	Config.GetCmdString   = std::bind(&DummyDevice::GetCommandIdString, this, std::placeholders::_1);
	Config.NodeInfoCMD_ID = (uint16_t)NodeInfoCMD;
	Config.NodeInfoACK_ID = (uint16_t)NodeInfoACK;
	Config.MessageACK_ID  = (uint16_t)MessageACK;
	Config.ApplicationDataACK_ID = (uint16_t)ApplicationDataACK;
	Config.DisplayWarnings = true;
	Config.DisplayErrors   = true;
	this->Interface = new GSBP_DD((char*)DeviceID, (char*)SerialDeviceFile, GSBP_DeviceClass__GSBPdevel, Config);
}

DummyDevice::~DummyDevice(void) {
	delete this->Interface;
}

bool DummyDevice::IsDeviceConnected(void)
{
	return this->Interface->IsDeviceConnected();
}

bool DummyDevice::GetNodeInfo(GSBP_DD::gsbp_ACK_nodeInfo_t *NodeInfo, bool PrintNodeInfo)
{
	uint16_t ErrorCode;
	return this->Interface->GetNodeInfo(NodeInfo, PrintNodeInfo, &ErrorCode);
}
bool DummyDevice::GetNodeInfo(GSBP_DD::gsbp_ACK_nodeInfo_t *NodeInfo, bool PrintNodeInfo, uint16_t *ErrorCode)
{
	return this->Interface->GetNodeInfo(NodeInfo, PrintNodeInfo, ErrorCode);
}

bool DummyDevice::InitialiseMCU(initCMD_t *InitConfig, initACK_t *InitResponce)
{
	uint16_t ErrorCode;
	return DummyDevice::InitialiseMCU(InitConfig, InitResponce, &ErrorCode);
}
bool DummyDevice::InitialiseMCU(initCMD_t *InitConfig, initACK_t *InitResponce, uint16_t* ErrorCode)
{
	*ErrorCode = GSBP_DD::NoError;
	this->DeviceInitialised = false;

	// send the command
	GSBP_DD::txPackage_t P = {0};
	P.CommandID = InitCMD;
	memcpy(P.Data, InitConfig, sizeof(initCMD_t));
	P.DataSize = sizeof(initCMD_t);
	uint64_t RequestID = this->Interface->SendPackage(&P, ErrorCode);
	if (RequestID == GSBP_DD::InvalidRequestID){
		// package was not send
		// TODO ErrorHandler
		InitResponce->success = false;
		return false;
	}

	// wait for the response
	GSBP_DD::rxPackage_t Ack = {0};
	uint32_t NumberOfOpenRequests = 0;
	if (this->Interface->GetResponse(RequestID, InitACK, &Ack, 500, &NumberOfOpenRequests, ErrorCode)){
		// TODO check for OpenRequests...
		memset(InitResponce, 0, sizeof(initACK_t));
		memcpy(InitResponce, Ack.Data, sizeof(initACK_t));
		initACK_t *ack = (initACK_t*)Ack.Data;
		if (ack->success){
			InitResponce->success = true;
			InitResponce->dataPeriodMS = ack->dataPeriodMS;
			InitResponce->dataSize = ack->dataSize;
			InitResponce->increment = ack->increment;
			this->DeviceInitialised = true;
			return true;
		} else {
			return false;
		}
	} else {
		// TODO check for OpenRequests...
		InitResponce->success = false;
		return false;
	}
}


bool DummyDevice::StartMCUApplication(void)
{
	uint16_t ErrorCode;
	return DummyDevice::StartMCUApplication(&ErrorCode);
}
bool DummyDevice::StartMCUApplication(uint16_t* ErrorCode)
{
	*ErrorCode = GSBP_DD::NoError;

	// send the command
	GSBP_DD::txPackage_t P = {0};
	P.CommandID = StartApplicationCMD;
	P.DataSize = 0;
	uint64_t RequestID = this->Interface->SendPackage(&P, ErrorCode);
    if (RequestID == GSBP_DD::InvalidRequestID){
    	// package was not send
    	// TODO ErrorHandler
    	return false;
    }

    // wait for the response
	GSBP_DD::rxPackage_t Ack = {0};
	uint32_t NumberOfOpenRequests = 0;
	if (this->Interface->GetResponse(RequestID, UniversalACK, &Ack, 500, &NumberOfOpenRequests, ErrorCode)){
		// TODO check for OpenRequests...
		return true;
	} else {
		// TODO check for OpenRequests...
		return false;
	}
}

uint16_t DummyDevice::GetData(int16_t Data[], uint16_t NumberOfValuesMax)
{
	uint16_t NumberOfValuesToGet = NumberOfValuesMax;
	if (NumberOfValuesToGet > this->NumberOfDataValues){
		NumberOfValuesToGet = this->NumberOfDataValues;
	}

	// look the buffer
	boost::mutex::scoped_lock lock(this->DataMutex);

	// copy the data to the external memory
	for (uint16_t i=0; i<NumberOfValuesToGet; i++){
		Data[i] = this->DataValues[i];
	}
	// move the remaining data to the start of the buffer
	for (uint16_t i=0; i<(this->NumberOfDataValues -NumberOfValuesToGet); i++){
		this->DataValues[i] = this->DataValues[i +NumberOfValuesToGet];
	}
	// update the data counter
	this->NumberOfDataValues -= NumberOfValuesToGet;

	// unlook the buffer
	lock.unlock();

	return NumberOfValuesToGet;
}

bool DummyDevice::StopMCUApplication(void)
{
	uint16_t ErrorCode;
	return DummyDevice::StopMCUApplication(&ErrorCode);
}
bool DummyDevice::StopMCUApplication(uint16_t* ErrorCode)
{
	*ErrorCode = GSBP_DD::NoError;

	// send the command
    GSBP_DD::txPackage_t P = {0};
    P.CommandID = StopApplicationCMD;
    P.DataSize = 0;
    uint64_t RequestID = this->Interface->SendPackage(&P, ErrorCode);
    if (RequestID == GSBP_DD::InvalidRequestID){
    	// package was not send
    	// TODO ErrorHandler
    	return false;
    }

    // wait for the response
	GSBP_DD::rxPackage_t Ack = {0};
	uint32_t NumberOfOpenRequests = 0;
	if (this->Interface->GetResponse(RequestID, UniversalACK, &Ack, 500, &NumberOfOpenRequests, ErrorCode)){
		// TODO check for OpenRequests...
		return true;
	} else {
		// TODO check for OpenRequests...
		return false;
   	}
}

bool DummyDevice::DeinitialiseMCU(void)
{
	uint16_t ErrorCode;
	return DummyDevice::DeinitialiseMCU(&ErrorCode);
}
bool DummyDevice::DeinitialiseMCU(uint16_t* ErrorCode)
{
	*ErrorCode = GSBP_DD::NoError;

	// send the command
    GSBP_DD::txPackage_t P = {0};
    P.CommandID = DeInitCMD;
    P.DataSize = 0;
    uint64_t RequestID = this->Interface->SendPackage(&P, ErrorCode);
    if (RequestID == GSBP_DD::InvalidRequestID){
    	// package was not send
    	// TODO ErrorHandler
    	return false;
    }

    // wait for the response
	GSBP_DD::rxPackage_t Ack = {0};
	uint32_t NumberOfOpenRequests = 0;
	if (this->Interface->GetResponse(RequestID, UniversalACK, &Ack, 1000, &NumberOfOpenRequests, ErrorCode)){
		// TODO check for OpenRequests...
		// done
		this->DeviceInitialised = false;
		return true;
	} else {
		// TODO check for OpenRequests...
		return false;
   	}
}


bool DummyDevice::GetStatus(mcuStatus_t *Status)
{
	uint16_t ErrorCode;
	return DummyDevice::GetStatus(Status, &ErrorCode);
}
bool DummyDevice::GetStatus(mcuStatus_t *Status, uint16_t *ErrorCode)
{
	*ErrorCode = GSBP_DD::NoError;

	// send the command
    GSBP_DD::txPackage_t P = {0};
    P.CommandID = StatusCMD;
    P.DataSize = 0;
    uint64_t RequestID = this->Interface->SendPackage(&P, ErrorCode);
    if (RequestID == GSBP_DD::InvalidRequestID){
    	// package was not send
    	// TODO ErrorHandler
    	return false;
    }

    // wait for the response
	GSBP_DD::rxPackage_t Ack = {0};
	uint32_t NumberOfOpenRequests = 0;
	if (this->Interface->GetResponse(RequestID, StatusACK, &Ack, 1000, &NumberOfOpenRequests, ErrorCode)){
		// TODO check for OpenRequests...
		// done
		mcuStatus_t *ack = (mcuStatus_t*)Ack.Data;
		Status->state = ack->state;
		Status->errorCode = ack->errorCode;
		memcpy(Status->msg, ack->msg, DUMMYDEVICE_STATUS_MSG_SIZE_MAX);
		return true;
	} else {
		// TODO check for OpenRequests...
		return false;
   	}
}


bool DummyDevice::PackageHandler(GSBP_DD::rxPackage_t *Package, uint64_t RequestID)
{
	//printf("%s ReceiveACK Debug: PackageHandler invoked for Response %lu -> ID: %u\n",
	//	this->DeviceIDClass, RequestID, Package->CommandID);
	//this->Interface->PrintPackage(Package);

	switch (Package->CommandID){
	case ApplicationDataACK:{ // Dummy Measurement Data
		// copy the ACK data to the buffer ...

		// get the ACK data
		measurementDataACK_t *ack = (measurementDataACK_t*) Package->Data;
		// look the buffer
		boost::mutex::scoped_lock lock(this->DataMutex);
		// copy the ACK data to the buffer
		for (uint16_t i=0; i<ack->numberOfValues; i++){
				this->DataValues[this->NumberOfDataValues++] = ack->data[i];
		}
		// unlook the buffer
		lock.unlock();

		return true;
		} break;
	default: // do nothing
		break;
	}
	return false;
}

const char*	DummyDevice::GetCommandIdString(uint16_t Cmd)
{
	return "CMD string TODO";
}
const char*	DummyDevice::GetErrorString(uint16_t ErrorCode)
{
	return "Error string TODO";
}


} // namespace

