# Setup Instructions for UART Interfaces used with the GSBP Module

## CubeMX Settings

In STM32CubeMX or STM32CubeIDE, the following settings must be set:

* under _Connectivity_ > _USB_  enable the USB device (e.g. `Device (FS)`), the default settings are fine
* under _Middleware_ > _USB_Device_ set `Class For FS IP` to `Communication Device Class (Virtual Port Com)`
* the default parameter are ok, however you can: 
    * set `USB CDC Xx Buffer Size' to 1, since those buffers will not be used
    * adjust the settings under `Device Descriptor` to your lickings
* activate the USB clock in the `Clock Configuration` tab

Then generate the code and adjust the generated file `usbd_cdc_if.c` .

## Necessary Changes to the File `usbd_cdc_if.c`

* add `#include "GSBP_Basic_Config.h"`  to line 26 between  
   `/* USER CODE BEGIN INCLUDE */` ... `/* USER CODE END INCLUDE */`  
* adapt `#define APP_RX_DATA_SIZE` and `#define APP_TX_DATA_SIZE`, line 68-69 to 4 since we will not use these buffers  
* edit `CDC_Init_FS`, User Code section 3 to
```C
      /* USER CODE BEGIN 3 */
         /* Set Application Buffers */
         /*
          * GSBP Buffer integration
          * -> use the GSBP RxBuffer directly
          */
         USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
         USBD_CDC_SetRxBuffer(&hUsbDeviceFS, GSBP_USB.RxBuffer); // use the GSBP buffer
         return (USBD_OK);
      /* USER CODE END 3 */
```
* change the `USER CODE BEGIN 6` section in function `CDC_Receive_FS()`
```C
      /* USER CODE BEGIN 6 */

         /*
          * GSBP Buffer integration
          * -> use the GSBP RxBuffer directly
          */
         // this is called from within an ISR, so use level 7, if DMA is used with the debug UART
         gsbpDebugMSG(7, "\nUSB RX: received %d bytes; SI %d\n", *Len, GSBP_USB.RxBufferSize);
         
         /* GSBP Implementation 1
          *  -> the GSBP_SaveBuffer function is called directly after the transfer
          *  ---> this might cause delays ....
          */
         /*
         GSBP_USB.RxBufferSize = *Len;
         GSBP_SaveBuffer(&GSBP_USB);
         // prepare the USB Endpoint for the next transfer
         USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &GSBP_USB.RxBuffer[0]);
         USBD_CDC_ReceivePacket(&hUsbDeviceFS);
         */
         
         /* GSBP Implementation 2
          *  -> the GSBP RxBuffer is filled up until it is almost full
          *  ---> no data is copied, there might be some data loss if the GSBP_SaveBuffer function resets the USB transfer while a transfer is currently active
          */
         
         GSBP_USB.RxBufferSize += *Len;
         if ((GSBP_USB.RxBufferSize +*Len) >= (GSBP_SETUP__RX_BUFFER_SIZE *0.8)){
         	GSBP_USB.RxBufferSize = *Len;
         	GSBP_SaveBuffer(&GSBP_USB);
         }
         GSBP_USB.State |= GSBP_HandleState__USBResetBuffer;
         USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &GSBP_USB.RxBuffer[GSBP_USB.RxBufferSize]);
         USBD_CDC_ReceivePacket(&hUsbDeviceFS);
         
         
         /* GSBP Implementation 2
          *  -> the GSBP RxBuffer is NOT used
          *  ---> the data is manually copied into the GSBP RxBuffer -> the CDC UserRxBuffer must be large enough
          */
         /*
         uint16_t i = 0;
         while((*Len > 0) && (GSBP_USB.RxBufferSize < GSBP_SETUP__RX_BUFFER_SIZE)){
         	GSBP_USB.RxBuffer[GSBP_USB.RxBufferSize] = Buf[i++];
         	GSBP_USB.RxBufferSize++;
         	(*Len)--;
         }
         USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
         USBD_CDC_ReceivePacket(&hUsbDeviceFS);
         */
         
         return (USBD_OK);
      /* USER CODE END 6 */
``` 
* select the prefered GSBP Implementation by commenting the others out (2 works good)
* for debugging, add to the `USER CODE BEGIN 7` section in function `CDC_Transmit_FS()`:
```C
      gsbpDebugMSG(7, "USB TX: send %d bytes\n\n", Len);
```
* add the custom CDC function around line 350 in the `USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION` section:
```C
      uint8_t CDC_GSBP_WaitUntilReadyToSend(uint32_t timeout)
      {
         USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
         while (hcdc->TxState != 0){
             if (HAL_GetTick() >= timeout){
                 return USBD_BUSY;
             }
         }
         return USBD_OK;
      }
```
* you might also have to change the function call to `SystemClock()` in `usbd_conf.c` if you have renamed the  function in `main.c`


