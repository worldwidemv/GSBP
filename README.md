# The GeneralSerialByteProtocol - GSBP

__The GeneralSerialByteProtocol consists of:__

* __a definition for a package based communication protocol suitable for data streams like UART / virtual COM ports and so on,__
* __a C module for microcontrollers (mostly used/tested with STM32 microcontrollern) implementing this protocol and providing abstracted functions for sending and receiving data__,
* __ a C++ class for PC programs to send commands to connected devices and receive there data/responses.__


## Package Structure and Protocol Definition

The basic layout of the default __GSBP package__ looks like:

`|StartByte|CMD (low)|Request ID|N Bytes (low)|[N Bytes (high)]|[ Payload[0] ... Payload[N] ]|[EndByte]`


The complete definition of the protocol and its features is available in the __[documentation folder](./documentation/readme.md)__.

## C module for microcontrollers

The C module for microcontrollers can be found in the __[MCU_code folder](./MCU_code/readme.md)__.

The design goals for the GSBP MCU functions are:

* use of the hardware features (DMA) for blocking and non-blocking receiving and sending of data,
* easy integration into multiple projects while maintainable, so that new features and bugfixes are easily distributed to the different projects,
* abstract and universal implementation with user friendly API functions and workflow, and
* good performance and low overhead.

An examples on how to setup and use the module can be found in the [examples folder](./examples/readme.md) with the two MCU examples:

* [GSBP_DevDummy__MCU_L432_UART](../examples/GSBP_DevDummy__MCU_L432_UART/readme.md) which uses a UART interface and the UART<->USB feature of the Nucleo-L432KC board and
* [GSBP_DevDummy__MCU_L432_USB](../examples/GSBP_DevDummy__MCU_L432_USB/readme.md) which uses the USB interface of the STM32L432 MCU featured by the Nucleo-L432KC board.

## C++ Class for PC Programs

The class files for the general interface class can be found in the __[PC_code folder](./PC_code/readme.md)__.

The similar design goals are:

* easy integration into own projects while maintainable, so that new features and bugfixes are easily distributed to the different projects,
* abstract and universal implementation with user friendly API functions and workflow, and
* good performance and low overhead.

While the MCU functions can be used directly, the C++ class is not directly usable. Instead, the class should be used to implement the project specific interface class, where the different commands and the project specific workflow is implemented.

An example / reference implementation can be found in the [examples folder](./examples/GSBP_DevDummy__PC_Cpp/readme.md).

## TODOs / Known Bugs

* checksums do not work properly
* implement hardware based checksums on the STM32