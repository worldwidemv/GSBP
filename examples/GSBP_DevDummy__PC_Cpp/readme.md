# GSBP PC Project for the Nucleo Board based MCU Projects

This example shows the usage of the GSBP C++ interface class and how to create your own device interface class.


The used UART is USART2, which is connected to the ST-LinkV2 of the Nucleo board, which provides the UART<->USB converter function needed.

The debug UART is set to `huart1`, which means that an additional UART<->USB converter needs to be connected to pin `PA_9` and GND (see [connection diagram] 
