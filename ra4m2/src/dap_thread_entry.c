#include "dap_thread.h"
/* DAP Thread entry function */
/* pvParameters contains TaskHandle_t */
#include "common_utils.h"
#include "usb_composite.h"
#include "CMSIS-DAP\DAP_config.h"
#include "CMSIS-DAP\DAP.h"
#include "r_usb_pcdc_cfg.h"
/**********************************************************************************************************************
 * @addtogroup usb_composite_ep
 * @{
 **********************************************************************************************************************/

#define UART_TXING 0x1
#define UART_RXING 0x2

/* external variables*/
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_report[];
extern bsp_leds_t g_bsp_leds;
extern uint8_t g_apl_string_descriptor_serial_number[];
extern uint8_t ExtendedPropertiesDescriptor[];
extern tyRAM4ECID ecd;

/* Local Module Variables */
static bool b_usb_configured = false;
static usb_pcdc_linecoding_t g_line_coding;

static volatile uint16_t USB_RequestIndexI; // Request Index In
static volatile uint16_t USB_RequestIndexO; // Request Index Out
static volatile uint16_t USB_RequestCountI; // Request Count In
static volatile uint16_t USB_RequestCountO; // Request Count Out
static volatile uint8_t USB_RequestIdle;    // Request Idle  Flag

static volatile uint16_t USB_ResponseIndexI; // Response Index In
static volatile uint16_t USB_ResponseIndexO; // Response Index Out
static volatile uint16_t USB_ResponseCountI; // Response Count In
static volatile uint16_t USB_ResponseCountO; // Response Count Out
static volatile uint8_t USB_ResponseIdle;    // Response Idle  Flag

static uint8_t USB_Request[DAP_PACKET_COUNT][DAP_PACKET_SIZE];
static uint8_t USB_Response[DAP_PACKET_COUNT][DAP_PACKET_SIZE];
static uint16_t USB_RespSize[DAP_PACKET_COUNT];

static volatile uint16_t PCDC_ToTargetIndexI; // Request Index In
static volatile uint16_t PCDC_ToTargetIndexO; // Request Index Out
static volatile uint16_t PCDC_ToTargetCountI; // Request Count In
static volatile uint16_t PCDC_ToTargetCountO; // Request Count Out
static volatile uint8_t PCDC_ToTargetIdle;    // Request Idle  Flag

static uint8_t PCDC_ToTarget[UART_PACKET_COUNT][CDC_DATA_LEN];
static uint16_t PCDC_ToTargetSize[UART_PACKET_COUNT];

static uint8_t g_buf_uart_tx[CDC_DATA_LEN];

static usb_pcdc_ctrllinestate_t g_control_line_state = {
    .bdtr = 0,
    .brts = 0,
};

/* private function declarations */
static void handle_error(fsp_err_t err, char *err_str);
static void set_pcdc_line_coding(volatile usb_pcdc_linecoding_t *p_line_coding, const uart_cfg_t *p_uart_test_cfg);
static void set_uart_line_coding_cfg(uart_cfg_t *p_uart_test_cfg, const volatile usb_pcdc_linecoding_t *p_line_coding);
static fsp_err_t process_usb_events(usb_event_info_t *p_event_info);

static baud_setting_t baud_setting;
static bool enable_bitrate_modulation = true;

static uint32_t g_baud_rate = RESET_VALUE;
static uint32_t error_rate_x_1000 = BAUD_ERROR_RATE;
static uart_cfg_t g_uart_test_cfg;
static sci_uart_extended_cfg_t sci_extend_cfg;
static uint32_t g_uart_activity = 0x0;
static uint8_t g_PCDC_tx_data[CDC_DATA_LEN];

#define START_PIPE (USB_PIPE1)   // Start pipe number
#define END_PIPE (USB_PIPE9 + 1) // Total pipe

uint8_t cmd_pipe = END_PIPE;
uint8_t rsp_pipe = END_PIPE;
uint8_t swo_pipe = END_PIPE;

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
    usb_event_info_t *p_event_info;

    bsp_unique_id_t const *p_uid = R_BSP_UniqueIdGet();
    char g_print_buffer[33];

    /* Enabled permanently for DAP and Led activity. */
    R_BSP_PinAccessEnable();

    if (g_apl_string_descriptor_serial_number[0] >= 4)
    {
        uint32_t maxIndex = (uint32_t)((g_apl_string_descriptor_serial_number[0] - 2) / 2);

        if (maxIndex > sizeof(p_uid->unique_id_bytes))
        {
            /* Remaining id bytes are fixed */
            maxIndex = sizeof(p_uid->unique_id_bytes);
        }
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

        for (uint8_t index = 0; index < maxIndex; index++)
        {
            g_apl_string_descriptor_serial_number[2 + (index * 2)] = (uint8_t)g_print_buffer[index];
        }
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
        BaseType_t err_semaphore = xSemaphoreGive(g_sem_uart_tx);
        if (pdTRUE != err_semaphore)
        {
            handle_error(1, "\r\n xSemaphoreGive on g_sem_uart_tx Failed \r\n");
        }

        err_semaphore = xSemaphoreGive(g_sem_pcdc_tx);
        if (pdTRUE != err_semaphore)
        {
            handle_error(1, "\r\n xSemaphoreGive on g_sem_pcdc_tx Failed \r\n");
        }
    }

    while (true)
    {
        if (xQueueReceive(g_queue_usb_event, &p_event_info, 0x0))
        {
            uint32_t n;
            err = process_usb_events(p_event_info);

            // Process pending requests
            while (USB_RequestCountI != USB_RequestCountO)
            {

                // Handle Queue Commands
                n = USB_RequestIndexO;
                while (USB_Request[n][0] == ID_DAP_QueueCommands)
                {
                    USB_Request[n][0] = ID_DAP_ExecuteCommands;
                    n++;
                    if (n == DAP_PACKET_COUNT)
                    {
                        n = 0U;
                    }
                    if (n == USB_RequestIndexI)
                    {
                        break;
                    }
                }

                // Execute DAP Command (process request and prepare response)
                USB_RespSize[USB_ResponseIndexI] =
                    (uint16_t)DAP_ExecuteCommand(USB_Request[USB_RequestIndexO], USB_Response[USB_ResponseIndexI]);

                // Update Request Index and Count
                USB_RequestIndexO++;
                if (USB_RequestIndexO == DAP_PACKET_COUNT)
                {
                    USB_RequestIndexO = 0U;
                }
                USB_RequestCountO++;

                if (USB_RequestIdle)
                {
                    if ((uint16_t)(USB_RequestCountI - USB_RequestCountO) != DAP_PACKET_COUNT)
                    {
                        USB_RequestIdle = 0U;
                        err = R_USB_PipeRead(&g_basic1_ctrl, USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE, cmd_pipe);
                        if (FSP_SUCCESS != err)
                        {
                            APP_ERR_PRINT("\r\nR_USB_PipeRead (USB_CLASS_PVND) failed.\r\n");
                        }
                    }
                }

                // Update Response Index and Count
                USB_ResponseIndexI++;
                if (USB_ResponseIndexI == DAP_PACKET_COUNT)
                {
                    USB_ResponseIndexI = 0U;
                }
                USB_ResponseCountI++;

                if (USB_ResponseIdle)
                {
                    if (USB_ResponseCountI != USB_ResponseCountO)
                    {
                        // Load data from response buffer to be sent back
                        n = USB_ResponseIndexO++;
                        if (USB_ResponseIndexO == DAP_PACKET_COUNT)
                        {
                            USB_ResponseIndexO = 0U;
                        }
                        USB_ResponseCountO++;
                        USB_ResponseIdle = 0U;
                        /* CMSIS-DAP command response to PC  */
                        err = R_USB_PipeWrite(&g_basic1_ctrl, USB_Response[n], USB_RespSize[n], rsp_pipe);
                        if (FSP_SUCCESS != err)
                        {
                            APP_ERR_PRINT("\r\nR_USB_PipeWrite API failed.\r\n");
                        }
                    }
                }
            }
        }

        /* Check if UART TX data has received from PC */
        if (PCDC_ToTargetCountI != PCDC_ToTargetCountO)
        {
            uint32_t n;
            g_uart_activity |= UART_TXING;
            if (xSemaphoreTake(g_sem_uart_tx, 0))
            {
                g_uart_activity |= UART_TXING;
                n = PCDC_ToTargetIndexO++;
                memcpy(g_buf_uart_tx, PCDC_ToTarget[n], PCDC_ToTargetSize[n]);
                err = R_SCI_UART_Write(&g_uart_ctrl, g_buf_uart_tx, PCDC_ToTargetSize[n]);
                if (FSP_SUCCESS != err)
                {
                    handle_error(err, "\r\n**  R_SCI_UART_Write API failed  **\r\n");
                }

                if (PCDC_ToTargetIndexO == UART_PACKET_COUNT)
                {
                    PCDC_ToTargetIndexO = 0U;
                }
                PCDC_ToTargetCountO++;

                if (PCDC_ToTargetIdle)
                {
                    if ((uint16_t)(PCDC_ToTargetCountI - PCDC_ToTargetCountO) != UART_PACKET_COUNT)
                    {
                        PCDC_ToTargetIdle = 0U;
                        err = R_USB_Read(&g_basic1_ctrl, PCDC_ToTarget[PCDC_ToTargetIndexI], CDC_DATA_LEN, USB_CLASS_PCDC);
                        if (FSP_SUCCESS != err)
                        {
                            APP_ERR_PRINT("\r\nR_USB_Read (USB_CLASS_PCDC) failed.\r\n");
                        }
                    }
                }
            }
        }
        else
        {
            g_uart_activity &= (uint32_t)~UART_TXING;
        }

        /* Check if UART RX data has received from target MCU */
        if (true == b_usb_configured)
        {
            QueueHandle_t *p_queue = (2 == g_uart_ctrl.data_bytes) ? &g_queue_uart_tx_16 : &g_queue_uart_tx_8;
            UBaseType_t msg_waiting_count = uxQueueMessagesWaiting(*p_queue);
            volatile uint32_t rx_data_size = (2 == g_uart_ctrl.data_bytes) ? 2 : 1;

            if (msg_waiting_count)
            {
                g_uart_activity |= UART_RXING;
            }
            else
            {
                g_uart_activity &= (uint32_t)~UART_RXING;
            }

            if (msg_waiting_count && xSemaphoreTake(g_sem_pcdc_tx, 0))
            {

                /* Pull out as many item from queue as possible */
                uint32_t unload_count = (msg_waiting_count < sizeof(g_PCDC_tx_data)) ? msg_waiting_count : sizeof(g_PCDC_tx_data);
                for (uint32_t itr = 0, idx = 0; itr < unload_count; itr++, idx += rx_data_size)
                {
                    if (pdTRUE != xQueueReceive(*p_queue, &g_PCDC_tx_data[idx], portMAX_DELAY))
                    {
                        handle_error(1, "\r\n Did not receive expected count of characters \r\n");
                    }
                }

                err = R_USB_Write(&g_basic1_ctrl, &g_PCDC_tx_data[0], unload_count * rx_data_size, USB_CLASS_PCDC);
                if (FSP_SUCCESS != err)
                {
                    handle_error(err, "\r\nR_USB_Write API failed.\r\n");
                }
            }
        }

        R_BSP_PinWrite((bsp_io_port_pin_t)g_bsp_leds.p_leds[LED_INDEX_VCOM],
                       g_uart_activity != 0 ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);

        vTaskDelay(1);
    }
}

static fsp_err_t process_usb_events(usb_event_info_t *p_event_info)
{
    fsp_err_t err = FSP_SUCCESS;
    uint16_t used_pipe = 0;
    uint8_t pipe = 0;
    usb_pipe_t pipe_info;

    /* USB event received */
    switch (p_event_info->event)
    {
    case USB_STATUS_CONFIGURED: /* Configured State */
    {
        APP_PRINT("USB Configured Successfully\r\n");
        R_USB_UsedPipesGet(&g_basic1_ctrl, &used_pipe, USB_CLASS_PVND);
        for (pipe = START_PIPE; pipe < END_PIPE; pipe++)
        {
            if (0 != (used_pipe & (1 << pipe)))
            {
                R_USB_PipeInfoGet(&g_basic1_ctrl, &pipe_info, pipe);
                if (USB_EP_DIR_IN != (pipe_info.endpoint & USB_EP_DIR_IN))
                {
                    /* DAP Command Pipe */
                    if (USB_TRANSFER_TYPE_BULK == pipe_info.transfer_type)
                    {
                        if ((pipe != USB_CFG_PCDC_BULK_OUT) && (cmd_pipe == END_PIPE))
                        {
                            cmd_pipe = pipe;
                            APP_PRINT("\r\ncmd_pipe Pipe Number: %d Endpoint:0x%02X ", pipe, pipe_info.endpoint);
                        }
                    }
                }
                else
                {
                    /* DAP Command Response Pipe */
                    if (USB_TRANSFER_TYPE_BULK == pipe_info.transfer_type)
                    {
                        if ((pipe != USB_CFG_PCDC_BULK_IN) && (rsp_pipe == END_PIPE))
                        {
                            rsp_pipe = pipe;
                            APP_PRINT("\r\nrsp_pipe Pipe Number: %d Endpoint:0x%02X ", pipe, pipe_info.endpoint);
                        }
                        else if ((pipe != USB_CFG_PCDC_BULK_IN) && (swo_pipe == END_PIPE))
                        {
                            swo_pipe = pipe;
                            APP_PRINT("\r\nswo_pipe Pipe Number: %d Endpoint:0x%02X ", pipe, pipe_info.endpoint);
                        }
                    }
                }
            }
        }

        // Initialize variables
        USB_RequestIndexI = 0U;
        USB_RequestIndexO = 0U;
        USB_RequestCountI = 0U;
        USB_RequestCountO = 0U;
        USB_RequestIdle = 1U;

        USB_ResponseIndexI = 0U;
        USB_ResponseIndexO = 0U;
        USB_ResponseCountI = 0U;
        USB_ResponseCountO = 0U;
        USB_ResponseIdle = 1U;

        PCDC_ToTargetIndexI = 0U;
        PCDC_ToTargetIndexO = 0U;
        PCDC_ToTargetCountI = 0U;
        PCDC_ToTargetCountO = 0U;
        PCDC_ToTargetIdle = 1U;

        /* Read data from CMSIS-DAP Command pipe */
        USB_RequestIdle = 0U;
        err = R_USB_PipeRead(&g_basic1_ctrl, USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE, cmd_pipe);
        if (FSP_SUCCESS != err)
        {
            APP_ERR_PRINT("\r\nR_USB_PipeRead (USB_CLASS_PVND) failed.\r\n");
        }

        /* Read data from serial port */
        PCDC_ToTargetIdle = 0U;
        err = R_USB_Read(&g_basic1_ctrl, PCDC_ToTarget[PCDC_ToTargetIndexI], CDC_DATA_LEN, USB_CLASS_PCDC);
        if (FSP_SUCCESS != err)
        {
            APP_ERR_PRINT("\r\nR_USB_Read (USB_CLASS_PCDC) failed.\r\n");
        }

        b_usb_configured = true;
        break;
    }

    case USB_STATUS_WRITE_COMPLETE: /* Write Complete State */
    {
        if (b_usb_configured)
        {
            switch (p_event_info->type)
            {
            case USB_CLASS_PCDC:
            {
                BaseType_t err_semaphore = xSemaphoreGive(g_sem_pcdc_tx);
                if (pdTRUE != err_semaphore)
                {
                    handle_error(1, "\r\n xSemaphoreGive on g_sem_pcdc_tx Failed \r\n");
                }
            }
            break;
            default:
            {
                if (rsp_pipe == p_event_info->pipe)
                {
                    /* Last write of CMSIS-DAP command response TO host machine has completed.
                       If we have another available, then that can be sent now otherwise we
                       set USB_ResponseIdle to indicate that we are free to send the next response
                       once one becomes available
                    */
                    if (USB_ResponseCountI != USB_ResponseCountO)
                    {
                        // Load data from response buffer to be sent back
                        err = R_USB_PipeWrite(&g_basic1_ctrl, USB_Response[USB_ResponseIndexO], USB_RespSize[USB_ResponseIndexO], rsp_pipe);
                        if (FSP_SUCCESS != err)
                        {
                            APP_ERR_PRINT("\r\nR_USB_PipeWrite API failed.\r\n");
                        }

                        USB_ResponseIndexO++;
                        if (USB_ResponseIndexO == DAP_PACKET_COUNT)
                        {
                            USB_ResponseIndexO = 0U;
                        }
                        USB_ResponseCountO++;
                    }
                    else
                    {
                        USB_ResponseIdle = 1U;
                    }
                }
            }
            break;
            }
            if (FSP_SUCCESS != err)
            {
                APP_ERR_PRINT("\r\nR_USB_Read API failed.\r\n");
            }
        }
        break;
    }

    case USB_STATUS_READ_COMPLETE:
    {
        if (b_usb_configured)
        {
            switch (p_event_info->type)
            {
            case USB_CLASS_PCDC:
                PCDC_ToTargetSize[PCDC_ToTargetIndexI] = (uint16_t)p_event_info->data_size;
                PCDC_ToTargetIndexI++;
                if (PCDC_ToTargetIndexI == UART_PACKET_COUNT)
                {
                    PCDC_ToTargetIndexI = 0U;
                }
                PCDC_ToTargetCountI++;

                if ((uint16_t)(PCDC_ToTargetCountI - PCDC_ToTargetCountO) != UART_PACKET_COUNT)
                {
                    err = R_USB_Read(&g_basic1_ctrl, PCDC_ToTarget[PCDC_ToTargetIndexI], CDC_DATA_LEN, USB_CLASS_PCDC);
                    if (FSP_SUCCESS != err)
                    {
                        APP_ERR_PRINT("\r\nR_USB_Read (USB_CLASS_PCDC) failed.\r\n");
                    }
                }
                else
                {
                    PCDC_ToTargetIdle = 1U;
                }

                break;
            default:
                if (cmd_pipe == p_event_info->pipe)
                {
                    if (USB_Request[USB_RequestIndexI][0] == ID_DAP_TransferAbort)
                    {
                        DAP_TransferAbort = 1U;
                    }
                    else
                    {
                        USB_RequestIndexI++;
                        if (USB_RequestIndexI == DAP_PACKET_COUNT)
                        {
                            USB_RequestIndexI = 0U;
                        }
                        USB_RequestCountI++;
                    }
                }
                // Start reception of next request packet
                if ((uint16_t)(USB_RequestCountI - USB_RequestCountO) != DAP_PACKET_COUNT)
                {
                    err = R_USB_PipeRead(&g_basic1_ctrl, USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE, cmd_pipe);
                    if (FSP_SUCCESS != err)
                    {
                        APP_ERR_PRINT("\r\nR_USB_PipeRead (USB_CLASS_PVND) failed.\r\n");
                    }
                }
                else
                {
                    USB_RequestIdle = 1U;
                }

                break;
            }
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
        }
        else if (USB_VENDOR_GET_MS_DESCRIPTOR_DEVICE == p_event_info->setup.request_type)
        {
            if (p_event_info->setup.request_index == EXT_COMPATID_OS_DESCRIPTOR)
            {
                /* Ignore interface number for the device */
                err = R_USB_PeriControlDataSet(&g_basic1_ctrl, (uint8_t *)&ecd, p_event_info->setup.request_length);
                if (FSP_SUCCESS != err)
                {
                    handle_error(err, "\r\nR_USB_PeriControlDataSet failed.\r\n");
                }
            }
        }
        else if (USB_VENDOR_GET_MS_DESCRIPTOR_INTERFACE == p_event_info->setup.request_type)
        {
            if (p_event_info->setup.request_index == EXT_PROP_OS_DESCRIPTOR)
            {
                if ((p_event_info->setup.request_value & 0x00FF) == INTERFACE_CMSIS_DAP)
                {
                    err = R_USB_PeriControlDataSet(&g_basic1_ctrl, ExtendedPropertiesDescriptor, p_event_info->setup.request_length);
                    if (FSP_SUCCESS != err)
                    {
                        handle_error(err, "\r\nR_USB_PeriControlDataSet failed.\r\n");
                    }
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
        b_usb_configured = false;
        break;
    }
    case USB_STATUS_RESUME:
    {
        APP_PRINT("\nUSB STATUS : USB_STATUS_RESUME\r\n");
        break;
    }
    default:
    {
        break;
    }
    }
    return err;
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

    if (pdTRUE != (xQueueSend(g_queue_usb_event, (const void *)&p_event_info, (TickType_t)(RESET_VALUE))))
    {
    }
}

/*******************************************************************************************************************/ /**
  *  @brief      UART user callback
  *  @param[in]  p_args

  *  @retval     None
  **********************************************************************************************************************/
void user_uart_callback(uart_callback_args_t *p_args)
{
    switch (p_args->event)
    {
    case UART_EVENT_RX_COMPLETE:
        break;
    case UART_EVENT_RX_CHAR:
    {
        sci_uart_instance_ctrl_t const *const p_ctrl = p_args->p_context;
        QueueHandle_t *p_queue = (2 == p_ctrl->data_bytes) ? &g_queue_uart_tx_16 : &g_queue_uart_tx_8;
        if (pdTRUE != (xQueueSendFromISR(*p_queue, &p_args->data, NULL)))
        {
            handle_error(1, "\r\n xQueueSend on g_queue_uart_tx Failed \r\n");
        }
    }
    break;
    /* Last byte is transmitting, ready for more data. */
    case UART_EVENT_TX_DATA_EMPTY:
        break;
    case UART_EVENT_TX_COMPLETE:
    {
        BaseType_t err_semaphore = xSemaphoreGiveFromISR(g_sem_uart_tx, NULL);
        if (pdTRUE != err_semaphore)
        {
            handle_error(1, "\r\n xSemaphoreGiveFromISR Failed \r\n");
        }
    }
    break;
    case UART_EVENT_ERR_PARITY:
        APP_PRINT("\r\n !! UART_EVENT_ERR_PARITY \r\n");
        break;
    case UART_EVENT_ERR_FRAMING:
        APP_PRINT("\r\n !! UART_EVENT_ERR_FRAMING \r\n");
        break;
    case UART_EVENT_ERR_OVERFLOW:
        APP_PRINT("\r\n !! UART_EVENT_ERR_OVERFLOW \r\n");
        break;
    case UART_EVENT_BREAK_DETECT:
        APP_PRINT("\r\n !! UART_EVENT_BREAK_DETECT \r\n");
        break;
    default: /** Do Nothing */
        break;
    }
}
