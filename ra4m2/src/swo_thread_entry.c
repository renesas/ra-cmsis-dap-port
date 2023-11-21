#include "swo_thread.h"
#include "common_utils.h"
#include "usb_composite.h"
#include "CMSIS-DAP\Driver_USART.h"
#include "CMSIS-DAP\cmsis_os2.h"
#include "CMSIS-DAP\DAP.h"

static SWO_UART_STAT swo_uart_stat = {0};
static SWO_USB_STAT swo_usb_stat = {0};

static ARM_USART_SignalEvent_t ARM_USART_Initialize_cb_event = NULL;
static uint8_t *ARM_USART_Receive_data = NULL;
static uint32_t ARM_USART_Receive_recvd = 0x0;
static uint32_t ARM_USART_Receive_num = 0x0;
osThreadId_t SWO_ThreadId = 0x0AFEBABE;

#define UART_CAPTURE_BUFFER_SIZE 16384
#define INDEX_MASK (UART_CAPTURE_BUFFER_SIZE - 1)
static uint8_t g_swo_uartrx_buf[UART_CAPTURE_BUFFER_SIZE] = {};
static volatile uint32_t UartIndexI = 0x0; /* Incoming Uart Index */
static volatile uint32_t UartIndexO = 0x0; /* Outgoing Uart Index */

__NO_RETURN void SWO_Thread(void *argument);

/* SWO Thread entry function */
/* pvParameters contains TaskHandle_t */
void swo_thread_entry(void *pvParameters)
{
    SWO_Thread(pvParameters);
}

/*******************************************************************************************************************/ /**
* @brief  Process the captured UART RX buffer and hand over requested amount to DAP SWO code.
**********************************************************************************************************************/
void ProcessUartSwoQueue(void)
{
    uint32_t IndexI = UartIndexI;

    if (IndexI != UartIndexO)
    {
        uint32_t queued;
        uint32_t ARM_USART_Receive_remain = ARM_USART_Receive_num - ARM_USART_Receive_recvd;

        if (IndexI > UartIndexO)
        {
            queued = IndexI - UartIndexO;
        }
        else
        {
            queued = (UART_CAPTURE_BUFFER_SIZE - UartIndexO) + IndexI;
        }

        if (ARM_USART_Receive_remain && queued)
        {
            while ((UartIndexO != IndexI) && ARM_USART_Receive_remain)
            {
                ARM_USART_Receive_data[ARM_USART_Receive_recvd++] = g_swo_uartrx_buf[UartIndexO++];
                UartIndexO &= (UART_CAPTURE_BUFFER_SIZE - 1);
                ARM_USART_Receive_remain -= 1;
            }
            if (ARM_USART_Receive_remain == 0x0)
            {
                ARM_USART_Receive_recvd = 0x0;
                ARM_USART_Receive_num = 0x0;
                ARM_USART_Receive_data = NULL;
                if (ARM_USART_Initialize_cb_event)
                {
                    ARM_USART_Initialize_cb_event(ARM_USART_EVENT_RECEIVE_COMPLETE);
                }
            }
        }
    }
}

/*******************************************************************************************************************/ /**
  * @brief     The function osThreadFlagsSet sets the thread flags for a thread specified by parameter thread_id
  * The function osThreadFlagsWait suspends the execution of the currently RUNNING thread until any or all of the
  * thread flags specified with the parameter flags are set. When these thread flags are already set, the function
  * returns instantly. Otherwise the thread is put into the state BLOCKED.

  * @param[IN] flags	specifies the flags to wait for.
  * @param[IN] options	specifies flags options (osFlagsXxxx).
  * @param[IN] timeout	Timeout Value or 0 in case of no time-out..

  * @retval    thread flags after setting or error code if highest bit set.
  **********************************************************************************************************************/
uint32_t osThreadFlagsWait(uint32_t flags,
                           uint32_t options,
                           uint32_t timeout)
{
    TickType_t xTicksToWait;

    if ((flags != 1) || (options != osFlagsWaitAny))
    {
        return osFlagsErrorParameter;
    }

    if (timeout == osWaitForever)
    {
        xTicksToWait = portMAX_DELAY;
    }
    else
    {
        xTicksToWait = pdMS_TO_TICKS(timeout);
    }

    if (xSemaphoreTake(g_sem_SWO_Thread, xTicksToWait))
    {
        return 0x0;
    }
    else
    {
        return osFlagsErrorTimeout;
    }
}

/*******************************************************************************************************************/ /**
  * @brief     The function osThreadFlagsSet sets the thread flags for a thread specified by parameter thread_id
  * The target thread waiting for a flag to be set will resume from BLOCKED state.

  * @param[IN] thread_id	thread ID obtained by osThreadNew or osThreadGetId.
  * @param[IN] flags	specifies the flags of the thread that shall be set.

  * @retval    thread flags after setting or error code if highest bit set.
  **********************************************************************************************************************/
uint32_t osThreadFlagsSet(osThreadId_t thread_id, uint32_t flags)
{
    uint32_t ret = 0x0;

    if ((thread_id == SWO_ThreadId) && (flags == 0x1))
    {
        /* Only handling specific calls from SWO.C */
        (void)xSemaphoreGive(g_sem_SWO_Thread);
    }
    else
    {
        /* This is not a handled call. Update this function to cater for it */
        ret = osFlagsErrorParameter;
    }

    return ret;
}

/*******************************************************************************************************************/ /**
  *  @brief      UART SWO callback
  *  Queue up the bytes from the Renesas UART Driver that are going to be passed to the SWO driver (when it requests them).
  *
  *  @param[in]  p_args

  *  @retval     None
  **********************************************************************************************************************/
void swo_uart_callback(uart_callback_args_t *p_args)
{
    switch (p_args->event)
    {
    case UART_EVENT_RX_COMPLETE:
        break;
    case UART_EVENT_RX_CHAR:
    {

        if (((UartIndexI + 1) & INDEX_MASK) == UartIndexO)
        {
            swo_uart_stat.QUEUE_OVERFLOW += 1;
            if (ARM_USART_Initialize_cb_event)
            {
                ARM_USART_Initialize_cb_event(ARM_USART_EVENT_RX_OVERFLOW);
            }
        }
        else
        {
            g_swo_uartrx_buf[UartIndexI++] = (uint8_t)p_args->data;
            UartIndexI &= INDEX_MASK;
        }
    }
    break;
    case UART_EVENT_ERR_PARITY:

        if (ARM_USART_Initialize_cb_event)
        {
            ARM_USART_Initialize_cb_event(ARM_USART_EVENT_RX_PARITY_ERROR);
        }

        break;
    case UART_EVENT_ERR_FRAMING:
        if (ARM_USART_Initialize_cb_event)
        {
            ARM_USART_Initialize_cb_event(ARM_USART_EVENT_RX_FRAMING_ERROR);
        }

        break;
    case UART_EVENT_ERR_OVERFLOW:
        swo_uart_stat.HW_FIFO_OVERFLOW += 1;
        if (ARM_USART_Initialize_cb_event)
        {
            ARM_USART_Initialize_cb_event(ARM_USART_EVENT_RX_OVERFLOW);
        }
        break;
    case UART_EVENT_BREAK_DETECT:
        if (ARM_USART_Initialize_cb_event)
        {
            ARM_USART_Initialize_cb_event(ARM_USART_EVENT_RX_BREAK);
        }

        break;
    default: /** Do Nothing */
        break;
    }
}

// SWO Data Queue Transfer
//   buf:    pointer to buffer with data
//   num:    number of bytes to transfer
void SWO_QueueTransfer(uint8_t *buf, uint32_t num)
{
    SWO_USB_REQUEST swo_usb_request;

    swo_usb_request.buf = buf;
    swo_usb_request.num = num;

    if (pdTRUE != (xQueueSend(g_queue_swo_usb, (const void *)&swo_usb_request, (TickType_t)(RESET_VALUE))))
    {
        APP_ERR_PRINT("\r\n !! SWO_QueueTransfer xQueueSend failed. \r\n");
    }

    (void)xSemaphoreGive(g_sem_DAP_Thread);

    if (num > swo_usb_stat.LARGEST_TX)
    {
        swo_usb_stat.LARGEST_TX = num;
    }

    if (num < swo_usb_stat.SMALLEST_TX)
    {
        swo_usb_stat.SMALLEST_TX = num;
    }
}

// SWO Data Abort : Abort the SWO Transfer of SWO Data to the PC
void SWO_AbortTransfer(void)
{
    SWO_USB_REQUEST swo_usb_request;

    swo_usb_request.buf = NULL;
    swo_usb_request.num = 0x0;

    if (pdTRUE != (xQueueSend(g_queue_swo_usb, (const void *)&swo_usb_request, (TickType_t)(RESET_VALUE))))
    {
        APP_ERR_PRINT("\r\n !! SWO_AbortTransfer xQueueSend failed. \r\n");
    }
}

/*******************************************************************************************************************/
// !!!! The following sections contain a translation layer for the CMSIS USART Driver to Renesas functionality to
// enable the CMSIS DAP SWO.C to function ONLY!!!!
/*******************************************************************************************************************/

/*******************************************************************************************************************/ /**
  *  @brief Initialize USART Interface.
    The function ARM_USART_Initialize initializes the USART interface. It is called when the middleware component starts operation.
    The function performs the following operations:
        Initializes the resources needed for the USART interface.
        Registers the ARM_USART_SignalEvent callback function.

  *  @param[in]  cb_event	Pointer to ARM_USART_SignalEvent

  *  @retval     Status Error Codes
  **********************************************************************************************************************/
static int32_t ARM_USART_Initialize(ARM_USART_SignalEvent_t cb_event)
{
    memset(&swo_uart_stat, 0x0, sizeof(swo_uart_stat));
    swo_uart_stat.SMALLEST_RCV = 0xFFFFFFFF;
    swo_usb_stat.SMALLEST_TX = 0xFFFFFFFF;
    ARM_USART_Initialize_cb_event = cb_event;

    return ARM_DRIVER_OK;
}

/*******************************************************************************************************************/ /**
  *  @brief De-initialize USART Interface.
    The function ARM_USART_Uninitialize de-initializes the resources of USART interface.
    It is called when the middleware component stops operation and releases the software resources used by the interface.

  *  @retval     Status Error Codes
  **********************************************************************************************************************/
static int32_t ARM_USART_Uninitialize(void)
{
    int32_t ret = ARM_DRIVER_OK;

    ARM_USART_Initialize_cb_event = NULL;

    if (g_uart_swo_ctrl.open)
    {
        fsp_err_t err = FSP_SUCCESS;
        err = R_SCI_UART_Close(&g_uart_swo_ctrl);

        if (FSP_SUCCESS != err)
        {
            APP_ERR_PRINT("\r\n**  R_SCI_UART_Close API failed  %d** \r\n", err);
            ret = ARM_DRIVER_ERROR;
        }
    }

    return ret;
}

/*******************************************************************************************************************/ /**
  *  @brief Control USART Interface Power.

    The function ARM_USART_PowerControl operates the power modes of the USART interface.

    The parameter state sets the operation and can have the following values:

    ARM_POWER_FULL : set-up peripheral for data transfers, enable interrupts (NVIC) and optionally DMA. Can be called multiple times. If the peripheral is already in this mode the function performs no operation and returns with ARM_DRIVER_OK.
    ARM_POWER_LOW : may use power saving. Returns ARM_DRIVER_ERROR_UNSUPPORTED when not implemented.
    ARM_POWER_OFF : terminates any pending data transfers, disables peripheral, disables related interrupts and DMA.

  *  @param[in]  state	Power state

  *  @retval     Status Error Codes
  **********************************************************************************************************************/
static int32_t ARM_USART_PowerControl(ARM_POWER_STATE state)
{
    switch (state)
    {
    case ARM_POWER_OFF:
        break;
    case ARM_POWER_LOW:
        break;
    case ARM_POWER_FULL:
        break;
    }
    return ARM_DRIVER_OK;
}

/*******************************************************************************************************************/ /**
  *  @brief Start receiving data from USART receiver.

    This functions ARM_USART_Receive is used in asynchronous mode to receive data from the USART receiver. It can also be used in synchronous mode when receiving data only (transmits the default value as specified by ARM_USART_Control with ARM_USART_SET_DEFAULT_TX_VALUE as control parameter).

    Receiver needs to be enabled by calling ARM_USART_Control with ARM_USART_CONTROL_RX as the control parameter and 1 as argument.

    The function parameters specify the buffer for data and the number of items to receive. The item size is defined by the data type which depends on the configured number of data bits.

    Data type is:

    uint8_t when configured for 5..8 data bits
    uint16_t when configured for 9 data bits
    Calling the function ARM_USART_Receive only starts the receive operation. The function is non-blocking and returns as soon as the driver has started the operation (driver typically configures DMA or the interrupt system for continuous transfer). When in synchronous slave mode the operation is only registered and started when the master starts the transfer. During the operation it is not allowed to call this function again or any other data transfer function when in synchronous mode. Also the data buffer must stay allocated. When receive operation is completed (requested number of items received) the ARM_USART_EVENT_RECEIVE_COMPLETE event is generated. Progress of receive operation can also be monitored by reading the number of items already received by calling ARM_USART_GetRxCount.

    Status of the receiver can be monitored by calling the ARM_USART_GetStatus and checking the rx_busy flag which indicates if reception is still in progress.

    During reception the following events can be generated (in asynchronous mode):

    ARM_USART_EVENT_RX_TIMEOUT : Receive timeout between consecutive characters detected (optional)
    ARM_USART_EVENT_RX_BREAK : Break detected (Framing error is not generated for Break condition)
    ARM_USART_EVENT_RX_FRAMING_ERROR : Framing error detected
    ARM_USART_EVENT_RX_PARITY_ERROR : Parity error detected
    ARM_USART_EVENT_RX_OVERFLOW : Data overflow detected (also in synchronous slave mode)
    ARM_USART_EVENT_RX_OVERFLOW event is also generated when receiver is enabled but data is lost because receive operation in asynchronous mode or receive/send/transfer operation in synchronous slave mode has not been started.

    Receive operation can be aborted by calling ARM_USART_Control with ARM_USART_ABORT_RECEIVE as the control parameter.


  *  @param[out]  data	Pointer to buffer for data to receive from USART receiver
  *  @param[in]  num	Number of data items to receive
  *  @retval     Status Error Codes
  **********************************************************************************************************************/
static int32_t ARM_USART_Receive(void *data, uint32_t num)
{
    /* SWO Driver is requesting 'num' SWO bytes. We will callback with 'ARM_USART_EVENT_RECEIVE_COMPLETE' when received. */
    ARM_USART_Receive_data = (uint8_t *)data;
    ARM_USART_Receive_recvd = 0x0;
    ARM_USART_Receive_num = num;
    if (num > swo_uart_stat.LARGEST_RCV)
    {
        swo_uart_stat.LARGEST_RCV = num;
    }
    if (num < swo_uart_stat.SMALLEST_RCV)
    {
        swo_uart_stat.SMALLEST_RCV = num;
    }
    return ARM_DRIVER_OK;
}

/*******************************************************************************************************************/ /**
  *  @brief Get received data count.
     The function ARM_USART_GetRxCount returns the number of the currently received data items
     during ARM_USART_Receive and ARM_USART_Transfer operation.

  *  @retval     number of data items received
  **********************************************************************************************************************/
static uint32_t ARM_USART_GetRxCount(void)
{
    return ARM_USART_Receive_recvd;
}

/*******************************************************************************************************************/ /**
  *  @brief Control USART Interface.

    The function ARM_USART_Control control the USART interface settings and execute various operations.

    The parameter control sets the operation and is explained in the table below. Values from different categories can be ORed with the exception of Miscellaneous Operations.

    The parameter arg provides, depending on the operation, additional information, for example the baudrate.

  *  @param[in]  control	Operation
  *  @param[in]  arg	Argument of operation (optional)

  *  @retval     Status Error Codes
  **********************************************************************************************************************/
static int32_t ARM_USART_Control(uint32_t control, uint32_t arg)
{
    int32_t ret = ARM_DRIVER_OK;
    static baud_setting_t swo_baud_setting;
    static uart_cfg_t swo_uart_local_cfg;
    static sci_uart_extended_cfg_t swo_sci_extend_cfg;
    fsp_err_t err = FSP_SUCCESS;

    switch (control)
    {
    case ARM_USART_CONTROL_RX:
        // Enable or disable receiver; arg : 0=disabled; 1=enabled
        break;
    case ARM_USART_ABORT_RECEIVE:
        // Abort receive operation
        break;
    case ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | ARM_USART_STOP_BITS_1:
        // Set to asynchronous UART mode. arg specifies baudrate.
        memcpy(&swo_uart_local_cfg, &g_uart_swo_cfg, sizeof(swo_uart_local_cfg));
        memcpy(&swo_sci_extend_cfg, g_uart_swo_cfg.p_extend, sizeof(swo_sci_extend_cfg));
        swo_uart_local_cfg.p_extend = &swo_sci_extend_cfg;
        swo_sci_extend_cfg.p_baud_setting = &swo_baud_setting;

        /* Calculate baud rate setting registers */
        err = R_SCI_UART_BaudCalculate(arg, true, BAUD_ERROR_RATE, &swo_baud_setting);
        if (FSP_SUCCESS != err)
        {
            ret = ARM_DRIVER_ERROR;
        }
        else
        {
            err = R_SCI_UART_Open(&g_uart_swo_ctrl, &swo_uart_local_cfg);
            if (FSP_SUCCESS != err)
            {
                ret = ARM_DRIVER_ERROR;
            }
        }
        break;
    default:
        /* We only support the limited need of the CMSIS-DAP SWO */
        ret = ARM_DRIVER_ERROR_UNSUPPORTED;
        break;
    }
    return ret;
}

/*******************************************************************************************************************/ /**
  *  @brief Get USART status.

    The function ARM_USART_GetStatus retrieves the current USART interface status.

  *  @retval     USART status ARM_USART_STATUS
  **********************************************************************************************************************/
static ARM_USART_STATUS ARM_USART_GetStatus(void)
{
    ARM_USART_STATUS status;
    status.tx_busy = 0;
    status.rx_busy = 1;
    status.tx_underflow = 0;
    status.rx_overflow = 0;
    status.rx_break = 0;
    status.rx_framing_error = 0;
    status.rx_parity_error = 0;
    return status;
}

// End USART Interface

extern ARM_DRIVER_USART Driver_USART0;
ARM_DRIVER_USART Driver_USART0 = {
    NULL,
    NULL,
    ARM_USART_Initialize,
    ARM_USART_Uninitialize,
    ARM_USART_PowerControl,
    NULL,
    ARM_USART_Receive,
    NULL,
    NULL,
    ARM_USART_GetRxCount,
    ARM_USART_Control,
    ARM_USART_GetStatus,
    NULL,
    NULL};
