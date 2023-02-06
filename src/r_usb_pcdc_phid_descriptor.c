/* ${REA_DISCLAIMER_PLACEHOLDER} */
/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/
#include "r_usb_basic.h"
#include "r_usb_basic_api.h"
#include "r_usb_phid_api.h"

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
#define USB_PRODUCTID (0x0201DU)

#define USB_IFPROTOCOL (USB_IFPRO_NONE)
#define ITEM_LEN (35U)
#define MXPS (64U)
#define NUM_EP (2U)

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
#define USB_PCDC_HID_CD_LEN (107U)
#define STRING_DESCRIPTOR0_LEN (4U)
#define STRING_DESCRIPTOR1_LEN (16U)
#define STRING_DESCRIPTOR2_LEN (49U)
#define STRING_DESCRIPTOR3_LEN (43U)
#define STRING_DESCRIPTOR4_LEN (22U)
#define STRING_DESCRIPTOR5_LEN (18U)
#define STRING_DESCRIPTOR6_LEN (28U)
#define STRING_DESCRIPTOR7_LEN (33U)
#define NUM_STRING_DESCRIPTOR (8U)

/* Descriptor data Mask */
#define USB_UCHAR_MAX (0xffU)
#define USB_W_TOTAL_LENGTH_MASK (256U)
#define USB_W_MAX_PACKET_SIZE_MASK (64U)
#define USB_PCDC_BCD_CDC_MASK (256U)

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
uint8_t g_apl_configuration[USB_PCDC_HID_CD_LEN + (USB_PCDC_HID_CD_LEN % 2)] =
    {
        USB_CD_BLENGTH,                                /*  0:bLength */
        USB_SOFT_CHANGE,                               /*  1:bDescriptorType */
        USB_PCDC_HID_CD_LEN % USB_W_TOTAL_LENGTH_MASK, /*  2:wTotalLength(L) */
        USB_PCDC_HID_CD_LEN / USB_W_TOTAL_LENGTH_MASK, /*  3:wTotalLength(H) */
        3,                                             /*  4:bNumInterfaces */
        1,                                             /*  5:bConfigurationValue */
        0,                                             /*  6:iConfiguration */
        USB_CF_RESERVED | USB_CF_SELFP,                /*  7:bmAttributes */
        (10 / 2),                                      /*  8:MAXPower (2mA unit) */

        /* Communication Device Class */

        /* Interface Association Descriptor (IAD) */
        0x08,                                     /*  0:bLength */
        USB_IAD_TYPE,                             /*  1:bDescriptorType */
        0x00,                                     /*  2:bFirstInterface */
        0x02,                                     /*  3:bInterfaceCount */
        USB_IFCLS_CDCC,                           /*  4:bFunctionClass  */
        USB_PCDC_CLASS_SUBCLASS_CODE_ABS_CTR_MDL, /* 5:bFunctionSubClass */
        0x01,                                     /*  6:bFunctionProtocol */
        0x00,                                     /*  7:iFunction */

        /* Interface Descriptor */
        USB_ID_BLENGTH,                           /*  0:bLength */
        USB_DT_INTERFACE,                         /*  1:bDescriptor */
        0,                                        /*  2:bInterfaceNumber */
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
        USB_EP_IN | USB_EP3, /*  2:bEndpointAddress */
        USB_EP_INT,          /*  3:bmAttribute */
        16,                  /*  4:wMAXPacketSize_lo */
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
        USB_EP_IN | USB_EP4,        /*  2:bEndpointAddress */
        USB_EP_BULK,                /*  3:bmAttribute */
        USB_W_MAX_PACKET_SIZE_MASK, /*  4:wMAXPacketSize_lo */
        0,                          /*  5:wMAXPacketSize_hi */
        0,                          /*  6:bInterval */

        /* Endpoint Descriptor 1 */
        USB_ED_BLENGTH,             /*  0:bLength */
        USB_DT_ENDPOINT,            /*  1:bDescriptorType */
        USB_EP_OUT | USB_EP5,       /*  2:bEndpointAddress */
        USB_EP_BULK,                /*  3:bmAttribute */
        USB_W_MAX_PACKET_SIZE_MASK, /*  4:wMAXPacketSize_lo */
        0,                          /*  5:wMAXPacketSize_hi */
        0,                          /*  6:bInterval */

        /* HID Class */

        /* Interface Descriptor */
        USB_ID_BLENGTH,   /*  0:bLength */
        USB_DT_INTERFACE, /*  1:bDescriptor */
        2,                /*  2:bInterfaceNumber */
        0,                /*  3:bAlternateSetting */
        NUM_EP,           /*  4:bNumEndpoints */
        USB_IFCLS_HID,    /*  5:bInterfaceClass */
        USB_IFSUB_NOBOOT, /*  6:bInterfaceSubClass */
        USB_IFPROTOCOL,   /*  7:bInterfaceProtocol */
        3,                /*  8:iInterface */

        /* HID Descriptor */
        9,                         /*  0:bLength */
        USB_DT_TYPE_HIDDESCRIPTOR, /*  1:bDescriptor */
        0x00,                      /*  2:HID Ver */
        0x01,                      /*  3:HID Ver */
        0x00,                      /*  4:bCountryCode */
        0x01,                      /*  5:bNumDescriptors */
        0x22,                      /*  6:bDescriptorType */
        ITEM_LEN,                  /*  7:wItemLength(L) */
        0x00,                      /*  8:wItemLength(H) */

        /* Endpoint Descriptor 0 */
        USB_ED_BLENGTH,      /*  0:bLength */
        USB_DT_ENDPOINT,     /*  1:bDescriptorType */
        USB_EP_IN | USB_EP1, /*  2:bEndpointAddress */
        USB_EP_INT,          /*  3:bmAttribute */
        MXPS,                /*  4:wMAXPacketSize_lo */
        0,                   /*  5:wMAXPacketSize_hi */
        0x01,                /*  6:bInterval */

        /* Endpoint Descriptor 1 */
        USB_ED_BLENGTH,       /*  0:bLength */
        USB_DT_ENDPOINT,      /*  1:bDescriptorType */
        USB_EP_OUT | USB_EP1, /*  2:bEndpointAddress */
        USB_EP_INT,           /*  3:bmAttribute */
        MXPS,                 /*  4:wMAXPacketSize_lo */
        0,                    /*  5:wMAXPacketSize_hi */
        0x01,                 /*  6:bInterval */
};

/* For High-Speed */
uint8_t g_apl_hs_configuration[USB_PCDC_HID_CD_LEN + (USB_PCDC_HID_CD_LEN % 2)] =
    {
        USB_CD_BLENGTH,                                /*  0:bLength */
        USB_SOFT_CHANGE,                               /*  1:bDescriptorType */
        USB_PCDC_HID_CD_LEN % USB_W_TOTAL_LENGTH_MASK, /*  2:wTotalLength(L) */
        USB_PCDC_HID_CD_LEN / USB_W_TOTAL_LENGTH_MASK, /*  3:wTotalLength(H) */
        3,                                             /*  4:bNumInterfaces */
        1,                                             /*  5:bConfigurationValue */
        0,                                             /*  6:iConfiguration */
        USB_CF_RESERVED | USB_CF_SELFP,                /*  7:bmAttributes */
        (10 / 2),                                      /*  8:MAXPower (2mA unit) */

        /* Communication Device Class */

        /* Interface Association Descriptor (IAD) */
        0x08,                                     /*  0:bLength */
        USB_IAD_TYPE,                             /*  1:bDescriptorType */
        0x00,                                     /*  2:bFirstInterface */
        0x02,                                     /*  3:bInterfaceCount */
        USB_IFCLS_CDCC,                           /*  4:bFunctionClass  */
        USB_PCDC_CLASS_SUBCLASS_CODE_ABS_CTR_MDL, /* 5:bFunctionSubClass */
        0x01,                                     /*  6:bFunctionProtocol */
        0x00,                                     /*  7:iFunction */

        /* Interface Descriptor */
        USB_ID_BLENGTH,                           /*  0:bLength */
        USB_DT_INTERFACE,                         /*  1:bDescriptor */
        0,                                        /*  2:bInterfaceNumber */
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
        USB_PCDC_BCD_CDC % USB_PCDC_BCD_CDC_MASK, /*  3:bcdCDC_lo */
        USB_PCDC_BCD_CDC / USB_PCDC_BCD_CDC_MASK, /*  4:bcdCDC_hi */

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
        USB_EP_IN | USB_EP3, /*  2:bEndpointAddress */
        USB_EP_INT,          /*  3:bmAttribute */
        16,                  /*  4:wMAXPacketSize_lo */
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
        USB_EP_IN | USB_EP4, /*  2:bEndpointAddress */
        USB_EP_BULK,         /*  3:bmAttribute */
        0,                   /*  4:wMAXPacketSize_lo */
        2,                   /*  5:wMAXPacketSize_hi */
        0,                   /*  6:bInterval */

        /* Endpoint Descriptor 1 */
        USB_ED_BLENGTH,       /*  0:bLength */
        USB_DT_ENDPOINT,      /*  1:bDescriptorType */
        USB_EP_OUT | USB_EP5, /*  2:bEndpointAddress */
        USB_EP_BULK,          /*  3:bmAttribute */
        0,                    /*  4:wMAXPacketSize_lo */
        2,                    /*  5:wMAXPacketSize_hi */
        0,                    /*  6:bInterval */

        /* HID Class */

        /* Interface Descriptor */
        USB_ID_BLENGTH,   /*  0:bLength */
        USB_DT_INTERFACE, /*  1:bDescriptor */
        2,                /*  2:bInterfaceNumber */
        0,                /*  3:bAlternateSetting */
        NUM_EP,           /*  4:bNumEndpoints */
        USB_IFCLS_HID,    /*  5:bInterfaceClass */
        USB_IFSUB_NOBOOT, /*  6:bInterfaceSubClass */
        USB_IFPROTOCOL,   /*  7:bInterfaceProtocol */
        3,                /*  8:iInterface */

        /* HID Descriptor */
        9,                         /*  0:bLength */
        USB_DT_TYPE_HIDDESCRIPTOR, /*  1:bDescriptor */
        0x00,                      /*  2:HID Ver */
        0x01,                      /*  3:HID Ver */
        0x00,                      /*  4:bCountryCode */
        0x01,                      /*  5:bNumDescriptors */
        0x22,                      /*  6:bDescriptorType */
        ITEM_LEN,                  /*  7:wItemLength(L) */
        0x00,                      /*  8:wItemLength(H) */

        /* Endpoint Descriptor 0 */
        USB_ED_BLENGTH,      /*  0:bLength */
        USB_DT_ENDPOINT,     /*  1:bDescriptorType */
        USB_EP_IN | USB_EP1, /*  2:bEndpointAddress */
        USB_EP_INT,          /*  3:bmAttribute */
        MXPS,                /*  4:wMAXPacketSize_lo */
        0,                   /*  5:wMAXPacketSize_hi */
        0x01,                /*  6:bInterval */

        /* Endpoint Descriptor 1 */
        USB_ED_BLENGTH,       /*  0:bLength */
        USB_DT_ENDPOINT,      /*  1:bDescriptorType */
        USB_EP_OUT | USB_EP1, /*  2:bEndpointAddress */
        USB_EP_INT,           /*  3:bmAttribute */
        MXPS,                 /*  4:wMAXPacketSize_lo */
        0,                    /*  5:wMAXPacketSize_hi */
        0x01,                 /*  6:bInterval */
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
        USB_DT_STRING,          /*  1:bDescriptorType */
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

uint8_t *g_apl_string_table[] =
    {
        g_apl_string_descriptor0,
        g_apl_string_descriptor1,
        g_apl_string_descriptor2,
        g_apl_string_descriptor3,
        g_apl_string_descriptor4,
        g_apl_string_descriptor5,
        g_apl_string_descriptor_serial_number,
        g_apl_string_descriptor7,
};

/************************************************************
 *  HID Report Descriptor for CMSIS-DAP                     *
 ************************************************************/
const uint8_t g_apl_report[] =
    {
        0x06, 0x00, 0xFF, // Usage Page (Vendor Defined 0xFF00)
        0x09, 0x01,       // Usage (0x01)
        0xA1, 0x01,       // Collection (Application)
        0x15, 0x00,       //   Logical Minimum (0)
        0x26, 0xFF, 0x00, //   Logical Maximum (255)
        0x75, 0x08,       //   Report Size (8)
        0x96, 0x40, 0x00, //   Report Count (64)
        0x09, 0x01,       //   Usage (0x01)
        0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x96, 0x40, 0x00, //   Report Count (64)
        0x09, 0x01,       //   Usage (0x01)
        0x91, 0x02,       //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x95, 0x01,       //   Report Count (1)
        0x09, 0x01,       //   Usage (0x01)
        0xB1, 0x02,       //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0xC0,             // End Collection
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
