/* generated common source file - do not edit */
#include "common_data.h"
ioport_instance_ctrl_t g_ioport_ctrl;
const ioport_instance_t g_ioport =
{ .p_api = &g_ioport_on_ioport, .p_ctrl = &g_ioport_ctrl, .p_cfg = &g_bsp_pin_cfg, };
QueueHandle_t g_uart_event_queue;
#if 1
StaticQueue_t g_uart_event_queue_memory;
uint8_t g_uart_event_queue_queue_memory[8 * 512];
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_uart_tx_mutex;
#if 1
StaticSemaphore_t g_uart_tx_mutex_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
SemaphoreHandle_t g_usb_tx_semaphore;
#if 1
StaticSemaphore_t g_usb_tx_semaphore_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
QueueHandle_t g_usb_tx_queue;
#if 1
StaticQueue_t g_usb_tx_queue_memory;
uint8_t g_usb_tx_queue_queue_memory[1 * 512];
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
QueueHandle_t g_usb_tx_x2_queue;
#if 1
StaticQueue_t g_usb_tx_x2_queue_memory;
uint8_t g_usb_tx_x2_queue_queue_memory[2 * 512];
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
void g_common_init(void)
{
    g_uart_event_queue =
#if 1
            xQueueCreateStatic (
#else
                xQueueCreate(
                #endif
                                512,
                                8
#if 1
                                ,
                                &g_uart_event_queue_queue_memory[0], &g_uart_event_queue_memory
#endif
                                );
    if (NULL == g_uart_event_queue)
    {
        rtos_startup_err_callback (g_uart_event_queue, 0);
    }
    g_uart_tx_mutex =
#if 1
            xSemaphoreCreateBinaryStatic (&g_uart_tx_mutex_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_uart_tx_mutex)
    {
        rtos_startup_err_callback (g_uart_tx_mutex, 0);
    }
    g_usb_tx_semaphore =
#if 1
            xSemaphoreCreateBinaryStatic (&g_usb_tx_semaphore_memory);
#else
                xSemaphoreCreateBinary();
                #endif
    if (NULL == g_usb_tx_semaphore)
    {
        rtos_startup_err_callback (g_usb_tx_semaphore, 0);
    }
    g_usb_tx_queue =
#if 1
            xQueueCreateStatic (
#else
                xQueueCreate(
                #endif
                                512,
                                1
#if 1
                                ,
                                &g_usb_tx_queue_queue_memory[0], &g_usb_tx_queue_memory
#endif
                                );
    if (NULL == g_usb_tx_queue)
    {
        rtos_startup_err_callback (g_usb_tx_queue, 0);
    }
    g_usb_tx_x2_queue =
#if 1
            xQueueCreateStatic (
#else
                xQueueCreate(
                #endif
                                512,
                                2
#if 1
                                ,
                                &g_usb_tx_x2_queue_queue_memory[0], &g_usb_tx_x2_queue_memory
#endif
                                );
    if (NULL == g_usb_tx_x2_queue)
    {
        rtos_startup_err_callback (g_usb_tx_x2_queue, 0);
    }
}
