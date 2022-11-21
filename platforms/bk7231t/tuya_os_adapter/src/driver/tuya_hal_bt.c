#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "uni_md5.h"
#include "aes_inf.h"
#include "rwip_config.h"
#include "ble_api.h"
#include "ble_pub.h"
#include "uart_pub.h"
#include "app_task.h"
#include "mcu_ps_pub.h"
#include "application.h"
#include "tuya_ble_type.h"
#include "tuya_hal_bt.h"
#include "tuya_hal_wifi.h"
#include "uni_log.h"
#include "app_sdp.h"

#define BLE_OPEN         1

void appm_update_adv_data( uint8_t* adv_buff, uint8_t adv_len, uint8_t* scan_buff, uint8_t scan_len);
OPERATE_RET tuya_bt_start_adv();

uint16_t ntf_enable;
#if 1//qdefined(TY_BT_MOD) && TY_BT_MOD == 1

bk_attm_desc_t btl_att_db[6] =
{
	//  Service Declaration
	[0]        =   {0x1910,  BK_PERM_SET(RD, ENABLE), 0, 0},
	
	//  Level Characteristic Declaration
	[1]        =   {0x2803,   BK_PERM_SET(RD, ENABLE), 0, 0},
	//  Level Characteristic Value
	//[2]        =   {0x2b11,   BK_PERM_SET(WRITE_REQ, ENABLE), BK_PERM_SET(RI, ENABLE) , 20},
	[2]        =   {0x2b11,   BK_PERM_SET(WRITE_REQ, ENABLE)|BK_PERM_SET(WRITE_COMMAND, ENABLE), BK_PERM_SET(RI, ENABLE) , 20},
	[3]        =   {0x2803,   BK_PERM_SET(RD, ENABLE), 0, 0},
	//  Level Characteristic Value
	[4]        =   {0x2b10,   BK_PERM_SET(NTF, ENABLE) , BK_PERM_SET(RI, ENABLE) , 20},

	//  Level Characteristic - Client Characteristic Configuration Descriptor

	[5]        =   {0x2902,  BK_PERM_SET(RD, ENABLE)|BK_PERM_SET(WRITE_REQ, ENABLE), 0, 0},
};

TY_BT_MSG_CB ty_bt_msg_cb;
uint8_t ble_stack_init_ok = 0;

void ble_write_callback(write_req_t *write_req)
{
    tuya_ble_data_buf_t data;

    if(write_req->att_idx == 5)
    {
        ntf_enable = (write_req->value[0]) | (write_req->value[1] << 8);
    }
    else
    {
        data.data = write_req->value;
        data.len = write_req->len;

        os_printf("%s len:%d\r\n", __FUNCTION__, data.len);
        if(ty_bt_msg_cb!=NULL)
            ty_bt_msg_cb(0, TY_BT_EVENT_RX_DATA, &data);
    }
}

uint8_t ble_read_callback(read_req_t *read_req)
{
    if(read_req->att_idx == 5)
    {
        *(read_req->value) = (uint8_t)ntf_enable;
    }
    return 2;
}

unsigned char ble_att_flag = 0;
bool if_start_adv_after_disconnect = true;
void ble_event_callback(ble_event_t event, void *param)
{
    switch(event)
    {
        case BLE_STACK_OK:
        {
            bk_printf("STACK INIT OK\r\n");
            struct bk_ble_db_cfg ble_db_cfg;
            ble_db_cfg.att_db = btl_att_db;
            ble_db_cfg.att_db_nb = 6;
            ble_db_cfg.prf_task_id = 0;
            ble_db_cfg.start_hdl = 0;
            ble_db_cfg.svc_perm = 0;
            ble_db_cfg.uuid = 0x1910;

            bk_ble_create_db(&ble_db_cfg);
        }
        break;
        case BLE_STACK_FAIL:
            bk_printf("STACK INIT FAIL\r\n");
        break;

        case BLE_CONNECT:
            bk_printf("BLE CONNECT\r\n");

			if(((struct gapc_connection_req_ind *)param)->sup_to < 200)
			{
	            struct gapc_conn_param conn_param;
				conn_param.intv_max = ((struct gapc_connection_req_ind *)param)->con_interval;
				conn_param.intv_min = ((struct gapc_connection_req_ind *)param)->con_interval;
				conn_param.latency = ((struct gapc_connection_req_ind *)param)->con_latency;
				conn_param.time_out = 600;
	        	appm_update_param(&conn_param);
			}
		
            mcu_prevent_set(MCU_PS_BLE_FROBID);
            if(ty_bt_msg_cb!=NULL)
                ty_bt_msg_cb(0, TY_BT_EVENT_CONNECTED ,NULL);
        break;

        case BLE_DISCONNECT:
        {
            bk_printf("BLE DISCONNECT\r\n");
			ble_att_flag = 0;
            mcu_prevent_clear(MCU_PS_BLE_FROBID);
            if(ty_bt_msg_cb!=NULL)
                ty_bt_msg_cb(0, TY_BT_EVENT_DISCONNECTED ,NULL);
            
            //该模式适用于BK7231T下需要手工disconnect bt
            if(if_start_adv_after_disconnect)
                tuya_bt_start_adv();
            else        //下次自动开启广播
                if_start_adv_after_disconnect = TRUE;
        }
        break;

        case BLE_MTU_CHANGE:
            bk_printf("BLE_MTU_CHANGE:%d\r\n", *(uint16_t *)param);
        break;
        case BLE_CFG_NOTIFY:
            bk_printf("BLE_CFG_NOTIFY:%d\r\n", *(uint16_t *)param);
        break;
        case BLE_CFG_INDICATE:
            bk_printf("BLE_CFG_INDICATE:%d\r\n", *(uint16_t *)param);
        break;
        case BLE_TX_DONE:
            os_printf("BLE_TX_DONE\r\n");
			ble_att_flag = 0;
        break;
        case BLE_GEN_DH_KEY:
        {
            bk_printf("BLE_GEN_DH_KEY\r\n");
            bk_printf("key_len:%d\r\n", ((struct ble_gen_dh_key_ind *)param)->len);
            for(int i = 0; i < ((struct ble_gen_dh_key_ind *)param)->len; i++)
            {
                bk_printf("%02x ", ((struct ble_gen_dh_key_ind *)param)->result[i]);
            }
            bk_printf("\r\n");
        }    
        break;
        case BLE_GET_KEY:
        {
            bk_printf("BLE_GET_KEY\r\n");
            bk_printf("pri_len:%d\r\n", ((struct ble_get_key_ind *)param)->pri_len);
            for(int i = 0; i < ((struct ble_get_key_ind *)param)->pri_len; i++)
            {
                bk_printf("%02x ", ((struct ble_get_key_ind *)param)->pri_key[i]);
            }
            bk_printf("\r\n");
        }    
        break;
        case BLE_CREATE_DB_OK:
        {
            bk_printf("CREATE DB SUCCESS\r\n");
            ble_stack_init_ok = 1;
            if(ty_bt_msg_cb!=NULL)
                ty_bt_msg_cb(0, TY_BT_EVENT_ADV_READY ,NULL);
            
            bk_printf("!!!!!!!!!!tuya_before_netcfg_cb\r\n");
            appm_start_advertising();
        }
        break;
        default:
            bk_printf("UNKNOW EVENT\r\n");
        break;
    }
}

void ble_recv_adv_callback(uint8_t *buf, uint8_t len)
{
#if (BLE_APP_CLIENT)
    uint8_t find = 0;

    find = appm_adv_data_decode(len, buf, NULL, 0);

    if(find)
    {
        extern int *scan_check_result;
        if(scan_check_result)
        {
            *scan_check_result = 2;
            bk_printf("scan_check_result\r\n");
        }
    }
#endif
}


OPERATE_RET tuya_hal_bt_port_init(ty_bt_param_t *p)
{

    bk_printf("!!!!!!!!!!tuya_bt_port_init\r\n");

    if((p!=NULL)&&(p->cb!=NULL))
        ty_bt_msg_cb = p->cb;
    else
        bk_printf("!!!!!!!!!!tuya_bt_port_init, p is null\r\n");

    ble_activate(NULL);
    ble_set_write_cb(ble_write_callback);
    ble_set_read_cb(ble_read_callback);
    ble_set_event_cb(ble_event_callback);
    ble_set_recv_adv_cb(ble_recv_adv_callback);

    return OPRT_OK;
}

OPERATE_RET tuya_bt_port_deinit(void)
{
	bk_printf("!!!!!!!!!!tuya_bt_port_deinit\r\n");    
	if(appm_stop_advertising() == ERR_SUCCESS)    
	{        
		rtos_delay_milliseconds(100);    
	}

	return OPRT_OK;
}

OPERATE_RET tuya_hal_bt_gap_disconnect(void)
{
    bk_printf("!!!!!!!!!!tuya_bt_gap_disconnect\r\n");
    if_start_adv_after_disconnect = TRUE;
    appm_disconnect(0x13);

    return OPRT_OK;
}


OPERATE_RET tuya_hal_bt_send(BYTE_T *data, UINT8_T len)
{
    os_printf("!!!!!!!!!!tuya_bt_send\r\n");
	while(ble_att_flag == 1){
		rtos_delay_milliseconds(1);
	}
	ble_att_flag = 1;

    bk_ble_send_ntf_value(len, data, 0, 4);

    return OPRT_OK;
}

OPERATE_RET tuya_hal_bt_reset_adv(tuya_ble_data_buf_t *adv, tuya_ble_data_buf_t *scan_resp)
{
    bk_printf("!!!!!!!!!!tuya_bt_reset_adv\r\n");
    /*if (kernel_state_get(TASK_APP) == APPM_ADVERTISING)
    {
        appm_stop_advertising();
        rtos_delay_milliseconds(100);
    }*/

    memset(&adv_info, 0x00, sizeof(adv_info));

    adv_info.channel_map = 7;
    adv_info.interval_min = 160;
    adv_info.interval_max = 160;

    memcpy(adv_info.advData, adv->data, adv->len);
    adv_info.advDataLen = adv->len;

    memcpy(adv_info.respData, scan_resp->data, scan_resp->len);
    adv_info.respDataLen = scan_resp->len;

    appm_update_adv_data(adv->data, adv->len, scan_resp->data, scan_resp->len);
    //appm_start_advertising();

    return OPRT_OK;
}

OPERATE_RET tuya_bt_get_rssi(SCHAR_T *rssi)
{
    bk_printf("!!!!!!!!!!tuya_bt_get_rssi\r\n");

    return OPRT_OK;
}

OPERATE_RET tuya_bt_start_adv()
{
    bk_printf("!!!!!!!!!!tuya_bt_start_adv\r\n");
    if(appm_start_advertising()==OPRT_OK)
        return OPRT_OK;

    return OPRT_COM_ERROR;
}

OPERATE_RET tuya_bt_stop_adv()
{
    bk_printf("!!!!!!!!!!tuya_bt_stop_adv\r\n");
    if(appm_stop_advertising()==OPRT_OK)
        return OPRT_OK;

    return OPRT_COM_ERROR;
}

OPERATE_RET tuya_hal_bt_assign_scan(IN OUT ty_bt_scan_info_t *info)
{
    return OPRT_OK;
}

#endif











