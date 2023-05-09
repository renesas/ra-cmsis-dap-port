/***********************************************************************************************************************
 * File Name    : usb_composite.h
 * Description  : Contains macros, data structures and function declaration used in EP
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/
#ifndef USB_COMPOSITE_H_
#define USB_COMPOSITE_H_

#define LINE_CODING_LENGTH (0x07U) // Line coding length
#define CDC_DATA_LEN (64U)
#define UART_PACKET_COUNT 255 

/* Macro definitions */
#define SCI_UART_INVALID_16BIT_PARAM   (0xFFFFU)
#define BAUD_ERROR_RATE                (5000U)
#define INVALID_SIZE                   (0U)
#define INTERFACE_PCDC_FIRST 0x0
#define INTERFACE_CMSIS_DAP 0x2

// Extended Compat ID Descriptor Format
typedef __PACKED_STRUCT 
{
    uint8_t bFirstInterfaceNumber; // The interface or function number
    uint8_t RESERVED1[1];          // Reserved for system use. Set this value to 0x01
    uint8_t compatibleID[8];       // The function’s compatible ID
    uint8_t subCompatibleID[8];    // The function’s subcompatible ID
    uint8_t RESERVED2[6];          // Reserved
} tyECIDDescFunctionSection;


typedef __PACKED_STRUCT 
{
    uint32_t dwLength;      // The length, in bytes, of the complete extended compat ID descriptor
    uint16_t bcdVersion;    // The descriptor’s version number, in binary coded decimal (BCD) format
    uint16_t wIndex;        // An index that identifies the particular OS feature descriptor
    uint8_t  bCount;        // The number of custom property sections
    uint8_t  RESERVED[7];  // Reserved . Fill this value with NULLs.

} tyExtendedCompatIDDescriptor;

typedef __PACKED_STRUCT 
{
    tyExtendedCompatIDDescriptor hdr;
    tyECIDDescFunctionSection    pvnd;
    tyECIDDescFunctionSection    pcdc;
} tyRAM4ECID;


#define MS_VENDOR_CODE_CMSIS_DAP 0x00 

// Extended compat ID OS descriptor.
#define EXT_COMPATID_OS_DESCRIPTOR 0x04
// Extended Properties OS Descriptor
#define EXT_PROP_OS_DESCRIPTOR 0x05
#define USB_VENDOR_GET_MS_DESCRIPTOR_DEVICE      ((MS_VENDOR_CODE_CMSIS_DAP << 8) | USB_DEV_TO_HOST | USB_VENDOR | USB_DEVICE)
#define USB_VENDOR_GET_MS_DESCRIPTOR_INTERFACE   ((MS_VENDOR_CODE_CMSIS_DAP << 8) | USB_DEV_TO_HOST | USB_VENDOR | USB_INTERFACE)


#endif /* USB_COMPOSITE_H_ */
