#include "dap_thread.h"
/* DAP Thread entry function */
/* pvParameters contains TaskHandle_t */
#include "common_utils.h"
#include "usb_composite.h"
#include "CMSIS-DAP\DAP_config.h"
#include "CMSIS-DAP\DAP.h"
/**********************************************************************************************************************
 * @addtogroup usb_composite_ep
 * @{
 **********************************************************************************************************************/

/* Local Macros */
#define MIN(i, j) (((i) < (j)) ? (i) : (j))

/* Local Types  */
typedef enum
{
    PERIPHERAL_NONE = 0,
    PERIPHERAL_USB,
    PERIPHERAL_UART,
} peripheral_t;

typedef struct
{
    uint32_t peripheral;
    union
    {
        uint32_t data_size;
    } u;
} queue_evt_t;

/* external variables*/
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_report[];
extern bsp_leds_t g_bsp_leds;
extern uint8_t g_apl_string_descriptor_serial_number[];

/* Local Module Variables */
static bool b_usb_attach = false;
static usb_pcdc_linecoding_t g_line_coding;
static uint8_t g_PHID_tx_data[PHID_DATA_LEN] = {RESET_VALUE};
static uint8_t g_PHID_rcv_buf[PHID_DATA_LEN] = {RESET_VALUE};
static uint8_t g_PCDC_rx_data[CDC_DATA_LEN] = {RESET_VALUE};
static usb_pcdc_ctrllinestate_t g_control_line_state = {
    .bdtr = 0,
    .brts = 0,
};

/* private function declarations */
static void led_uart_activity(void);
static void handle_error(fsp_err_t err, char *err_str);
static void set_pcdc_line_coding(volatile usb_pcdc_linecoding_t *p_line_coding, const uart_cfg_t *p_uart_test_cfg);
static void set_uart_line_coding_cfg(uart_cfg_t *p_uart_test_cfg, const volatile usb_pcdc_linecoding_t *p_line_coding);

static baud_setting_t baud_setting;
static bool enable_bitrate_modulation = true;
static uint32_t g_baud_rate = RESET_VALUE;
static uint32_t error_rate_x_1000 = BAUD_ERROR_RATE;
static uart_cfg_t g_uart_test_cfg;
static sci_uart_extended_cfg_t sci_extend_cfg;
static uint8_t usb_tx_buffer[512];
static uint8_t hid_send_data[BUFF_SIZE] BSP_ALIGN_VARIABLE(ALIGN);

/*******************************************************************************************************************/ /**
  *  @brief       Initialize line coding parameters based on UART configuration
  *  @param[in]   p_line_coding       Updates the line coding member values
                  p_uart_test_cfg     Pointer to store UART configuration properties
  *  @retval      None
  **********************************************************************************************************************/
static void set_pcdc_line_coding(volatile usb_pcdc_linecoding_t *p_line_coding, const uart_cfg_t *p_uart_test_cfg)
{
    /* Configure the line coding based on initial settings */
    if (g_uart_cfg.stop_bits == UART_STOP_BITS_1)
        p_line_coding->b_char_format = 0;
    else if (g_uart_cfg.stop_bits == UART_STOP_BITS_2)
        p_line_coding->b_char_format = 2;

    if (g_uart_cfg.parity == UART_PARITY_OFF)
        p_line_coding->b_parity_type = 0;
    else if (g_uart_cfg.parity == UART_PARITY_ODD)
        p_line_coding->b_parity_type = 1;
    else if (g_uart_cfg.parity == UART_PARITY_EVEN)
        p_line_coding->b_parity_type = 2;

    if (p_uart_test_cfg->data_bits == UART_DATA_BITS_8)
        p_line_coding->b_data_bits = 8;
    else if (p_uart_test_cfg->data_bits == UART_DATA_BITS_7)
        p_line_coding->b_data_bits = 7;
    else if (p_uart_test_cfg->data_bits == UART_DATA_BITS_9)
        p_line_coding->b_data_bits = 9;

    /* Ideally put the baud rate into p_line_coding;
     * but FSP does not have an API to calculate the baud-rate
     * based on the UART configuration values */
    ;
}

/*******************************************************************************************************************/ /**
  *  @brief       Initialize UART config values based on user input values through serial terminal
  *  @param[in]   p_uart_test_cfg   Pointer to store UART configuration properties
                  p_line_coding     Updates the line coding member values
  *  @retval      None
  **********************************************************************************************************************/
static void set_uart_line_coding_cfg(uart_cfg_t *p_uart_test_cfg, const volatile usb_pcdc_linecoding_t *p_line_coding)
{
    /* Set number of parity bits */
    switch (p_line_coding->b_parity_type)
    {
    default:
        p_uart_test_cfg->parity = UART_PARITY_OFF;
        break;
    case 1:
        p_uart_test_cfg->parity = UART_PARITY_ODD;
        break;
    case 2:
        p_uart_test_cfg->parity = UART_PARITY_EVEN;
        break;
    }
    /* Set number of data bits */
    switch (p_line_coding->b_data_bits)
    {
    default:
    case 8:
        p_uart_test_cfg->data_bits = UART_DATA_BITS_8;
        break;
    case 7:
        p_uart_test_cfg->data_bits = UART_DATA_BITS_7;
        break;
    case 9:
        p_uart_test_cfg->data_bits = UART_DATA_BITS_9;
        break;
    }
    /* Set number of stop bits */
    switch (p_line_coding->b_char_format)
    {
    default:
        p_uart_test_cfg->stop_bits = UART_STOP_BITS_1;
        break;
    case 2:
        p_uart_test_cfg->stop_bits = UART_STOP_BITS_2;
        break;
    }
}

/*******************************************************************************************************************/ /**
  *  @brief     Pulse the LED high when UART traffic is happening.
  *  @param[IN] None

  * @retval    None
  **********************************************************************************************************************/
static void led_uart_activity(void)
{
    int32_t ledindex = LED_INDEX_VCOM;

    if (ledindex >= 0 && ledindex < g_bsp_leds.led_count)
    {
        R_BSP_PinWrite((bsp_io_port_pin_t)g_bsp_leds.p_leds[ledindex], BSP_IO_LEVEL_HIGH);
        R_BSP_PinWrite((bsp_io_port_pin_t)g_bsp_leds.p_leds[ledindex], BSP_IO_LEVEL_LOW);
    }
}

/*******************************************************************************************************************/ /**
 *  @brief       Closes the USB and UART module , Print and traps error.
 *  @param[IN]   status    error status
 *  @param[IN]   err_str   error string

 *  @retval      None
 **********************************************************************************************************************/
static void handle_error(fsp_err_t err, char *err_str)
{
    if (FSP_SUCCESS != err)
    {
        if (FSP_SUCCESS != R_USB_Close(&g_basic1_ctrl))
        {
            APP_ERR_PRINT("\r\n** R_USB_Close API Failed ** \r\n ");
        }

        if (FSP_SUCCESS != R_SCI_UART_Close(&g_uart_ctrl))
        {
            APP_ERR_PRINT("\r\n**  R_SCI_UART_Close API failed  ** \r\n");
        }

        APP_PRINT(err_str);
        APP_ERR_TRAP(err);
    }
}

/*******************************************************************************************************************/ /**
  * @brief     Function processes usb status request.
  * @param[IN] None

  * @retval    Any Other Error code apart from FSP_SUCCESS on Unsuccessful operation.
  **********************************************************************************************************************/
void dap_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    fsp_err_t err = FSP_SUCCESS;
    BaseType_t err_queue = pdFALSE;
    queue_evt_t q_instance;
    bsp_unique_id_t const *p_uid = R_BSP_UniqueIdGet();
    char g_print_buffer[33];

    /* Enabled permanently for DAP and Led activity. */
    R_BSP_PinAccessEnable();

    /* Update the USB Serial Number with the device UID */
    sprintf(g_print_buffer, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
            p_uid->unique_id_bytes[0], p_uid->unique_id_bytes[1],
            p_uid->unique_id_bytes[2], p_uid->unique_id_bytes[3],
            p_uid->unique_id_bytes[4], p_uid->unique_id_bytes[5],
            p_uid->unique_id_bytes[6], p_uid->unique_id_bytes[7],
            p_uid->unique_id_bytes[8], p_uid->unique_id_bytes[9],
            p_uid->unique_id_bytes[10], p_uid->unique_id_bytes[11],
            p_uid->unique_id_bytes[12], p_uid->unique_id_bytes[13],
            p_uid->unique_id_bytes[14], p_uid->unique_id_bytes[15]);

    for (uint8_t index = 0; index < MIN(sizeof(p_uid->unique_id_bytes), g_apl_string_descriptor_serial_number[0]); index++)
    {
        g_apl_string_descriptor_serial_number[2 + (index * 2)] = (uint8_t)g_print_buffer[index];
    }

    /* Open USB instance */
    err = R_USB_Open(&g_basic1_ctrl, &g_basic1_cfg);
    if (FSP_SUCCESS != err)
    {
        handle_error(err, "\r\nR_USB_Open failed.\r\n");
    }
    APP_PRINT("\r\nUSB Opened successfully.\n\r");

    /* Open the UART with initial configuration.*/
    memcpy(&g_uart_test_cfg, &g_uart_cfg, sizeof(g_uart_test_cfg));
    memcpy(&sci_extend_cfg, g_uart_cfg.p_extend, sizeof(sci_extend_cfg));
    g_uart_test_cfg.p_extend = &sci_extend_cfg;
    err = R_SCI_UART_Open(&g_uart_ctrl, &g_uart_test_cfg);
    if (FSP_SUCCESS != err)
    {
        handle_error(err, "\r\n**  R_SCI_UART_Open API failed  **\r\n");
    }

    /* First time Synchronization of the line coding between UART & USB */
    set_pcdc_line_coding(&g_line_coding, &g_uart_test_cfg);

    /* Initialize the semaphores to allow one synchronization event to occur */
    {
        BaseType_t err_semaphore = xSemaphoreGive(g_uart_tx_mutex);
        if (pdTRUE != err_semaphore)
        {
            handle_error(1, "\r\n xSemaphoreGive on g_uart_tx_mutex Failed \r\n");
        }

        err_semaphore = xSemaphoreGive(g_usb_tx_semaphore);
        if (pdTRUE != err_semaphore)
        {
            handle_error(1, "\r\n xSemaphoreGive on g_usb_tx_semaphore Failed \r\n");
        }
    }

    while (true)
    {
        /* Check if UART event is received */
        err_queue = xQueueReceive(g_uart_event_queue, (void *const)&q_instance, (portMAX_DELAY));
        if (pdTRUE != err_queue)
        {
            APP_ERR_PRINT("\r\nNo USB Event received. Please check USB connection \r\n");
        }
        else
        {
            if (PERIPHERAL_UART == q_instance.peripheral)
            {
                /* Data received on the UART interface, pass to USB host*/
                if (true == b_usb_attach)
                {
                    QueueHandle_t *p_queue = (2 == g_uart_ctrl.data_bytes) ? &g_usb_tx_x2_queue : &g_usb_tx_queue;
                    UBaseType_t msg_waiting_count = uxQueueMessagesWaiting(*p_queue);
                    volatile uint32_t rx_data_size = (2 == g_uart_ctrl.data_bytes) ? 2 : 1;

                    if (0U < msg_waiting_count)
                    {
                        /* Pull out as many item from queue as possible */
                        uint32_t unload_count = (msg_waiting_count < sizeof(usb_tx_buffer)) ? msg_waiting_count : sizeof(usb_tx_buffer);

                        /* Wait for previous USB transfer to complete */
                        BaseType_t err_semaphore = xSemaphoreTake(g_usb_tx_semaphore, portMAX_DELAY);
                        if (pdTRUE == err_semaphore)
                        {
                            for (uint32_t itr = 0, idx = 0; itr < unload_count; itr++, idx += rx_data_size)
                            {
                                if (pdTRUE != xQueueReceive(*p_queue, &usb_tx_buffer[idx], portMAX_DELAY))
                                {
                                    handle_error(1, "\r\n Did not receive expected count of characters \r\n");
                                }
                            }

                            /* Write data to host machine */
                            err = R_USB_Write(&g_basic1_ctrl, &usb_tx_buffer[0], (uint32_t)unload_count * rx_data_size, USB_CLASS_PCDC);
                            if (FSP_SUCCESS != err)
                            {
                                handle_error(err, "\r\nR_USB_Write API failed.\r\n");
                            }
                        }
                    }
                }
                continue;
            }
            if (PERIPHERAL_USB == q_instance.peripheral)
            {
                /* Data received from USB host, write out on the UART interface*/
                if (true == b_usb_attach)
                {
                    if (0U < q_instance.u.data_size)
                    {
                        /* Wait till previously queued data is out completely */
                        {
                            BaseType_t err_semaphore = xSemaphoreTake(g_uart_tx_mutex, portMAX_DELAY);

                            if (pdTRUE != err_semaphore)
                            {
                                handle_error(1, "\r\nxSemaphoreTake on g_uart_tx_mutex Failed \r\n");
                            }
                        }

                        err = R_SCI_UART_Write(&g_uart_ctrl, g_PCDC_rx_data, q_instance.u.data_size);
                        if (FSP_SUCCESS != err)
                        {
                            handle_error(err, "\r\n**  R_SCI_UART_Write API failed  **\r\n");
                        }
                    }
                    else
                    {
                        /* Buffer is physically transmitted since UART_EVENT_TX_COMPLETE was generated. */
                        /* Continue to read data from USB. */
                        /* The amount of data received will be known when USB_STATUS_READ_COMPLETE event occurs*/
                        err = R_USB_Read(&g_basic1_ctrl, g_PCDC_rx_data, CDC_DATA_LEN, USB_CLASS_PCDC);
                        if (FSP_SUCCESS != err)
                        {
                            handle_error(err, "\r\nR_USB_Read API failed.\r\n");
                        }
                    }
                }
                continue;
            }
        }
    }
}

/*******************************************************************************************************************/ /**
  * @brief       This function is callback for FreeRTOS+Composite.
  * @param[IN]   usb_event_info_t  *p_event_info
  * @param[IN]   usb_hdl_t         handler
  * @param[IN]   usb_onoff_t       on_off

  * @retval      None.
  **********************************************************************************************************************/
void usb_composite_callback(usb_event_info_t *p_event_info, usb_hdl_t handler, usb_onoff_t on_off)
{

    FSP_PARAMETER_NOT_USED(handler);
    FSP_PARAMETER_NOT_USED(on_off);
    queue_evt_t instance;
    fsp_err_t err = FSP_SUCCESS;

    /* USB event received */
    switch (p_event_info->event)
    {
    case USB_STATUS_CONFIGURED: /* Configured State */
    {
        APP_PRINT("USB Configured Successfully\r\n");
        /* Read data from serial port */
        err = R_USB_Read(&g_basic1_ctrl, g_PCDC_rx_data, CDC_DATA_LEN, USB_CLASS_PCDC);
        if (FSP_SUCCESS != err)
        {
            APP_ERR_PRINT("\r\nR_USB_Read (USB_CLASS_PCDC) failed.\r\n");
        }

        /* Read data from CMSIS-DAP PHID pipe */
        err = R_USB_Read(&g_basic1_ctrl, g_PHID_rcv_buf, 3, USB_CLASS_PHID);
        if (FSP_SUCCESS != err)
        {
            APP_ERR_PRINT("\r\nR_USB_Read (USB_CLASS_PHID) failed.\r\n");
        }

        break;
    }

    case USB_STATUS_WRITE_COMPLETE: /* Write Complete State */
    {
        if (b_usb_attach)
        {
            switch (p_event_info->type)
            {
            case USB_CLASS_PCDC:
            {
                /* Last write of UART data TO host machine has completed. */
                BaseType_t err_semaphore = xSemaphoreGive(g_usb_tx_semaphore);
                if (pdTRUE != err_semaphore)
                {
                    handle_error(1, "\r\n xSemaphoreGive on g_usb_tx_semaphore Failed \r\n");
                }
            }

            break;
            case USB_CLASS_PHID:
                /* Last write of CMSIS-DAP command response TO host machine has completed. */
                /* Read data from CMSIS-DAP PHID to accept next command */
                err = R_USB_Read(&g_basic1_ctrl, g_PHID_rcv_buf, DAP_PACKET_SIZE, USB_CLASS_PHID);
                if (FSP_SUCCESS != err)
                {
                    APP_ERR_PRINT("\r\nR_USB_Read (USB_CLASS_PHID) failed.\r\n");
                }
                break;
            default:
                break;
            }
            if (FSP_SUCCESS != err)
            {
                APP_ERR_PRINT("\r\nR_USB_Read API failed.\r\n");
            }
        }
        else
        {
            // Do Nothing as USB is removed and not connected yet.
        }
        break;
    }

    case USB_STATUS_READ_COMPLETE: /* Read Complete State */
    {
        if (b_usb_attach)
        {
            switch (p_event_info->type)
            {
            case USB_CLASS_PCDC:
                instance.peripheral = PERIPHERAL_USB;
                instance.u.data_size = p_event_info->data_size;

                /* Send the event from queue */
                if (pdTRUE != (xQueueSend(g_uart_event_queue, (const void *)&instance, (TickType_t)(RESET_VALUE))))
                {
                    handle_error(1, "\r\n xQueueSend on g_uart_event_queue Failed \r\n");
                }
                break;
            case USB_CLASS_PHID:
                /* CMSIS-DAP command from PC */
                DAP_ExecuteCommand(g_PHID_rcv_buf, g_PHID_tx_data);

                /* CMSIS-DAP command response to PC  */
                err = R_USB_Write(&g_basic1_ctrl, (uint8_t *)g_PHID_tx_data, DAP_PACKET_SIZE, USB_CLASS_PHID);
                if (FSP_SUCCESS != err)
                {
                    APP_ERR_PRINT("\r\nR_USB_Write API failed.\r\n");
                }
                break;
            default:
                break;
            }
        }
        else
        {
            // Do Nothing as USB is removed and not connected yet.
        }
        break;
    }

    case USB_STATUS_REQUEST: /* Receive Class Request */
    {
        /* Perform usb status request operation.*/
        /* Check for the specific CDC class request IDs */
        if (USB_PCDC_SET_LINE_CODING == (p_event_info->setup.request_type & USB_BREQUEST))
        {
            err = R_USB_PeriControlDataGet(&g_basic1_ctrl, (uint8_t *)&g_line_coding, LINE_CODING_LENGTH);
            if (FSP_SUCCESS == err)
            {
                /* Line Coding information read from the control pipe */
                sci_extend_cfg.p_baud_setting = &baud_setting;
                g_uart_test_cfg.p_extend = &sci_extend_cfg;

                /* Calculate the baud rate*/
                g_baud_rate = g_line_coding.dw_dte_rate;

                if (INVALID_SIZE < g_baud_rate)
                {
                    /* Calculate baud rate setting registers */
                    err = R_SCI_UART_BaudCalculate(g_baud_rate, enable_bitrate_modulation, error_rate_x_1000, &baud_setting);
                    if (FSP_SUCCESS != err)
                    {
                        handle_error(err, "\r\nR_SCI_UART_BaudCalculate failed.\r\n");
                    }
                }

                /* Set number of parity bits */
                set_uart_line_coding_cfg(&g_uart_test_cfg, &g_line_coding);
                /* Close module */
                err = R_SCI_UART_Close(&g_uart_ctrl);
                if (FSP_SUCCESS != err)
                {
                    APP_ERR_PRINT("\r\n**  R_SCI_UART_Close API failed  ** \r\n");
                }

                /* Open UART with changed UART settings */
                err = R_SCI_UART_Open(&g_uart_ctrl, &g_uart_test_cfg);
                if (FSP_SUCCESS != err)
                {
                    handle_error(err, "\r\nR_SCI_UART_Open failed.\r\n");
                }
            }
        }
        else if (USB_PCDC_GET_LINE_CODING == (p_event_info->setup.request_type & USB_BREQUEST))
        {
            /* Set the class request.*/
            err = R_USB_PeriControlDataSet(&g_basic1_ctrl, (uint8_t *)&g_line_coding, LINE_CODING_LENGTH);
            if (FSP_SUCCESS != err)
            {
                handle_error(err, "\r\nR_USB_PeriControlDataSet failed.\r\n");
            }
        }
        else if (USB_PCDC_SET_CONTROL_LINE_STATE == (p_event_info->setup.request_type & USB_BREQUEST))
        {
            /* Get the status of the control signals */
            err = R_USB_PeriControlDataGet(&g_basic1_ctrl,
                                           (uint8_t *)&g_control_line_state,
                                           sizeof(usb_pcdc_ctrllinestate_t));

            if (FSP_SUCCESS == err)
            {
                g_control_line_state.bdtr = (unsigned char)((p_event_info->setup.request_value >> 0) & 0x01);
                g_control_line_state.brts = (unsigned char)((p_event_info->setup.request_value >> 1) & 0x01);

                /* Toggle the line state if the flow control pin is set to a value (other than SCI_UART_INVALID_16BIT_PARAM) */
                if (SCI_UART_INVALID_16BIT_PARAM != g_uart_ctrl.flow_pin)
                {
                    R_BSP_PinWrite(g_uart_ctrl.flow_pin,
                                   (g_control_line_state.brts == 0) ? BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
                }
            }

            /* Set the usb status as ACK response.*/
            err = R_USB_PeriControlStatusSet(&g_basic1_ctrl, USB_SETUP_STATUS_ACK);
            if (FSP_SUCCESS != err)
            {
                handle_error(err, "\r\nR_USB_PeriControlDataSet failed.\r\n");
            }
        }
        else if (USB_GET_DESCRIPTOR == (p_event_info->setup.request_type & USB_BREQUEST))
        {
            /* check for request value */
            if (USB_GET_REPORT_DESCRIPTOR == p_event_info->setup.request_value)
            {
                err = R_USB_PeriControlDataSet(&g_basic1_ctrl, (uint8_t *)g_apl_report, USB_RECEIVE_REPORT_DESCRIPTOR);
                if (FSP_SUCCESS != err)
                {
                    handle_error(err, "\r\nR_USB_PeriControlDataSet failed.\r\n");
                }
            }
            else if (USB_GET_HID_DESCRIPTOR == p_event_info->setup.request_value)
            {
                for (uint8_t hid_des = RESET_VALUE; hid_des < USB_RECEIVE_HID_DESCRIPTOR; hid_des++)
                {
                    hid_send_data[hid_des] = g_apl_configuration[CD_LENGTH + hid_des];
                }

                /* Configuration Descriptor address set. */
                err = R_USB_PeriControlDataSet(&g_basic1_ctrl, hid_send_data, USB_RECEIVE_HID_DESCRIPTOR);
                if (FSP_SUCCESS != err)
                {
                    handle_error(err, "\r\nR_USB_PeriControlDataSet failed.\r\n");
                }
            }
        }
        else
        {
            // Do Nothing.
        }
        if (FSP_SUCCESS != err)
        {
            APP_ERR_PRINT("\r\nusb_status_request failed.\r\n");
        }
        break;
    }

    case USB_STATUS_REQUEST_COMPLETE: /* Request Complete State */
    {
        if (USB_PCDC_SET_LINE_CODING == (p_event_info->setup.request_type & USB_BREQUEST))
        {
            APP_PRINT("\nUSB STATUS : USB_STATUS_REQUEST_COMPLETE \nRequest_Type: USB_PCDC_SET_LINE_CODING \n");
        }
        else if (USB_PCDC_GET_LINE_CODING == (p_event_info->setup.request_type & USB_BREQUEST))
        {
            APP_PRINT("\nUSB STATUS : USB_STATUS_REQUEST_COMPLETE \nRequest_Type: USB_PCDC_GET_LINE_CODING \n");
        }
        else
        {
            // Do Nothing.
        }
        break;
    }

    case USB_STATUS_DETACH:
    case USB_STATUS_SUSPEND:
    {
        APP_PRINT("\nUSB STATUS : USB_STATUS_DETACH & USB_STATUS_SUSPEND\r\n");
        /* Reset the usb attached flag as indicating usb is removed.*/
        b_usb_attach = false;
        break;
    }
    case USB_STATUS_RESUME:
    {
        APP_PRINT("\nUSB STATUS : USB_STATUS_RESUME\r\n");
        /* set the usb attached flag*/
        b_usb_attach = true;
        break;
    }
    default:
    {
        break;
    }
    }
}

/*******************************************************************************************************************/ /**
  *  @brief      UART user callback
  *  @param[in]  p_args

  *  @retval     None
  **********************************************************************************************************************/
void user_uart_callback(uart_callback_args_t *p_args)
{
    queue_evt_t linstance;

    switch (p_args->event)
    {
    /* Receive complete event. */
    case UART_EVENT_RX_COMPLETE:
        break;
    /* Character received. */
    case UART_EVENT_RX_CHAR:
    {
        sci_uart_instance_ctrl_t const *const p_ctrl = p_args->p_context;

        linstance.peripheral = PERIPHERAL_UART;
        linstance.u.data_size = (2 == p_ctrl->data_bytes) ? 2 : 1;

        QueueHandle_t *p_queue = (2 == p_ctrl->data_bytes) ? &g_usb_tx_x2_queue : &g_usb_tx_queue;
        /* Add data to queue if we have space...discard latest bytes if not */
        if (!xQueueIsQueueFullFromISR(*p_queue))
        {
            if (pdTRUE != (xQueueSendFromISR(*p_queue, &p_args->data, NULL)))
            {
                handle_error(1, "\r\n xQueueSend on g_uart_event_queue Failed \r\n");
            }
        }
        /* Send the event from queue */
        if (!xQueueIsQueueFullFromISR(g_uart_event_queue))
        {
            if (pdTRUE != (xQueueSendFromISR(g_uart_event_queue, &linstance, NULL)))
            {
                handle_error(1, "\r\n xQueueSend on g_uart_event_queue Failed \r\n");
            }
        }

        led_uart_activity();
    }
    break;
    /* Last byte is transmitting, ready for more data. */
    case UART_EVENT_TX_DATA_EMPTY:
        break;
    /* Transmit complete event. */
    case UART_EVENT_TX_COMPLETE:
    {
        BaseType_t err_semaphore = xSemaphoreGiveFromISR(g_uart_tx_mutex, NULL);
        if (pdTRUE != err_semaphore)
        {
            handle_error(1, "\r\n xSemaphoreGiveFromISR Failed \r\n");
        }

        /* Indicate that all bytes were transferred over; and you're ready to listen again on USB. */
        linstance.peripheral = PERIPHERAL_USB;
        linstance.u.data_size = 0;

        if (pdTRUE != (xQueueSendFromISR(g_uart_event_queue, &linstance, NULL)))
        {
            handle_error(1, "\r\n xQueueSend on g_uart_event_queue Failed \r\n");
        }
        led_uart_activity();
    }
    break;

    case UART_EVENT_ERR_PARITY:
        /* Parity error event. */
        APP_PRINT("\r\n !! UART_EVENT_ERR_PARITY \r\n");
        break;

    case UART_EVENT_ERR_FRAMING:
        /* Mode fault error event.
        Likely to be a baud rate mismatch
        */
        APP_PRINT("\r\n !! UART_EVENT_ERR_FRAMING \r\n");
        break;

    case UART_EVENT_ERR_OVERFLOW:
        /* FIFO Overflow error event. */
        APP_PRINT("\r\n !! UART_EVENT_ERR_OVERFLOW \r\n");
        break;

    case UART_EVENT_BREAK_DETECT:
        /* Break detect error event. */
        APP_PRINT("\r\n !! UART_EVENT_BREAK_DETECT \r\n");
        break;

    default: /** Do Nothing */
        break;
    }
}
