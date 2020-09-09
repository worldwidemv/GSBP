# Example Implementations for the GSBP

This folder contains the MCU and PC examples for the GeneralSerialByteProtocol.

The PC example is generally designed to work with the MCU examples and implement a simple dummy device, which sends dummy measurement data with a configured measurement frequency. This data is received by the PC program and stored in a CSV file. Scilab and Matlab scripts are provided to plot the [dummy data](./DummyData_Example.jpg).

The two MCU examples are:

* [GSBP_DevDumy__MCU_L432_UART](../examples/GSBP_DevDumy__MCU_L432_UART/readme.md) which uses a UART interface and the UART<->USB feature of the Nucleo-L432KC board and
* [GSBP_DevDumy__MCU_L432_USB](../examples/GSBP_DevDumy__MCU_L432_USB/readme.md) which uses the USB interface of the STM32L432 MCU featured by the Nucleo-L432KC board.

Both MCU examples work with the PC example because they implement the same dummy device.

## Executing the Examples

The MCU projects are [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) projects (Version 1.4.0).  
The PC C++ project can also be opened and compiled with [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html), since the CubeIDE is based in Eclipse an therefore supports standard C/C++ projects.  
Since STM32CubeIDE is a cross platform application, you should be able to import and compile this project on any platform.

However, the development is done under Linux, so you might have to adjust the paths and so one.  
__I am also using Linux symlinks for files, which are not project specific, which might not work on your platform!__

### Example Execution for the MCU Example `GSBP_DevDumy__MCU_L432_UART`

* import and compile the MCU example `GSBP_DevDumy__MCU_L432_UART`
* program the Nucleo-L432KC board with the generated executable
* import and compile the MCU example `GSBP_DevDumy__PC_Cpp`
* go into the folder `GSBP_DevDumy__PC_Cpp/Debug/`
* run the program `./DevSAP`  with the option `/dev/ttyACM0 1000` (Linux)  
* you should end up with a similar output:
```C
$ ./DevSAP /dev/ttyACM0 1000
Test Program for GeneralSerialByteProtocol Development
==============================================

Init took: 196.000000ms
DEVEL SAP Device Information:
   Board ID: 0
   Device Class: 1
   Serial Number: 1904010001
   Version GSBP: [0][1]
   Version Firmware: [0][1]
   Description: GSBP: NODE DESCRIPTION -> TODO


   -->  100 % done ....


DEVEL SAP GSBP Statistics:
   Packages received = 65 (missing: 0 | broken checksum: 0 | broken structure: 0 | bytes discarded: 0)
   Packages send = 7


Program done
Program runtime: 1.256000 sec
```
* have a look at the file `Dummy_Data.csv` for the received MCU dummy measurement data
* change the init configuration in `GSBP_DevDumy__PC_Cpp/src/StandAloneProgram.cpp` and see how the dummy measurement data changes ...
