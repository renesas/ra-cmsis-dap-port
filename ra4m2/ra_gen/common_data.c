/* generated common source file - do not edit */
#include "common_data.h"
ioport_instance_ctrl_t g_ioport_ctrl;
const ioport_instance_t g_ioport =
{ .p_api = &g_ioport_on_ioport, .p_ctrl = &g_ioport_ctrl, .p_cfg = &g_bsp_pin_cfg, };
SemaphoreHandle_t g_sem_uart_tx;
#if 1
StaticSemaphore_t g_sem_uart_tx_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_sem_pcdc_tx;
#if 1
StaticSemaphore_t g_sem_pcdc_tx_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
QueueHandle_t g_queue_uart_tx_8;
#if 1
StaticQueue_t g_queue_uart_tx_8_memory;
uint8_t g_queue_uart_tx_8_queue_memory[1 * 2048];
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
QueueHandle_t g_queue_uart_tx_16;
#if 1
StaticQueue_t g_queue_uart_tx_16_memory;
uint8_t g_queue_uart_tx_16_queue_memory[2 * 2048];
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
QueueHandle_t g_queue_usb_event;
#if 1
StaticQueue_t g_queue_usb_event_memory;
uint8_t g_queue_usb_event_queue_memory[4 * 20];
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
void g_common_init(void)
{
    g_sem_uart_tx =
#if 1
            xSemaphoreCreateBinaryStatic (&g_sem_uart_tx_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_sem_uart_tx)
    {
        rtos_startup_err_callback (g_sem_uart_tx, 0);
    }
    g_sem_pcdc_tx =
#if 1
            xSemaphoreCreateBinaryStatic (&g_sem_pcdc_tx_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_sem_pcdc_tx)
    {
        rtos_startup_err_callback (g_sem_pcdc_tx, 0);
    }
    g_queue_uart_tx_8 =
#if 1
            xQueueCreateStatic (
#else
                xQueueCreate(
                #endif
                                2048,
                                1
#if 1
                                ,
                                &g_queue_uart_tx_8_queue_memory[0], &g_queue_uart_tx_8_memory
#endif
                                );
    if (NULL == g_queue_uart_tx_8)
    {
        rtos_startup_err_callback (g_queue_uart_tx_8, 0);
    }
    g_queue_uart_tx_16 =
#if 1
            xQueueCreateStatic (
#else
                xQueueCreate(
                #endif
                                2048,
                                2
#if 1
                                ,
                                &g_queue_uart_tx_16_queue_memory[0], &g_queue_uart_tx_16_memory
#endif
                                );
    if (NULL == g_queue_uart_tx_16)
    {
        rtos_startup_err_callback (g_queue_uart_tx_16, 0);
    }
    g_queue_usb_event =
#if 1
            xQueueCreateStatic (
#else
                xQueueCreate(
                #endif
                                20,
                                4
#if 1
                                ,
                                &g_queue_usb_event_queue_memory[0], &g_queue_usb_event_memory
#endif
                                );
    if (NULL == g_queue_usb_event)
    {
        rtos_startup_err_callback (g_queue_usb_event, 0);
    }
}
