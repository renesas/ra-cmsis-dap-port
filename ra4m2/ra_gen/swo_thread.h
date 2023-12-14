/* generated thread header file - do not edit */
#ifndef SWO_THREAD_H_
#define SWO_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void swo_thread_entry(void * pvParameters);
                #else
extern void swo_thread_entry(void *pvParameters);
#endif
#include "r_sci_uart.h"
#include "r_uart_api.h"
FSP_HEADER
/** UART on SCI Instance. */
extern const uart_instance_t g_uart_swo;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t g_uart_swo_ctrl;
extern const uart_cfg_t g_uart_swo_cfg;
extern const sci_uart_extended_cfg_t g_uart_swo_cfg_extend;

#ifndef swo_uart_callback
void swo_uart_callback(uart_callback_args_t *p_args);
#endif
FSP_FOOTER
#endif /* SWO_THREAD_H_ */
