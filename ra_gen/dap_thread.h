/* generated thread header file - do not edit */
#ifndef DAP_THREAD_H_
#define DAP_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void dap_thread_entry(void * pvParameters);
                #else
extern void dap_thread_entry(void *pvParameters);
#endif
#include "r_sci_uart.h"
#include "r_uart_api.h"
#include "r_usb_basic.h"
#include "r_usb_basic_api.h"
#include "r_usb_phid_api.h"
#include "r_usb_pcdc_api.h"
FSP_HEADER
/** UART on SCI Instance. */
extern const uart_instance_t g_uart;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t g_uart_ctrl;
extern const uart_cfg_t g_uart_cfg;
extern const sci_uart_extended_cfg_t g_uart_cfg_extend;

#ifndef user_uart_callback
void user_uart_callback(uart_callback_args_t *p_args);
#endif
/* Basic on USB Instance. */
extern const usb_instance_t g_basic1;

/** Access the USB instance using these structures when calling API functions directly (::p_api is not used). */
extern usb_instance_ctrl_t g_basic1_ctrl;
extern const usb_cfg_t g_basic1_cfg;

#ifndef NULL
void NULL(void*);
#endif

#if 2 == BSP_CFG_RTOS
#ifndef usb_composite_callback
void usb_composite_callback(usb_event_info_t *, usb_hdl_t, usb_onoff_t);
#endif
#endif
/** PHID Driver on USB Instance. */
/** CDC Driver on USB Instance. */
FSP_FOOTER
#endif /* DAP_THREAD_H_ */
