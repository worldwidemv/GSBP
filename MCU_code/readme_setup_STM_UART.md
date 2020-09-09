# Setup Instructions for UART Interfaces used with the GSBP Module

## CubeMX Settings

In STM32CubeMX or STM32CubeIDE, the following settings must be set:

* under _Connectivity_ > _USARTx_  set `Mode` to `Asynchronous` (`Hardware Flow Control` is generally not necessary, but can be enabled if desired)
    * `Word Length` must be set to 8, but `Baud Rate`, `Parity`, and `Stop Bits` can be set according to your needs
    * set the `Advanced Parameters` / `Advanced Features` according to your needs
* if `GSBP_SETUP__UART_RX_METHOD` is set to 1 (interrupt based __receiving__), you need to enable the `USARTx global interrupt` in the `NVIC Settings` tab
* if `GSBP_SETUP__UART_RX_METHOD` is set to 2 (DMA based __receiving__), you need to:
    * Add the `USARTx_RX` DMA Request in the `DMA Settings` tab and set the desired priority.
    * Then, select the `USARTx_RX` entry and change the `Mode` to `Circular`.
    * Enable the `USARTx global interrupt` in the `NVIC Settings` tab.
* if `GSBP_SETUP__UART_TX_METHOD` is set to 1 (interrupt based __sending__), you need to enable the `USARTx global interrupt` in the `NVIC Settings` tab
* if `GSBP_SETUP__UART_TX_METHOD` is set to 2 (DMA based __sending__), you need to:
    * Add the `USARTx_TX` DMA Request in the `DMA Settings` tab and set the desired priority.
    * Enable the `USARTx global interrupt` in the `NVIC Settings` tab.

Interrupt priorities can be set under _System Core_ > _NVIC_.

Then generate the code. There are no code files to adjust if the UART interface is used.