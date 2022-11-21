/***********************************************************
*  File: tuya_uart.c
*  Author: nzy
*  Date: 20171106
***********************************************************/
#include <string.h>
#include "mem_pool.h"
#include "tuya_uart.h"
#include "uni_log.h"
#include "tuya_hal_semaphore.h"

#include "drv_model_pub.h"
#include "uart_pub.h"
#include "BkDriverUart.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#define __TUYA_UART_GLOBALS

/***********************************************************
*************************variable define********************
***********************************************************/
typedef struct {
    UINT_T buf_len;
    BYTE_T *buf;
    USHORT_T in;
    USHORT_T out;
    BOOL_T unblock;
    SEM_HANDLE uart_sem;
    BOOL_T has_sem_get;
}TUYA_UART_S;
STATIC TUYA_UART_S ty_uart[TY_UART_NUM];

/***********************************************************
*************************function define********************
***********************************************************/
void ty_read_uart_data_to_buffer(int port, void* param);
void set_printf_port(UINT8 port);
#ifdef PLATFORM_BEKEN
void bk_send_byte(UINT8 uport, UINT8 data);
#endif

/***********************************************************
*  Function: ty_uart_init
*  Input: port badu bits parity stop
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET ty_uart_init(IN CONST TY_UART_PORT_E port,IN CONST TY_UART_BAUD_E badu,\
                               IN CONST TY_DATA_BIT_E bits,IN CONST TY_PARITY_E parity,\
                               IN CONST TY_STOPBITS_E stop,IN CONST UINT_T bufsz,IN CONST BOOL_T unblock)
{
    bk_uart_config_t config;
    if((port >= TY_UART_NUM) || (bufsz == 0)) {
        return OPRT_INVALID_PARM;
    }

    if(ty_uart[port].buf == NULL){
        memset(&ty_uart[port], 0, sizeof(TUYA_UART_S));
        ty_uart[port].buf = Malloc(bufsz);
        if(ty_uart[port].buf == NULL){
            return OPRT_MALLOC_FAILED;
        }
        
        ty_uart[port].buf_len = bufsz;
        ty_uart[port].in = 0;
        ty_uart[port].out = 0;
        ty_uart[port].unblock = unblock;
        ty_uart[port].has_sem_get = TRUE;
        PR_DEBUG("uart  unblocked: %d, buf size : %d",unblock,bufsz);

        if(!unblock) {
            OPERATE_RET op_ret = tuya_hal_semaphore_create_init(&(ty_uart[port].uart_sem), 0, 10);
            if(OPRT_OK != op_ret){
                PR_ERR("create uart semaphore failed");
                return op_ret;
            }
        }
    }else {
        return OPRT_COM_ERROR;
    }

    config.baud_rate = badu;
    config.data_width = bits;
    config.parity = parity;    //0:no parity,1:odd,2:even
    config.stop_bits = stop;   //0:1bit,1:2bit
    config.flow_control = 0;   //FLOW_CTRL_DISABLED
    config.flags = 0;

    bk_uart_initialize(port, &config, NULL);
    bk_uart_set_rx_callback(port, ty_read_uart_data_to_buffer, NULL);
    return OPRT_OK;
}

/***********************************************************
*  Function: ty_uart_free
*  Input:free uart
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET ty_uart_free(IN CONST TY_UART_PORT_E port)
{
    if(port >= TY_UART_NUM) {
       return OPRT_INVALID_PARM;
    }

    // uart deinit
    bk_uart_set_rx_callback(port, NULL, NULL);

    if(ty_uart[port].buf != NULL) {
        Free(ty_uart[port].buf);
        ty_uart[port].buf = NULL;
    }
    ty_uart[port].buf_len = 0;

    if(!ty_uart[port].unblock) {
        tuya_hal_semaphore_release(ty_uart[port].uart_sem);
    }

    ty_uart[port].unblock = FALSE;

    return OPRT_OK;
}

STATIC UINT_T __ty_uart_read_data_size(IN CONST TY_UART_PORT_E port)
{
    UINT_T remain_buf_size = 0;

    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    if(ty_uart[port].in >= ty_uart[port].out) {
        remain_buf_size = ty_uart[port].in-ty_uart[port].out;
    }else {
        remain_buf_size = ty_uart[port].in + ty_uart[port].buf_len - ty_uart[port].out;
    }
    GLOBAL_INT_RESTORE();
    
    return remain_buf_size;
}

void ty_read_uart_data_to_buffer(int port, void* param)
{
    int rc = 0;
    
    while((rc = uart_read_byte(port)) != -1)
    {
        if(__ty_uart_read_data_size(port) < (ty_uart[port].buf_len-1)) 
        {
            ty_uart[port].buf[ty_uart[port].in++] = rc;
            if(ty_uart[port].in >= ty_uart[port].buf_len){
                ty_uart[port].in = 0;
            }
        }
//        else
//            bk_printf("L:%d, size:%d", __LINE__, __ty_uart_read_data_size(port));
    }
    
    if(!ty_uart[port].unblock) {
        tuya_hal_semaphore_post(ty_uart[port].uart_sem);
    }
}

/***********************************************************
*  Function: ty_uart_send_data
*  Input: port data len
*  Output: none
*  Return: none
***********************************************************/
VOID ty_uart_send_data(IN CONST TY_UART_PORT_E port,IN CONST BYTE_T *data,IN CONST UINT_T len)
{
    UINT_T i = 0;

    if(port >= TY_UART_NUM) {
        return;
    }

    for(i = 0; i < len; i++) {
       bk_send_byte(port,*(data+i));
    }
}

/***********************************************************
*  Function: ty_uart_read_data
*  Input: len->data buf len
*  Output: buf->read data buf
*  Return: actual read data size
***********************************************************/
UINT_T ty_uart_read_data(IN CONST TY_UART_PORT_E port,OUT BYTE_T *buf,IN CONST UINT_T len)
{
    UINT_T actual_size = 0;
    UINT_T cur_num;
    UINT_T i = 0;

    if(NULL == buf || 0 == len) {
        return 0;
    }

//    if(0 == __ty_uart_read_data_size(port)) {
        if(!ty_uart[port].unblock) {
            OPERATE_RET op_ret = tuya_hal_semaphore_wait(ty_uart[port].uart_sem);
            if(OPRT_OK != op_ret) {
                PR_ERR("WaitSemaphore failed");
                return op_ret;
            }
            
            ty_uart[port].has_sem_get = TRUE;
        }
//    }

    cur_num = __ty_uart_read_data_size(port);
    if(cur_num > ty_uart[port].buf_len - 1) {
        PR_NOTICE("uart fifo is full! buf_zize:%d  len:%d",cur_num,len);
    }
    
    if(len > cur_num) {
        actual_size = cur_num;
    }else {
        actual_size = len;
    }
    
    for(i = 0; i < actual_size; i++) {
        *buf++ = ty_uart[port].buf[ty_uart[port].out++];
        if(ty_uart[port].out >= ty_uart[port].buf_len) {
            ty_uart[port].out = 0;
        }
    }

    return actual_size;
}
    
VOID ty_set_log_port(LOG_PORT_E port)
{
    if(LOG_PORT1 == port)
        set_printf_port(1);
    else
        set_printf_port(2);
}

