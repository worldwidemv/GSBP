/*
 * # Stand Alone Program #
 *   a C++ program to test the communication interface, based on the
 *   GSBP interface class.
 *
 *   Copyright (C) 2015-2020 Markus Valtin <os@markus.valtin.net>
 *
 *   Author:  Markus Valtin
 *   File:    *.cpp -> Source file for the stand alone program
 *   Version: 1 (09.2020)
 *
 *   This file is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This file is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Copyright Header.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cerrno>

#include "DeviceInterface.hpp"

using namespace std;
using namespace nsDUMMYDEVICE_01;

#define N_DATA_VALUES_MAX			1000

// variables
bool doAbortProgram;
// prototypes
void abort_program(int sig);

// main program
int main ( int argc, char *argv[] )
{
	(void) signal(SIGINT, abort_program);

	char *DeviceName;
	char  NameTemp[100] = {0};
	FILE *fdData;

	int ValuesToGet = 0, ValuesReceived = 0;
	int WatchdogTime = 0;
	int StatusProzent = 0, StatusProzentOld = 0;
	struct timeval Time;
	long T0 = 0, T1 = 0, T2 = 0;

	if (argc < 3){ // at least 2 Arguments
		// We print argv[0] assuming it is the program name
		cout << "Usage: " << argv[0] << " <device name> <number of data values>\n";
		exit(-1);

		// Debug options
		char DeviceName2[100];
		sprintf(DeviceName2, "/dev/ttyACM1");
		DeviceName = DeviceName2;
		ValuesToGet = 100;
	} else {
		// We assume argv[1] is a filename to open
		DeviceName = argv[1];

		ValuesToGet = atoi(argv[2]);
		if ((ValuesToGet <= 0) | (ValuesToGet > 100000000)) {
			fprintf(stderr,	"Error: Number of values invalid! Must be a number between 1 and 100000000! (IS: \'%i\')\n", ValuesToGet);
			exit(-1);
		}
	}

	// output file
	sprintf(NameTemp, "Dummy_Data.csv");
	fdData = fopen(NameTemp, "w");
	if (fdData < NULL){
		fprintf(stderr,"Error: Output file \'%s\' can not be opened. \n\n", NameTemp);
		exit(-1);
	}

	// Main program
	// ##############################################################################################
	printf("Test Program for GeneralSerialByteProtocol Development\n==============================================\n");
	doAbortProgram = false;

	// INIT
	// Open the Device
	auto Device = new DummyDevice("DEVEL SAP", DeviceName);   // Create Device Class
	// START
	gettimeofday(&Time, NULL);
	T0 = Time.tv_sec * 1000 + Time.tv_usec / 1000;

	initCMD_t InitSetup = {0};
	InitSetup.dataPeriodMS = 10;	// 10ms -> 100 Hz
	InitSetup.dataSize = 20;		// 20 values each
	InitSetup.increment = 255;		// count up in steps of 255

	// Initialisation
	initACK_t Result = {0};
	if (!Device->InitialiseMCU(&InitSetup, &Result)) {
		// the initialisatzion failed -> abort
		// free memory
		delete Device;

		gettimeofday(&Time, NULL);
		T2 = Time.tv_sec * 1000 + Time.tv_usec / 1000;

		printf("\nAbort Program\nProgram runtime: %f sec\n\n", (T2 - T0) / 1000.0);
		exit(-1);
	}

	gettimeofday(&Time, NULL);
	T2 = Time.tv_sec * 1000.0 + Time.tv_usec / 1000.0;
	printf("\nInit took: %fms\n", (double) (T2 - T0));

	// get and print NodeInfo
	GSBP_DD::gsbp_ACK_nodeInfo_t NodeInfo = {0};
	Device->GetNodeInfo(&NodeInfo, true);

	/*
	 *  LOOP to get the dummy data
	 */
	// progress display preparations
	gettimeofday(&Time, NULL);
	T1 = Time.tv_sec * 1000 + Time.tv_usec / 1000;
	printf("\n   -->  0 %% done ....");
	fflush(stdout);

	// MCU status
	mcuStatus_t Status = {0};
	Device->GetStatus(&Status); // TODO: do something with this

	// dummy data
	int16_t DataValues[N_DATA_VALUES_MAX];
	uint16_t NumberOfValues = 0;

	// starting the dummy measurement
	Device->StartMCUApplication();

	while (ValuesReceived < (int) ValuesToGet && !doAbortProgram) {
		// current time
		gettimeofday(&Time,NULL);
		T2 = Time.tv_sec * 1000.0 + Time.tv_usec / 1000.0;

		// get dummy data
		NumberOfValues = Device->GetData(DataValues, N_DATA_VALUES_MAX);
		if (NumberOfValues > 0){
			for (uint16_t i=0; i<NumberOfValues; i++){
				ValuesReceived++;
				fprintf(fdData, "%+06d, %+5d\n", ValuesReceived, DataValues[i]);
			}
			WatchdogTime = 0;
		}

		// status info
		StatusProzent = (int) ((100.0 / (double) ValuesToGet) * (double) ValuesReceived);
		if (StatusProzent != StatusProzentOld) {
			printf("\r   --> % 2d %% done ....", StatusProzent);
			fflush(stdout);
			StatusProzentOld = StatusProzent;
		}

		usleep(10000); // 10ms
		WatchdogTime++;
		if (WatchdogTime > 200){
			// no new value was received in 2 sec -> abort
			printf("\n\e[1m\e[91mWatchdog was triggered:\e[0m no new value within 2 sec received! -> aborting\n\n");
			break;
		}
	}
	printf("\n\n");
	// END

	// stopping the dummy measurement
	Device->StopMCUApplication();

	// deinitialising the device
	Device->DeinitialiseMCU();

	// free memory
	delete Device;

	// close the files
	fflush(fdData);
	fclose(fdData);

	// end the program
	gettimeofday(&Time,NULL);
	T2 = Time.tv_sec*1000 + Time.tv_usec/1000;
	printf("\nProgram done\nProgram runtime: %f sec\n\n", (T2 -T1) / 1000.0);
	exit(0);
}

void abort_program(int sig) {
	doAbortProgram = true;
}
