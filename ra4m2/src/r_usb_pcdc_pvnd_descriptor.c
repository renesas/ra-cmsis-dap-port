/* ${REA_DISCLAIMER_PLACEHOLDER} */
/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/
#include "r_usb_basic.h"
#include "r_usb_basic_api.h"
#include "usb_composite.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/
/* bcdUSB */
#define USB_BCDNUM (0x0200U)
/* Release Number */
#define USB_RELEASE (0x0200U)
/* DCP max packet size */
#define USB_DCPMAXP (64U)
/* Configuration number */
#define USB_CONFIGNUM (1U)
/* Vendor ID */
#define USB_VENDORID (0x045BU)
/* Product ID */
#define USB_PRODUCTID (0x0201FU)

/* Miscellaneous Device Class */
#define USB_MISC_CLASS (0xEF)
/* Common Class */
#define USB_COMMON_CLASS (0x02)
/* Interface Association Descriptor */
#define USB_IAD_DESC (0x01)
/* Interface Association Descriptor Type */
#define USB_IAD_TYPE (0x0B)

/* Class-Specific Configuration Descriptors */
#define USB_PCDC_CS_INTERFACE (0x24U)

/* bDescriptor SubType in Communications Class Functional Descriptors */
/* Header Functional Descriptor */
#define USB_PCDC_DT_SUBTYPE_HEADER_FUNC (0x00U)
/* Call Management Functional Descriptor. */
#define USB_PCDC_DT_SUBTYPE_CALL_MANAGE_FUNC (0x01U)
/* Abstract Control Management Functional Descriptor. */
#define USB_PCDC_DT_SUBTYPE_ABSTRACT_CTR_MANAGE_FUNC (0x02U)
/* Union Functional Descriptor */
#define USB_PCDC_DT_SUBTYPE_UNION_FUNC (0x06U)

/* Communications Class Subclass Codes */
#define USB_PCDC_CLASS_SUBCLASS_CODE_ABS_CTR_MDL (0x02U)

/* USB Class Definitions for Communications Devices Specification
 release number in binary-coded decimal. */
#define USB_PCDC_BCD_CDC (0x0110U)

/* Descriptor length */
#define USB_DQD_LEN (10U)
#define USB_DD_BLENGTH (18U)
#define USB_PCDC_PVND_CD_LEN (105U)
#define STRING_DESCRIPTOR0_LEN (4U)
#define STRING_DESCRIPTOR1_LEN (16U)
#define STRING_DESCRIPTOR2_LEN (44U)
#define STRING_DESCRIPTOR3_LEN (32U)
#define STRING_DESCRIPTOR4_LEN (22U)
#define STRING_DESCRIPTOR5_LEN (18U)
#define STRING_DESCRIPTOR6_LEN (28U)
#define STRING_DESCRIPTOR7_LEN (22U)
#define STRING_MSFT100_LEN     (18U)
#define NUM_STRING_DESCRIPTOR  (0xEF)

/* Descriptor data Mask */
#define USB_UCHAR_MAX (0xffU)
#define USB_W_TOTAL_LENGTH_MASK (256U)
#define USB_W_MAX_PACKET_SIZE_MASK (64U)
#define USB_PCDC_BCD_CDC_MASK (256U)

/* Definitions for Vendor Class */
#define USB_VALUE_256                 (256U)
#define USB_VALUE_FFH                 (0xFFU)
#define USB_VENDOR_CODE               (USB_VALUE_FFH)
#define USB_MXPS_BULK_FULL            (64U)
#define USB_MXPS_BULK_HI              (512U)
#define USB_MXPS_INT                  (64U)

/* Standard Device Descriptor */
uint8_t g_apl_device[USB_DD_BLENGTH + (USB_DD_BLENGTH % 2)] =
    {
        USB_DD_BLENGTH,                                           /*  0:bLength */
        USB_DT_DEVICE,                                            /*  1:bDescriptorType */
        (USB_BCDNUM &(uint8_t)USB_UCHAR_MAX),                     /*  2:bcdUSB_lo */
        ((uint8_t)(USB_BCDNUM >> 8) & (uint8_t)USB_UCHAR_MAX),    /*  3:bcdUSB_hi */
        USB_MISC_CLASS,                                           /*  4:bDeviceClass */
        USB_COMMON_CLASS,                                         /*  5:bDeviceSubClass */
        USB_IAD_DESC,                                             /*  6:bDeviceProtocol */
        (uint8_t)USB_DCPMAXP,                                     /*  7:bMAXPacketSize(for DCP) */
        (USB_VENDORID &(uint8_t)USB_UCHAR_MAX),                   /*  8:idVendor_lo */
        ((uint8_t)(USB_VENDORID >> 8) & (uint8_t)USB_UCHAR_MAX),  /*  9:idVendor_hi */
        ((uint16_t)USB_PRODUCTID &(uint8_t)USB_UCHAR_MAX),        /* 10:idProduct_lo */
        ((uint8_t)(USB_PRODUCTID >> 8) & (uint8_t)USB_UCHAR_MAX), /* 11:idProduct_hi */
        (USB_RELEASE &(uint8_t)USB_UCHAR_MAX),                    /* 12:bcdDevice_lo */
        ((uint8_t)(USB_RELEASE >> 8) & (uint8_t)USB_UCHAR_MAX),   /* 13:bcdDevice_hi */
        1,                                                        /* 14:iManufacturer */
        2,                                                        /* 15:iProduct */
        6,                                                        /* 16:iSerialNumber */
        USB_CONFIGNUM                                             /* 17:bNumConfigurations */
};

/************************************************************
 *  Device Qualifier Descriptor   *
 ************************************************************/
uint8_t g_apl_qualifier_descriptor[USB_DQD_LEN + (USB_DQD_LEN % 2)] =
    {
        USB_DQD_LEN,                                           /*  0:bLength */
        USB_DT_DEVICE_QUALIFIER,                               /*  1:bDescriptorType */
        (USB_BCDNUM &(uint8_t)USB_UCHAR_MAX),                  /*  2:bcdUSB_lo */
        ((uint8_t)(USB_BCDNUM >> 8) & (uint8_t)USB_UCHAR_MAX), /*  3:bcdUSB_hi */
        0,                                                     /*  4:bDeviceClass */
        0,                                                     /*  5:bDeviceSubClass */
        0,                                                     /*  6:bDeviceProtocol */
        (uint8_t)USB_DCPMAXP,                                  /*  7:bMAXPacketSize(for DCP) */
        USB_CONFIGNUM,                                         /*  8:bNumConfigurations */
        0                                                      /*  9:bReserved */
};

/************************************************************
 *  Configuration Or Other_Speed_Configuration Descriptor   *
 ************************************************************/
/* For Full-Speed */
uint8_t g_apl_configuration[USB_PCDC_PVND_CD_LEN + (USB_PCDC_PVND_CD_LEN % 2)] =
    {
        USB_CD_BLENGTH,                                /*  0:bLength */
        USB_SOFT_CHANGE,                               /*  1:bDescriptorType */
        USB_PCDC_PVND_CD_LEN % USB_W_TOTAL_LENGTH_MASK,/*  2:wTotalLength(L) */
        USB_PCDC_PVND_CD_LEN / USB_W_TOTAL_LENGTH_MASK,/*  3:wTotalLength(H) */
        3,                                             /*  4:bNumInterfaces */
        1,                                             /*  5:bConfigurationValue */
        0,                                             /*  6:iConfiguration */
        USB_CF_RESERVED | USB_CF_BUSP,                 /*  7:bmAttributes */
        (500 / 2),                                     /*  8:MAXPower (2mA unit) */


        /* Communication Device Class */

        /* Interface Association Descriptor (IAD) */
        0x08,                                     /*  0:bLength */
        USB_IAD_TYPE,                             /*  1:bDescriptorType */
        INTERFACE_PCDC_FIRST,                     /*  2:bFirstInterface */
        0x02,                                     /*  3:bInterfaceCount */
        USB_IFCLS_CDCC,                           /*  4:bFunctionClass  */
        USB_PCDC_CLASS_SUBCLASS_CODE_ABS_CTR_MDL, /* 5:bFunctionSubClass */
        0x01,                                     /*  6:bFunctionProtocol */
        0x00,                                     /*  7:iFunction */

        /* Interface Descriptor */
        USB_ID_BLENGTH,                           /*  0:bLength */
        USB_DT_INTERFACE,                         /*  1:bDescriptor */
        INTERFACE_PCDC_FIRST,                     /*  2:bInterfaceNumber */
        0,                                        /*  3:bAlternateSetting */
        1,                                        /*  4:bNumEndpoints */
        USB_IFCLS_CDCC,                           /*  5:bInterfaceClass */
        USB_PCDC_CLASS_SUBCLASS_CODE_ABS_CTR_MDL, /*  6:bInterfaceSubClass */
        1,                                        /*  7:bInterfaceProtocol */
        7,                                        /*  8:iInterface */

        /* Communications Class Functional Descriptorss */
        5,                                          /*  0:bLength */
        USB_PCDC_CS_INTERFACE,                      /*  1:bDescriptorType */
        USB_PCDC_DT_SUBTYPE_HEADER_FUNC,            /*  2:bDescriptorSubtype */
        USB_PCDC_BCD_CDC % USB_W_TOTAL_LENGTH_MASK, /*  3:bcdCDC_lo */
        USB_PCDC_BCD_CDC / USB_W_TOTAL_LENGTH_MASK, /*  4:bcdCDC_hi */

        /* Communications Class Functional Descriptorss */
        4,                                            /*  0:bLength */
        USB_PCDC_CS_INTERFACE,                        /*  1:bDescriptorType */
        USB_PCDC_DT_SUBTYPE_ABSTRACT_CTR_MANAGE_FUNC, /*  2:bDescriptorSubtype */
        2,                                            /*  3:bmCapabilities */

        /* Communications Class Functional Descriptorss */
        5,                              /*  0:bLength */
        USB_PCDC_CS_INTERFACE,          /*  1:bDescriptorType */
        USB_PCDC_DT_SUBTYPE_UNION_FUNC, /*  2:bDescriptorSubtype */
        0,                              /*  3:bMasterInterface */
        1,                              /*  4:bSlaveInterface0 */

        /* Communications Class Functional Descriptorss */
        5,                                    /*  0:bLength */
        USB_PCDC_CS_INTERFACE,                /*  1:bDescriptorType */
        USB_PCDC_DT_SUBTYPE_CALL_MANAGE_FUNC, /*  2:bDescriptorSubtype */
        /* D1:1-Device can send/receive call management
         information over a Data Class interface. */
        /* D0:1-Device handles call management itself. */
        3, /*  3:bmCapabilities */
        1, /*  4:bDataInterface */

        /* Endpoint Descriptor 0 */
        USB_ED_BLENGTH,      /*  0:bLength */
        USB_DT_ENDPOINT,     /*  1:bDescriptorType */
        USB_EP_IN | USB_EP1, /*  2:bEndpointAddress */
        USB_EP_INT,          /*  3:bmAttribute */
        USB_W_MAX_PACKET_SIZE_MASK,                  /*  4:wMAXPacketSize_lo */
        0,                   /*  5:wMAXPacketSize_hi */
        0x10,                /*  6:bInterval */

        /* Interface Descriptor */
        USB_ID_BLENGTH,   /*  0:bLength */
        USB_DT_INTERFACE, /*  1:bDescriptor */
        1,                /*  2:bInterfaceNumber */
        0,                /*  3:bAlternateSetting */
        2,                /*  4:bNumEndpoints */
        USB_IFCLS_CDCD,   /*  5:bInterfaceClass */
        0,                /*  6:bInterfaceSubClass */
        0,                /*  7:bInterfaceProtocol */
        7,                /*  8:iInterface */

        /* Endpoint Descriptor 0 */
        USB_ED_BLENGTH,             /*  0:bLength */
        USB_DT_ENDPOINT,            /*  1:bDescriptorType */
        USB_EP_IN | USB_EP2,        /*  2:bEndpointAddress */
        USB_EP_BULK,                /*  3:bmAttribute */
        USB_W_MAX_PACKET_SIZE_MASK, /*  4:wMAXPacketSize_lo */
        0,                          /*  5:wMAXPacketSize_hi */
        0,                          /*  6:bInterval */

        /* Endpoint Descriptor 1 */
        USB_ED_BLENGTH,             /*  0:bLength */
        USB_DT_ENDPOINT,            /*  1:bDescriptorType */
        USB_EP_OUT | USB_EP3,       /*  2:bEndpointAddress */
        USB_EP_BULK,                /*  3:bmAttribute */
        USB_W_MAX_PACKET_SIZE_MASK, /*  4:wMAXPacketSize_lo */
        0,                          /*  5:wMAXPacketSize_hi */
        0,                          /*  6:bInterval */

        /* Vendor specific Device Class */
        
        /* Interface Descriptor */
        USB_ID_BLENGTH,   /*  0:bLength */
        USB_DT_INTERFACE, /*  1:bDescriptor */
        INTERFACE_CMSIS_DAP,/*  2:bInterfaceNumber */
        0,                /*  3:bAlternateSetting */
        3,                /*  4:bNumEndpoints */
        USB_VENDOR_CODE,  /*  5:bInterfaceClass */
        0x00,             /*  6:bInterfaceSubClass */
        0x00,             /*  7:bInterfaceProtocol */
        3,                /*  8:iInterface */

        /* Endpoint Descriptor 0 */
        USB_ED_BLENGTH,                                /*  0:bLength */
        USB_DT_ENDPOINT,                               /*  1:bDescriptorType */
        (uint8_t)(USB_EP_OUT | USB_EP4),                /*  2:bEndpointAddress */
        USB_EP_BULK,                                   /*  3:bmAttribute */
        (uint8_t)(USB_MXPS_BULK_FULL % USB_VALUE_256), /*  4:wMaxPacketSize_lo */
        (uint8_t)(USB_MXPS_BULK_FULL / USB_VALUE_256), /*  5:wMaxPacketSize_hi */
        0,                                             /*  6:bInterval */

        /* Endpoint Descriptor 1 */
        USB_ED_BLENGTH,                                /*  0:bLength */
        USB_DT_ENDPOINT,                               /*  1:bDescriptorType */
        (uint8_t)(USB_EP_IN | USB_EP5),               /*  2:bEndpointAddress */
        USB_EP_BULK,                                   /*  3:bmAttribute */
        (uint8_t)(USB_MXPS_BULK_FULL % USB_VALUE_256), /*  4:wMaxPacketSize_lo */
        (uint8_t)(USB_MXPS_BULK_FULL / USB_VALUE_256), /*  5:wMaxPacketSize_hi */
        0,                                             /*  6:bInterval */
        
        
        /* Endpoint Descriptor 2 */
        USB_ED_BLENGTH,                              /*  0:bLength */
        USB_DT_ENDPOINT,                             /*  1:bDescriptorType */
        (uint8_t)(USB_EP_IN | USB_EP6),             /*  2:bEndpointAddress */
        USB_EP_BULK,                                 /*  3:bmAttribute */
        (uint8_t)(USB_MXPS_BULK_FULL % USB_VALUE_256), /*  4:wMaxPacketSize_lo */
        (uint8_t)(USB_MXPS_BULK_FULL / USB_VALUE_256), /*  5:wMaxPacketSize_hi */
        0,  

};

/* For High-Speed */
uint8_t g_apl_hs_configuration[USB_PCDC_PVND_CD_LEN + (USB_PCDC_PVND_CD_LEN % 2)] =
    {
        USB_CD_BLENGTH,                                /*  0:bLength */
        USB_SOFT_CHANGE,                               /*  1:bDescriptorType */
        USB_PCDC_PVND_CD_LEN % USB_W_TOTAL_LENGTH_MASK, /*  2:wTotalLength(L) */
        USB_PCDC_PVND_CD_LEN / USB_W_TOTAL_LENGTH_MASK, /*  3:wTotalLength(H) */
        3,                                             /*  4:bNumInterfaces */
        1,                                             /*  5:bConfigurationValue */
        0,                                             /*  6:iConfiguration */
        USB_CF_RESERVED | USB_CF_SELFP,                /*  7:bmAttributes */
        (500 / 2),                                     /*  8:MAXPower (2mA unit) */

        /* Communication Device Class */

        /* Interface Association Descriptor (IAD) */
        0x08,                                     /*  0:bLength */
        USB_IAD_TYPE,                             /*  1:bDescriptorType */
        INTERFACE_PCDC_FIRST,                     /*  2:bFirstInterface */
        0x02,                                     /*  3:bInterfaceCount */
        USB_IFCLS_CDCC,                           /*  4:bFunctionClass  */
        USB_PCDC_CLASS_SUBCLASS_CODE_ABS_CTR_MDL, /* 5:bFunctionSubClass */
        0x01,                                     /*  6:bFunctionProtocol */
        0x00,                                     /*  7:iFunction */

        /* Interface Descriptor */
        USB_ID_BLENGTH,                           /*  0:bLength */
        USB_DT_INTERFACE,                         /*  1:bDescriptor */
        INTERFACE_PCDC_FIRST,                     /*  2:bInterfaceNumber */
        0,                                        /*  3:bAlternateSetting */
        1,                                        /*  4:bNumEndpoints */
        USB_IFCLS_CDCC,                           /*  5:bInterfaceClass */
        USB_PCDC_CLASS_SUBCLASS_CODE_ABS_CTR_MDL, /*  6:bInterfaceSubClass */
        1,                                        /*  7:bInterfaceProtocol */
        0,                                        /*  8:iInterface */

        /* Communications Class Functional Descriptorss */
        5,                                        /*  0:bLength */
        USB_PCDC_CS_INTERFACE,                    /*  1:bDescriptorType */
        USB_PCDC_DT_SUBTYPE_HEADER_FUNC,          /*  2:bDescriptorSubtype */
        USB_PCDC_BCD_CDC % USB_W_TOTAL_LENGTH_MASK, /*  3:bcdCDC_lo */
        USB_PCDC_BCD_CDC / USB_W_TOTAL_LENGTH_MASK, /*  4:bcdCDC_hi */

        /* Communications Class Functional Descriptorss */
        4,                                            /*  0:bLength */
        USB_PCDC_CS_INTERFACE,                        /*  1:bDescriptorType */
        USB_PCDC_DT_SUBTYPE_ABSTRACT_CTR_MANAGE_FUNC, /*  2:bDescriptorSubtype */
        2,                                            /*  3:bmCapabilities */

        /* Communications Class Functional Descriptorss */
        5,                              /*  0:bLength */
        USB_PCDC_CS_INTERFACE,          /*  1:bDescriptorType */
        USB_PCDC_DT_SUBTYPE_UNION_FUNC, /*  2:bDescriptorSubtype */
        0,                              /*  3:bMasterInterface */
        1,                              /*  4:bSlaveInterface0 */

        /* Communications Class Functional Descriptorss */
        5,                                    /*  0:bLength */
        USB_PCDC_CS_INTERFACE,                /*  1:bDescriptorType */
        USB_PCDC_DT_SUBTYPE_CALL_MANAGE_FUNC, /*  2:bDescriptorSubtype */
        /* D1:1-Device can send/receive call management
         information over a Data Class interface. */
        /* D0:1-Device handles call management itself. */
        3, /*  3:bmCapabilities */
        1, /*  4:bDataInterface */

        /* Endpoint Descriptor 0 */
        7,                   /*  0:bLength */
        USB_DT_ENDPOINT,     /*  1:bDescriptorType */
        USB_EP_IN | USB_EP1, /*  2:bEndpointAddress */
        USB_EP_INT,          /*  3:bmAttribute */
        USB_W_MAX_PACKET_SIZE_MASK,                  /*  4:wMAXPacketSize_lo */
        0,                   /*  5:wMAXPacketSize_hi */
        0x10,                /*  6:bInterval */

        /* Interface Descriptor */
        USB_ID_BLENGTH,   /*  0:bLength */
        USB_DT_INTERFACE, /*  1:bDescriptor */
        1,                /*  2:bInterfaceNumber */
        0,                /*  3:bAlternateSetting */
        2,                /*  4:bNumEndpoints */
        USB_IFCLS_CDCD,   /*  5:bInterfaceClass */
        0,                /*  6:bInterfaceSubClass */
        0,                /*  7:bInterfaceProtocol */
        7,                /*  8:iInterface */

        /* Endpoint Descriptor 0 */
        USB_ED_BLENGTH,      /*  0:bLength */
        USB_DT_ENDPOINT,     /*  1:bDescriptorType */
        USB_EP_IN | USB_EP2, /*  2:bEndpointAddress */
        USB_EP_BULK,         /*  3:bmAttribute */
        0,                   /*  4:wMAXPacketSize_lo */
        2,                   /*  5:wMAXPacketSize_hi */
        0,                   /*  6:bInterval */

        /* Endpoint Descriptor 1 */
        USB_ED_BLENGTH,       /*  0:bLength */
        USB_DT_ENDPOINT,      /*  1:bDescriptorType */
        USB_EP_OUT | USB_EP3, /*  2:bEndpointAddress */
        USB_EP_BULK,          /*  3:bmAttribute */
        0,                    /*  4:wMAXPacketSize_lo */
        2,                    /*  5:wMAXPacketSize_hi */
        0,                    /*  6:bInterval */

        /* Vendor specific Device Class */
        
        /* Interface Descriptor */
        USB_ID_BLENGTH,   /*  0:bLength */
        USB_DT_INTERFACE, /*  1:bDescriptor */
        INTERFACE_CMSIS_DAP,/*  2:bInterfaceNumber */
        0,                /*  3:bAlternateSetting */
        3,                /*  4:bNumEndpoints */
        USB_VENDOR_CODE,  /*  5:bInterfaceClass */
        0x00,             /*  6:bInterfaceSubClass */
        0x00,             /*  7:bInterfaceProtocol */
        0,                /*  8:iInterface */

        /* Endpoint Descriptor 0 */
        USB_ED_BLENGTH,                              /*  0:bLength */
        USB_DT_ENDPOINT,                             /*  1:bDescriptorType */
        (uint8_t)(USB_EP_IN | USB_EP4),              /*  2:bEndpointAddress */
        USB_EP_BULK,                                 /*  3:bmAttribute */
        (uint8_t)(USB_MXPS_BULK_HI % USB_VALUE_256), /*  4:wMaxPacketSize_lo */
        (uint8_t)(USB_MXPS_BULK_HI / USB_VALUE_256), /*  5:wMaxPacketSize_hi */
        0,                                           /*  6:bInterval */

        /* Endpoint Descriptor 1 */
        USB_ED_BLENGTH,                              /*  0:bLength */
        USB_DT_ENDPOINT,                             /*  1:bDescriptorType */
        (uint8_t)(USB_EP_OUT | USB_EP5),             /*  2:bEndpointAddress */
        USB_EP_BULK,                                 /*  3:bmAttribute */
        (uint8_t)(USB_MXPS_BULK_HI % USB_VALUE_256), /*  4:wMaxPacketSize_lo */
        (uint8_t)(USB_MXPS_BULK_HI / USB_VALUE_256), /*  5:wMaxPacketSize_hi */
        0,                                           /*  6:bInterval */

        /* Endpoint Descriptor 2 */
        USB_ED_BLENGTH,                              /*  0:bLength */
        USB_DT_ENDPOINT,                             /*  1:bDescriptorType */
        (uint8_t)(USB_EP_IN | USB_EP6),             /*  2:bEndpointAddress */
        USB_EP_BULK,                                 /*  3:bmAttribute */
        (uint8_t)(USB_MXPS_BULK_HI % USB_VALUE_256), /*  4:wMaxPacketSize_lo */
        (uint8_t)(USB_MXPS_BULK_HI / USB_VALUE_256), /*  5:wMaxPacketSize_hi */
        0,   



};

/*************************************
 *    String Descriptor              *
 *************************************/
/* UNICODE 0x0409 English (United States) */
uint8_t g_apl_string_descriptor0[STRING_DESCRIPTOR0_LEN + (STRING_DESCRIPTOR0_LEN % 2)] =
    {
        STRING_DESCRIPTOR0_LEN, /*  0:bLength */
        USB_DT_STRING,          /*  1:bDescriptorType */
        0x09, 0x04              /*  2:wLANGID[0] */
};

/* iManufacturer */
uint8_t g_apl_string_descriptor1[STRING_DESCRIPTOR1_LEN + (STRING_DESCRIPTOR1_LEN % 2)] =
    {
        STRING_DESCRIPTOR1_LEN, /*  0:bLength */
        USB_DT_STRING,                                              /*  1:bDescriptorType */
        'R',
        0x00, /*  2:wLANGID[0] */
        'e',
        0x00,
        'n',
        0x00,
        'e',
        0x00,
        's',
        0x00,
        'a',
        0x00,
        's',
        0x00,
};

/* iProduct */
uint8_t g_apl_string_descriptor2[STRING_DESCRIPTOR2_LEN + (STRING_DESCRIPTOR2_LEN % 2)] =
    {
        STRING_DESCRIPTOR2_LEN, /*  0:bLength */
        USB_DT_STRING,          /*  1:bDescriptorType */

        'R', 0x00,
        'A', 0x00,
        '4', 0x00,
        'M', 0x00,
        '2', 0x00,
        ' ', 0x00,
        'C', 0x00,
        'M', 0x00,
        'S', 0x00,
        'I', 0x00,
        'S', 0x00,
        '-', 0x00,
        'D', 0x00,
        'A', 0x00,
        'P', 0x00,
        ' ', 0x00,
        'P', 0x00,
        'r', 0x00,
        'o', 0x00,
        'b', 0x00,
        'e', 0x00};

/* iInterface */
uint8_t g_apl_string_descriptor3[STRING_DESCRIPTOR3_LEN + (STRING_DESCRIPTOR3_LEN % 2)] =
    {
        STRING_DESCRIPTOR3_LEN, /*  0:bLength */
        USB_DT_STRING,          /*  1:bDescriptorType */
        'R', 0x00,
        'A', 0x00,
        '4', 0x00,
        'M', 0x00,
        '2', 0x00,
        ' ', 0x00,
        'C', 0x00,
        'M', 0x00,
        'S', 0x00,
        'I', 0x00,
        'S', 0x00,
        '-', 0x00,
        'D', 0x00,
        'A', 0x00,
        'P', 0x00};

/* iConfiguration */
uint8_t g_apl_string_descriptor4[STRING_DESCRIPTOR4_LEN + (STRING_DESCRIPTOR4_LEN % 2)] =
    {
        STRING_DESCRIPTOR4_LEN, /*  0:bLength */
        USB_DT_STRING,          /*  1:bDescriptorType */
        'F', 0x00,              /*  2:wLANGID[0] */
        'u', 0x00,
        'l', 0x00,
        'l', 0x00,
        '-', 0x00,
        'S', 0x00,
        'p', 0x00,
        'e', 0x00,
        'e', 0x00,
        'd', 0x00};

/* iConfiguration */
uint8_t g_apl_string_descriptor5[STRING_DESCRIPTOR5_LEN + (STRING_DESCRIPTOR5_LEN % 2)] =
    {
        STRING_DESCRIPTOR5_LEN, /*  0:bLength */
        USB_DT_STRING,          /*  1:bDescriptorType */
        'H', 0x00,              /*  2:wLANGID[0] */
        'i', 0x00,
        '-', 0x00,
        'S', 0x00,
        'p', 0x00,
        'e', 0x00,
        'e', 0x00,
        'd', 0x00};

/* iSerialNumber */
/* This will be programatically updated with a device Unique ID */
uint8_t g_apl_string_descriptor_serial_number[STRING_DESCRIPTOR6_LEN + (STRING_DESCRIPTOR6_LEN % 2)] =
    {
        STRING_DESCRIPTOR6_LEN, /*  0:bLength */
        USB_DT_STRING,          /*  1:bDescriptorType */
        '0',
        0x00, /*  2:wLANGID[0] */
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
        '0',
        0x00,
};

/* iInterface */
uint8_t g_apl_string_descriptor7[STRING_DESCRIPTOR7_LEN + (STRING_DESCRIPTOR7_LEN % 2)] =
    {
        STRING_DESCRIPTOR7_LEN, /*  0:bLength */
        USB_DT_STRING,          /*  1:bDescriptorType */
        'R', 0x00,
        'A', 0x00,
        '4', 0x00,
        'M', 0x00,
        '2', 0x00,
        '-', 0x00,
        'V', 0x00,
        'C', 0x00,
        'O', 0x00,
        'M', 0x00};

uint8_t MSFT100[STRING_MSFT100_LEN + (STRING_MSFT100_LEN % 2)] =
    {
        STRING_MSFT100_LEN,      /*  0:bLength */
        USB_DT_STRING,          /*  1:bDescriptorType */
        'M', 0x00,
        'S', 0x00,
        'F', 0x00,
        'T', 0x00,
        '1', 0x00,
        '0', 0x00,
        '0', 0x00,
        MS_VENDOR_CODE_CMSIS_DAP, /* Vendor code */
        0x00 /* Pad */
        };

uint8_t *g_apl_string_table[NUM_STRING_DESCRIPTOR] =
    {
        g_apl_string_descriptor0,
        g_apl_string_descriptor1,
        g_apl_string_descriptor2,
        g_apl_string_descriptor3,
        g_apl_string_descriptor4,
        g_apl_string_descriptor5,
        g_apl_string_descriptor_serial_number,
        g_apl_string_descriptor7,
        NULL,
        NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
        
        MSFT100 /* String Index : 238 */
};

tyRAM4ECID ecd = {
    .hdr = {
        .dwLength               = sizeof(tyRAM4ECID),
        .bcdVersion             = 0x0100,
        .wIndex                 = 0x0004,
        .bCount                 = 0x02,
        .RESERVED               = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    },
    .pvnd = {
        .bFirstInterfaceNumber = INTERFACE_CMSIS_DAP,
        .RESERVED1             = {0x01},
        .compatibleID          = {'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00},
        .subCompatibleID       = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .RESERVED2             = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    },
    .pcdc = {
        .bFirstInterfaceNumber = INTERFACE_PCDC_FIRST,
        .RESERVED1             = {0x01},
        .compatibleID          = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .subCompatibleID       = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .RESERVED2             = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    }
};

uint8_t ExtendedPropertiesDescriptor[0x0000008E] = 
{ 
    // Header Section
    0x8E, 0x00, 0x00, 0x00, // dwLength
    0x00, 0x01,             // bcdVersion
    0x05, 0x00,             // wIndex
    0x01, 0x00,             // wCount
    
    // Custom Property Section
    0x84, 0x00, 0x00, 0x00, // dwSize
    0x01, 0x00, 0x00, 0x00, // dwPropertyDataType (A NULL-terminated Unicode String (REG_SZ))
    0x28, 0x00,             // wPropertyNameLength
    
    // bPropertyName DeviceInterfaceGUIDs\0
    'D' , 0x00, 'e' , 0x00, 'v' , 0x00, 'i' , 0x00, 'c' , 0x00, 'e' , 0x00, 'I' , 0x00, 'n' , 0x00, 
    't' , 0x00, 'e' , 0x00, 'r' , 0x00, 'f' , 0x00, 'a' , 0x00, 'c' , 0x00, 'e' , 0x00, 'G' , 0x00, 
    'U' , 0x00, 'I' , 0x00, 'D' , 0x00, 0x00, 0x00,

    // dwPropertyDataLength
    0x4E, 0x00, 0x00, 0x00, 
    
     // bPropertyData "{CDB3B5AD-293B-4663-AA36-1AAE46463776}"
    '{' , 0x00, 'C' , 0x00, 'D' , 0x00, 'B' , 0x00, '3' , 0x00, 'B' , 0x00, '5' , 0x00, 'A' , 0x00, 
    'D' , 0x00, '-' , 0x00, '2' , 0x00, '9' , 0x00, '3' , 0x00, 'B' , 0x00, '-' , 0x00, '4' , 0x00, 
    '6' , 0x00, '6' , 0x00, '3' , 0x00, '-' , 0x00, 'A' , 0x00, 'A' , 0x00, '3' , 0x00, '6' , 0x00, 
    '-' , 0x00, '1' , 0x00, 'A' , 0x00, 'A' , 0x00, 'E' , 0x00, '4' , 0x00, '6' , 0x00, '4' , 0x00, 
    '6' , 0x00, '3' , 0x00, '7' , 0x00, '7' , 0x00, '6' , 0x00, '}' , 0x00, 0x00, 0x00
};  


const usb_descriptor_t g_usb_descriptor =
    {
        g_apl_device,               /* Pointer to the device descriptor */
        g_apl_configuration,        /* Pointer to the configuration descriptor for Full-speed */
        g_apl_hs_configuration,     /* Pointer to the configuration descriptor for Hi-speed */
        g_apl_qualifier_descriptor, /* Pointer to the qualifier descriptor */
        g_apl_string_table,         /* Pointer to the string descriptor table */
        NUM_STRING_DESCRIPTOR};

/******************************************************************************
 End  Of File
 ******************************************************************************/
