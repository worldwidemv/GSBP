# GSBP MCU Project for the Nucleo-L432KC Board with GSBP via UART

This example shows the usage of the GSBP module with an UART interface.  
The used UART is USART2, which is connected to the ST-LinkV2 of the Nucleo board, which provides the UART<->USB converter function needed.

The debug UART is set to `huart1`, which means that an additional UART<->USB converter needs to be connected to pin `PA_9` and GND (see [connection diagram](./nucleo_l432kc_UART.png)).

## Using the Project

The MCU project is a [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) project (Version 1.4.0).  
Since STM32CubeIDE is a cross platform application, you should be able to import and compile this project on any platform.

However, the development is done under Linux, so you might have to adjust the paths and so one.  
__I am also using Linux symlinks for the files `GSBP_Basic.h` and `GSBP_Basic.c`, so you might have to delete these symlinks and copy the files from `../MCU_code/` into the `App/` folder,__ if your setup does not find the files.

### Importing the Project

Importing is simply done, by going to the menu _File_ > _Import_ and selecting `Existing Project into Workspace`. Then just navigate to the location for this project. The import wizard should have found and selected project, so you can finish the import with the `Finish` button.

The project `GSBP_DevDumy__MCU_L432_UART` should now show up in your _Project Explorer_ and you should be able to compile the project. Double check the paths in the project properties, if you have any compile errors. See also the [general documentation](../../MCU_code/readme.md), section _Using the GSBP Module in Own Projects_ > _Setup_.

## PC Connection

The only connection needed is the USB connection to the Nucleo board. This will power the board and provide the virtual COM port used by the PC program (`COMx` (Windows) / `/dev/ttyACMx` (Linux)).

## Project Setup / Changes to the CubeMX/CubeIDE Exports

### Board Setup

The board was selected in STM32CubeMX / STM32CubeIDE with all peripherals initialised with there default modes.  
In addition to this default configuration of the Nucleo board, `PA_3`, `PA_4`, and `PA_5` were configured as `GPIO_Output` with the labels `D1`, `D2`, and `D3`.

The UART Interface was configured according to [the UART setup instructions](../../MCU_code/readme_setup_STM_UART.md). 
The `USART1` is used as debugging, so the interface was enabled and the `USART1_TX` DMA channel was enabled.  
The baud rate for both UART interfaces was set to 1000000 since this is the max speed my serial terminal (cutecom) supports (the cutecom setting is 998999 to be exact, which is close enough to work with a MCU setup of 1 Mbaud).

The system clock speed was then set to 80 MHz.

### GSBP Setup

In `GSBP_Basic_Config.h` the following changes where made:

* The device class ID was set to 1, which is my definition for the GSBP development dummy device.
* Only one GSBP handle was set, which is a UART interface (`UART_USED = 1`; `USB_USED = 0).
* The RX max payload is set to 100 byte and TX one is set to 1000.
* The `UART` specific settings and the _GSBP package structure_ settings were left on there defaults (DMA for sending and receiving). 
* The debugging level was set to 6 and the debug UART was set to `huart1`. The debug UART does use DMA since no `gsbpDebugMSG()` calls come from an ISR.
* The GSBP debug pins D1-3 were uncommented, so the timing of the GSBP functions can be viewed on D1-D3 with an logic analyser (see [general GSBP MCU documentation](../../MCU_code/readme.md), section _Debugging GSBP itself_).
* In the `main.h` the variable `uint8_t mcuState` was introduced and used in the custom implementation of the `GSBP_EvaluatePackage()` function.

In `GSBP_Basic_Config.c` the following changes where made:

* The UART handle was set to the default handle.
* The `GSBP_EvaluatePackage()` function was adjusted to the development dummy example workflow.

### Main Setup

In the `main.h` file, the definitions for a simple state machine were stored, as well as the state variable 'uint8_t mcuState`. The define `LED_PERIOD_MS` was also set there.

All other changes where done in the `main.c` file, where in `USER CODE BEGIN 2` section some dummy variables where created and the `GSBP_Init()` function is called. Inside of the main loop, the simple state machine of the example was implemented, as well as calls to the GSBP and LED toggle.


## Program Functionality

After the MCU starts, the GSBP is configured an starts waiting for commands from the PC. The program goes then into the main loop were:

* the LED is periodically toggled,
* the GSBP callback is periodically called, which evaluates any received package and might change the global `mcuState`,
* a simple state machine is run, with the following states:
    * `preInit` --> Do nothing wait state while waiting for the _Init_ command.
	* `doInit` --> Transition state, in which the initialisation is done, after the _Init_ command was received. The ACK is send in from this state, which proceeds to the `postInit` state after the initialisation is done. 
	* `postInit` --> Do nothing wait state while waiting for the _startMeasurement_ command.
	* `startMeasurement` --> Transition state, in which the dummy measurement is started. After the ACK is send, the state transitions to `measurementActive`.
	* `measurementActive` -->  Wait state while waiting for the _stopMeasurement_ command. During this state the dummy N measurements are send to the PC with frequency X, set with the initialisation command. The `int16_t` dummy measurements increase with Y (also a init parameter).
	* `stopMeasurement` --> Transition state, in which the dummy measurement is stopped. After the ACK is send, the state transitions to `postInit`.
	* `doDeInit` --> Transition state, in which the dummy device is deinitialised. After the ACK is send, the state transitions to `preInit`.
