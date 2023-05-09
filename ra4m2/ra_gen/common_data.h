/* generated common header file - do not edit */
#ifndef COMMON_DATA_H_
#define COMMON_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "r_ioport.h"
#include "bsp_pin_cfg.h"
FSP_HEADER
#define IOPORT_CFG_NAME g_bsp_pin_cfg

/* IOPORT Instance */
extern const ioport_instance_t g_ioport;

/* IOPORT control structure. */
extern ioport_instance_ctrl_t g_ioport_ctrl;
extern SemaphoreHandle_t g_sem_uart_tx;
extern SemaphoreHandle_t g_sem_pcdc_tx;
extern QueueHandle_t g_queue_uart_tx_8;
extern QueueHandle_t g_queue_uart_tx_16;
extern QueueHandle_t g_queue_usb_event;
void g_common_init(void);
FSP_FOOTER
#endif /* COMMON_DATA_H_ */
