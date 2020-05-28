/** @file
  Serial I/O Port library functions with no library constructor/destructor


  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Device/SC8810Uart.h>

/*

  Initialize UART1 with 115200 baudrate, 8 data bits, no parity, 1 stop bit, no start bits.

  @return    Always return EFI_SUCCESS.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINT32    divider;
  UINT32    i = 0;
  UINT32    baudrate = 115200;

  while (!(MmioRead32(ARM_UART1_Base + ARM_UART_STS0) & BIT_15))
  {
     i++;
     if(i >= UART_SET_BAUDRATE_TIMEOUT)
     {
        return RETURN_DEVICE_ERROR;
     }
  }

  divider = ( (ARM_APB_CLK + baudrate / 2) / baudrate);


  /* Disable UART*/
  MmioWrite32(GR_CTRL_REG , (GR_CTRL_REG) & (~(GR_UART_CTRL_EN)));
  /*Disable Interrupt */
  MmioWrite32((ARM_UART1_Base + ARM_UART_IEN), 0);
  /* Enable UART*/
  MmioWrite32(GR_CTRL_REG, (GR_CTRL_REG) | (GR_UART_CTRL_EN));

  /* Set baud rate  */
  MmioWrite32((ARM_UART1_Base + ARM_UART_CLKD0), LWORD (divider));
  MmioWrite32((ARM_UART1_Base + ARM_UART_CLKD1), HWORD (divider));


  /* Set port for 8 bit, one stop, no parity  */
  MmioWrite32((ARM_UART1_Base + ARM_UART_CTL0), UARTCTL_BL8BITS | UARTCTL_SL1BITS);
  MmioWrite32((ARM_UART1_Base + ARM_UART_CTL1), 0);
  MmioWrite32((ARM_UART1_Base + ARM_UART_CTL2), 0);

//	/* clear input buffer */
//	if(serial_tstc())
//	  serial_getc();

  return RETURN_SUCCESS;
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes writed to serial device.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
)
{
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while (((MmioRead32(ARM_UART1_Base + ARM_UART_STS1) >> 8 ) & 0xFF) != 0);
    MmioWrite32(ARM_UART1_Base + ARM_UART_TXD, (*Buffer & 0xFF));
  }
  
  while (((MmioRead32(ARM_UART1_Base + ARM_UART_STS1) >> 8 ) & 0xFF) != 0);
  
  return NumberOfBytes;
}


/**
  Read data from serial device and save the datas in buffer.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Aactual number of bytes read from serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while (SerialPortPoll());
    *Buffer = MmioRead8(ARM_UART1_Base + ARM_UART_RXD) & 0xFF;
  }

  return NumberOfBytes;
}


/**
  Check to see if any data is avaiable to be read from the debug device.

  @retval EFI_SUCCESS       At least one byte of data is avaiable to be read
  @retval EFI_NOT_READY     No data is avaiable to be read
  @retval EFI_DEVICE_ERROR  The serial device is not functioning properly

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  if ((MmioRead32(ARM_UART1_Base + ARM_UART_STS1) & 0xFF) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Sets the control bits on a serial device.

  @param[in] Control            Sets the bits of Control that are settable.

  @retval RETURN_SUCCESS        The new control bits were set on the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32 Control
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Retrieve the status of the control bits on a serial device.

  @param[out] Control           A pointer to return the current control signals from the serial device.

  @retval RETURN_SUCCESS        The control bits were read from the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32 *Control
  )
{
  *Control = 0;
  if (!SerialPortPoll ()) {
    *Control = EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }
  return RETURN_SUCCESS;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receice time out, parity,
  data bits, and stop bits on a serial device.

  @param BaudRate           The requested baud rate. A BaudRate value of 0 will use the
                            device's default interface speed.
                            On output, the value actually set.
  @param ReveiveFifoDepth   The requested depth of the FIFO on the receive side of the
                            serial interface. A ReceiveFifoDepth value of 0 will use
                            the device's default FIFO depth.
                            On output, the value actually set.
  @param Timeout            The requested time out for a single character in microseconds.
                            This timeout applies to both the transmit and receive side of the
                            interface. A Timeout value of 0 will use the device's default time
                            out value.
                            On output, the value actually set.
  @param Parity             The type of parity to use on this serial device. A Parity value of
                            DefaultParity will use the device's default parity value.
                            On output, the value actually set.
  @param DataBits           The number of data bits to use on the serial device. A DataBits
                            vaule of 0 will use the device's default data bit setting.
                            On output, the value actually set.
  @param StopBits           The number of stop bits to use on this serial device. A StopBits
                            value of DefaultStopBits will use the device's default number of
                            stop bits.
                            On output, the value actually set.

  @retval RETURN_SUCCESS            The new attributes were set on the serial device.
  @retval RETURN_UNSUPPORTED        The serial device does not support this operation.
  @retval RETURN_INVALID_PARAMETER  One or more of the attributes has an unsupported value.
  @retval RETURN_DEVICE_ERROR       The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64             *BaudRate,
  IN OUT UINT32             *ReceiveFifoDepth,
  IN OUT UINT32             *Timeout,
  IN OUT EFI_PARITY_TYPE    *Parity,
  IN OUT UINT8              *DataBits,
  IN OUT EFI_STOP_BITS_TYPE *StopBits
  )
{
  return RETURN_UNSUPPORTED;
}

