/***********************************************************
*  File: wifi_hwl.c
*  Author: nzy
*  Date: 20170914
***********************************************************/
#define _WIFI_HWL_GLOBAL
#include "tuya_hal_wifi.h"
#include "rw_pub.h"
#include "wlan_ui_pub.h"
#include "uart_pub.h"
#include "tuya_os_adapter.h"
#include "../system/tuya_hal_system_internal.h"
#include "../errors_compat.h"
#include "uni_log.h"
#if defined(PLATFORM_BEKEN)
#include "bk7011_cal_pub.h"
#include "param_config.h"

int bk_wlan_dtim_rf_ps_timer_pause(void);
int bk_wlan_dtim_rf_ps_timer_start(void);
int bk_wlan_get_channel(void);
#endif

#include "mem_pub.h"
#include "start_type_pub.h"
#include "str_pub.h"

VOID *Malloc(IN SIZE_T reqSize);
VOID Free(IN VOID *ptr);

/***********************************************************
*************************micro define***********************
***********************************************************/

#if 0
#define LOGD PR_DEBUG
#define LOGN PR_NOTICE
#define LOGE PR_ERR
#else
#define LOGD(...) bk_printf("[WIFI DEBUG]" __VA_ARGS__)
#define LOGN(...) bk_printf("[WIFI NOTICE]" __VA_ARGS__)
#define LOGE(...) bk_printf("[WIFI ERROR]" __VA_ARGS__)
#endif

#define SCAN_MAX_AP 64
typedef struct {
    AP_IF_S *ap_if;
    uint8_t ap_if_nums;
    uint8_t ap_if_count;
    SEM_HANDLE sem_handle;
}SACN_AP_RESULT_S;


#define UNVALID_SIGNAL -127
#define SCAN_WITH_SSID 1
/***********************************************************
*************************variable define********************
***********************************************************/
static WF_WK_MD_E wf_mode = WWM_STATION; //WWM_LOWPOWER
static SNIFFER_CALLBACK snif_cb = NULL;

static SEM_HANDLE scanHandle = NULL;

static bool lp_mode = FALSE;

/***********************************************************
*************************function define********************
***********************************************************/
static void __dhcp_thread(void* pArg);
static void __hwl_promisc_callback(unsigned char *buf, unsigned int len, void* userdata);

static VOID scan_cb(void *ctxt, uint8_t vif_idx)
{
    if(scanHandle)
    {
        tuya_hal_semaphore_post(scanHandle);
    }
}

/***********************************************************
*  Function: hwl_wf_all_ap_scan
*  Input: none
*  Output: ap_ary num
*  Return: int
***********************************************************/
int tuya_hal_wifi_all_ap_scan(AP_IF_S **ap_ary, uint32_t *num)
{
    AP_IF_S *item;
    AP_IF_S *array;
    OPERATE_RET ret;
    INT_T i;
    //INT_T index;
    INT_T scan_cnt;
    struct scanu_rst_upload *scan_rst;
    struct sta_scan_res *scan_rst_ptr;

    if((NULL == ap_ary) || (NULL == num)) {
        return OPRT_INVALID_PARM;
    }
    
    ret = tuya_hal_semaphore_create_init(&scanHandle, 0, 1);
    if ( ret !=  OPRT_OK ) {
        return ret;
    }

    mhdr_scanu_reg_cb(scan_cb, 0);
    bk_wlan_start_scan();
    tuya_hal_semaphore_wait(scanHandle);
    tuya_hal_semaphore_release(scanHandle);

    scan_rst = sr_get_scan_results();
    if( scan_rst == NULL ) {
        return OPRT_COM_ERROR;
    }

    scan_cnt = bk_wlan_get_scan_ap_result_numbers();

    if(scan_cnt > SCAN_MAX_AP) {
        scan_cnt = SCAN_MAX_AP;
    }

    if(0 == scan_cnt) {
        sr_release_scan_results(scan_rst);
        return OPRT_COM_ERROR;
    }
    
    array = tuya_hal_internal_malloc(SIZEOF(AP_IF_S) * scan_cnt);
    if(NULL == array){
        sr_release_scan_results(scan_rst);
        return OPRT_MALLOC_FAILED;
    }
    
    for(i = 0; i < scan_cnt; i++) {
        scan_rst_ptr = scan_rst->res[i];
        item = &array[i];

        item->channel = scan_rst_ptr->channel;
        item->rssi = scan_rst_ptr->level;
        os_memcpy(item->bssid, scan_rst_ptr->bssid, 6);
        os_strcpy((char *)item->ssid, scan_rst_ptr->ssid);
        item->s_len = os_strlen((char *)item->ssid);
    }
    
    *ap_ary = array;
    *num = scan_cnt & 0xff;
    sr_release_scan_results(scan_rst);
    return ret;
}

/***********************************************************
*  Function: hwl_wf_assign_ap_scan
*  Input: ssid
*  Output: ap
*  Return: int
***********************************************************/
int tuya_hal_wifi_assign_ap_scan(const char *ssid, AP_IF_S **ap)
{
    AP_IF_S *item;
    AP_IF_S *array;
    OPERATE_RET ret;
    INT_T i, scan_cnt;
    struct scanu_rst_upload *scan_rst;
    struct sta_scan_res *scan_rst_ptr;

    if((NULL == ssid) || (NULL == ap)) {
        return OPRT_INVALID_PARM;
    }

    ret = tuya_hal_semaphore_create_init(&scanHandle, 0, 1);
    if ( ret !=  OPRT_OK ) {
        return ret;
    }

    mhdr_scanu_reg_cb(scan_cb, 0);
    bk_wlan_start_assign_scan((UINT8 **)&ssid, 1);

    tuya_hal_semaphore_wait(scanHandle);
    tuya_hal_semaphore_release(scanHandle);

    scan_rst = sr_get_scan_results();
    if( scan_rst == NULL ) {
        return OPRT_COM_ERROR;
    }       

    scan_cnt = bk_wlan_get_scan_ap_result_numbers();
    if(scan_cnt >= 1) {
        scan_cnt = 1;
    }
    
    if(0 == scan_cnt) {
        sr_release_scan_results(scan_rst);
        return OPRT_COM_ERROR;
    }
    
    array = tuya_hal_internal_malloc(sizeof(AP_IF_S) * scan_cnt);
    if(NULL == array) {
        sr_release_scan_results(scan_rst);
        return OPRT_MALLOC_FAILED;
    }

    for(i = 0; i < scan_cnt; i++) {
        scan_rst_ptr = scan_rst->res[i];
        item = &array[i];

        item->channel = scan_rst_ptr->channel;
        item->rssi = scan_rst_ptr->level;
        os_memcpy(item->bssid, scan_rst_ptr->bssid, 6);
        os_strcpy((char *)item->ssid, scan_rst_ptr->ssid);
        item->s_len = os_strlen((char *)item->ssid);
    }

    *ap = array;
    sr_release_scan_results(scan_rst);

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_release_ap
*  Input: ap
*  Output: none
*  Return: int
***********************************************************/
int tuya_hal_wifi_release_ap(AP_IF_S *ap)
{
    if(NULL == ap) {
        return OPRT_INVALID_PARM;
    }

    Free(ap);
    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_set_cur_channel
*  Input: chan
*  Output: none
*  Return: int
***********************************************************/
int tuya_hal_wifi_set_cur_channel(const uint8_t chan)
{
    OPERATE_RET ret;
    int status;

    if((chan > 14) || (chan < 1)) {
        return OPRT_INVALID_PARM;
    }

    status = bk_wlan_set_channel(chan);
    if(status) {
        ret = OPRT_INVALID_PARM;
    }else {
        ret = OPRT_OK;
    }

    return ret;
}

/***********************************************************
*  Function: hwl_wf_get_cur_channel
*  Input: none
*  Output: chan
*  Return: int
***********************************************************/
int tuya_hal_wifi_get_cur_channel(uint8_t *chan)
{
    *chan = bk_wlan_get_channel();
    
	return OPRT_OK;
}

static void _wf_sniffer_set_cb(uint8_t *data, int len, hal_wifi_link_info_t *info)
{
    if(NULL != snif_cb)
	    (*snif_cb)(data, len);
}

/***********************************************************
*  Function: hwl_wf_sniffer_set
*  Input: en cb
*  Output: none
*  Return: int
***********************************************************/
int tuya_hal_wifi_sniffer_set(const bool en, const SNIFFER_CALLBACK cb)
{
    if(en) {
        WF_WK_MD_E mode = WWM_LOWPOWER;
        tuya_hal_wifi_get_work_mode(&mode);
        if((mode == WWM_SOFTAP) || (mode == WWM_STATIONAP)) {
            bk_wlan_set_ap_monitor_coexist(1);
        }else {
            bk_wlan_set_ap_monitor_coexist(0);
        }
        
        snif_cb = cb;
        bk_wlan_register_monitor_cb(_wf_sniffer_set_cb);
        bk_wlan_start_monitor();
    }else {
        bk_wlan_register_monitor_cb(NULL);
        bk_wlan_stop_monitor();
    }

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_get_ip
*  Input: wf
*  Output: ip
*  Return: int
***********************************************************/
int tuya_hal_wifi_get_ip(const WF_IF_E wf, NW_IP_S *ip)
{
    OPERATE_RET ret = OPRT_OK;
	IPStatusTypedef wNetConfig;
	WiFi_Interface iface;

	os_memset(&wNetConfig, 0x0, sizeof(IPStatusTypedef));
	
	switch ( wf ) {
		case WF_STATION:
	        iface = STATION;
			wNetConfig.dhcp = DHCP_CLIENT;
			break;
		case WF_AP:
			iface = SOFT_AP;
			wNetConfig.dhcp = DHCP_SERVER;
			break;
		default:
			ret = OPRT_INVALID_PARM;
	}

	if ( ret ==  OPRT_OK ) {
		bk_wlan_get_ip_status(&wNetConfig, iface);
		os_strcpy(ip->ip, wNetConfig.ip);
		os_strcpy(ip->mask, wNetConfig.mask);
		os_strcpy(ip->gw, wNetConfig.gate);
	}

	return ret;    
}

/***********************************************************
*  Function: hwl_wf_set_ip
*  Input: wf
*  Output: ip
*  Return: int
***********************************************************/
int tuya_hal_wifi_set_ip(const WF_IF_E wf, const NW_IP_S *ip)
{
    OPERATE_RET ret = OPRT_OK;
	IPStatusTypedef wNetConfig;
	WiFi_Interface iface;

	os_memset(&wNetConfig, 0x0, sizeof(IPStatusTypedef));
	
	switch ( wf ) {
		case WF_STATION:
	        iface = STATION;
			wNetConfig.dhcp = DHCP_CLIENT;
			break;
		case WF_AP:
			iface = SOFT_AP;
			wNetConfig.dhcp = DHCP_SERVER;
			break;
		default:
			ret = OPRT_INVALID_PARM;
	}

	if ( ret ==  OPRT_OK ) {
		os_strcpy((char *)wNetConfig.ip, ip->ip);
		os_strcpy((char *)wNetConfig.mask, ip->mask);
		os_strcpy((char *)wNetConfig.gate, ip->gw);
		os_strcpy((char *)wNetConfig.dns, ip->ip);
		bk_wlan_set_ip_status(&wNetConfig, iface);
	}

	return ret;
}

#if 0
static unsigned char hwl_wf_base16_decode(char code)
{
    if ('0' <= code && code <= '9') {
        return code - '0';
    } else if ('a' <= code && code <= 'f') {
        return code - 'a' + 10;
    } else if ('A' <= code && code <= 'F') {
        return code - 'A' + 10;
    } else {
        return 0;
    }
}

int tuya_hal_wifi_station_get_ap_mac(NW_MAC_S *mac)
{
	return tuya_hal_wifi_get_mac(WF_AP, mac);
}
#endif
/***********************************************************
*  Function: hwl_wf_get_mac
*  Input: wf
*  Output: mac
*  Return: int
***********************************************************/
int tuya_hal_wifi_get_mac(const WF_IF_E wf, NW_MAC_S *mac)
{
   if(wf == WF_STATION)
        wifi_get_mac_address((char *)mac,2);
   else
        wifi_get_mac_address((char *)mac,1);
    
    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_set_mac
*  Input: wf mac
*  Output: none
*  Return: int
***********************************************************/
int tuya_hal_wifi_set_mac(const WF_IF_E wf, const NW_MAC_S *mac)
{
    wifi_set_mac_address((char *)mac);
    
	return OPRT_OK;
}

static int _wf_wk_mode_exit(WF_WK_MD_E mode)
{
	int ret = OPRT_OK;
	
	switch(mode) {
	    case WWM_LOWPOWER : 
		    break;

	    case WWM_SNIFFER: 
			bk_wlan_stop_monitor();
		    break;

	    case WWM_STATION: 
		    bk_wlan_stop(STATION);
		    break;

	    case WWM_SOFTAP: 
		    bk_wlan_stop(SOFT_AP);
		    break;

	    case WWM_STATIONAP: 
		    bk_wlan_stop(SOFT_AP);
		    bk_wlan_stop(STATION);
		    break;
			
		default:
			break;
	}
		
	return ret;
}
/***********************************************************
*  Function: hwl_wf_wk_mode_set
*  Input: mode
*  Output: none
*  Return: int
***********************************************************/
int tuya_hal_wifi_set_work_mode(const WF_WK_MD_E mode)
{
	OPERATE_RET ret = OPRT_OK;
	WF_WK_MD_E current_mode;

	ret = tuya_hal_wifi_get_work_mode(&current_mode);
	if((OPRT_OK == ret) && (current_mode != mode))
	{
		_wf_wk_mode_exit(current_mode);
	}
	wf_mode = mode;

	return ret;
}

/***********************************************************
*  Function: hwl_close_concurrent_ap
*  Input: none
*  Output: mode
*  Return: OPERATE_RET
***********************************************************/
int tuya_hal_wifi_close_concurrent_ap(void)
{
    //wifi_suspend_softap();  ---注释掉，这个功能还没有调试
    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_wk_mode_get
*  Input: none
*  Output: mode
*  Return: int
***********************************************************/
int tuya_hal_wifi_get_work_mode(WF_WK_MD_E *mode)
{
    *mode = wf_mode;
    
    return OPRT_OK;
}


//#if defined(ENABLE_AP_FAST_CONNECT) && (ENABLE_AP_FAST_CONNECT==1)
/***********************************************************
*  Function: hwl_wf_fast_station_connect
*  Input: none
*  Output: fast_ap_info
*  Return: int
***********************************************************/
int hwl_wf_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_S *fast_ap_info)
{

    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_fast_station_connect
*  Input: ssid passwd
*  Output: mode
*  Return: int
***********************************************************/
int hwl_wf_fast_station_connect(FAST_WF_CONNECTED_AP_INFO_S *fast_ap_info)
{
 
    return OPRT_OK;

}
//#endif

/***********************************************************
*  Function: hwl_wf_station_connect
*  Input: ssid passwd
*  Output: mode
*  Return: int
***********************************************************/
// only support wap/wap2 
int tuya_hal_wifi_station_connect(const char *ssid, const char *passwd)
{
	network_InitTypeDef_st wNetConfig;
	os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

	os_strcpy((char *)wNetConfig.wifi_ssid, ssid);
	os_strcpy((char *)wNetConfig.wifi_key, passwd);

	wNetConfig.wifi_mode = STATION;
	wNetConfig.dhcp_mode = DHCP_CLIENT;

	bk_wlan_start(&wNetConfig);
	
	return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_station_disconnect
*  Input: none
*  Output: none
*  Return: int
***********************************************************/
int tuya_hal_wifi_station_disconnect(void)
{
    //bk_wlan_stop(STATION);
	return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_station_get_conn_ap_rssi
*  Input: none
*  Output: rssi
*  Return: int
***********************************************************/
int tuya_hal_wifi_station_get_conn_ap_rssi(int8_t *rssi)
{
    OPERATE_RET ret = OPRT_OK;
	LinkStatusTypeDef sta;

	ret = bk_wlan_get_link_status(&sta);
	if ( ret == OPRT_OK ) {
		*rssi = sta.wifi_strength;
	}

	return ret;
}

/***********************************************************
*  Function: hwl_wf_station_stat_get
*  Input: none
*  Output: stat
*  Return: int
***********************************************************/
int tuya_hal_wifi_station_get_status(WF_STATION_STAT_E *stat)
{
    static UCHAR_T flag = FALSE;
    rw_evt_type type = mhdr_get_station_status();
    {
        switch(type) {
            case RW_EVT_STA_IDLE:                   *stat = WSS_IDLE;               break;
            case RW_EVT_STA_SCANNING:               *stat = WSS_CONNECTING;         break;
            case RW_EVT_STA_SCAN_OVER:              *stat = WSS_CONNECTING;         break;
            case RW_EVT_STA_CONNECTING:             *stat = WSS_CONNECTING;         break;
            case RW_EVT_STA_PASSWORD_WRONG:         *stat = WSS_PASSWD_WRONG;       break;
            case RW_EVT_STA_NO_AP_FOUND:            *stat = WSS_NO_AP_FOUND;        break;
            case RW_EVT_STA_ASSOC_FULL:             *stat = WSS_CONN_FAIL;          break;
            case RW_EVT_STA_BEACON_LOSE:            *stat = WSS_CONN_FAIL;          break;
            case RW_EVT_STA_DISCONNECTED:           *stat = WSS_CONN_FAIL;          break;
            case RW_EVT_STA_CONNECT_FAILED:         *stat = WSS_CONN_FAIL;          break;
            case RW_EVT_STA_CONNECTED:              *stat = WSS_CONN_SUCCESS;       break;
            case RW_EVT_STA_GOT_IP:                 *stat = WSS_GOT_IP;             break;
            case RW_EVT_AP_CONNECTED:               *stat = WSS_CONN_SUCCESS;       break;
            case RW_EVT_AP_DISCONNECTED:            *stat = WSS_CONN_FAIL;          break;
            case RW_EVT_AP_CONNECT_FAILED:          *stat = WSS_CONN_FAIL;          break;
            case RW_EVT_MAX: break;
        }
    }
    
    if((!flag) && (*stat == WSS_GOT_IP)) {
        bk_wlan_dtim_rf_ps_mode_enable();
        bk_wlan_dtim_rf_ps_timer_start();
        if(tuya_hal_get_lp_mode()) {
            bk_wlan_mcu_ps_mode_enable();
            lp_mode = TRUE;
        } else
            lp_mode = FALSE;
        
        flag = TRUE;
    }else if(*stat != WSS_GOT_IP) {
        flag = FALSE;
    }
    
    return OPRT_OK;
}

/***********************************************************
*  Function: hwl_wf_ap_start
*  Input: cfg
*  Output: none
*  Return: int
***********************************************************/
int tuya_hal_wifi_ap_start(const WF_AP_CFG_IF_S *cfg)
{
    OPERATE_RET ret = OPRT_OK;
	network_InitTypeDef_ap_st wNetConfig;
	
    os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_ap_st));

	switch ( cfg->md ) {
	    case WAAM_OPEN :
			wNetConfig.security = SECURITY_TYPE_NONE;
	        break;
	    case WAAM_WEP :
			wNetConfig.security = SECURITY_TYPE_WEP;
	        break;
	    case WAAM_WPA_PSK :
			wNetConfig.security = SECURITY_TYPE_WPA2_TKIP;
	        break;
	    case WAAM_WPA2_PSK :
			wNetConfig.security = SECURITY_TYPE_WPA2_AES;
	        break;
	    case WAAM_WPA_WPA2_PSK:
			wNetConfig.security = SECURITY_TYPE_WPA2_MIXED;
	        break;
	    default:
	        ret = OPRT_INVALID_PARM;
	}

	if ( ret == OPRT_OK ) {
		wNetConfig.channel = cfg->chan;
		wNetConfig.dhcp_mode = DHCP_SERVER;
		os_memcpy((char *)wNetConfig.wifi_ssid, cfg->ssid, cfg->s_len);
		os_memcpy((char *)wNetConfig.wifi_key, cfg->passwd, cfg->p_len);
		
		os_strcpy((char *)wNetConfig.local_ip_addr, "192.168.175.1");
		os_strcpy((char *)wNetConfig.net_mask, "255.255.255.0");
		os_strcpy((char *)wNetConfig.gateway_ip_addr, "192.168.175.1");
		os_strcpy((char *)wNetConfig.dns_server_ip_addr, "192.168.175.1");
		
		bk_printf("ssid:%s	key:%s  channnel: %d\r\n", wNetConfig.wifi_ssid, wNetConfig.wifi_key,wNetConfig.channel);
		bk_wlan_start_ap_adv(&wNetConfig);
	}

	return ret;
}

/***********************************************************
*  Function: hwl_wf_ap_stop
*  Input: none
*  Output: none
*  Return: int
***********************************************************/
int tuya_hal_wifi_ap_stop(void)
{
 	bk_wlan_stop(SOFT_AP);

    return OPRT_OK;
}


wifi_country_t country_code[] = 
{
	{.cc= "CN", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
	{.cc= "US", .schan=1, .nchan=11, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
	{.cc= "EU", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
	{.cc= "JP", .schan=1, .nchan=14, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL}
	//{.cc= "AU", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL}	
};

int tuya_hal_wifi_set_country_code(const char *p_country_code)
{
	OPERATE_RET ret = OPRT_OK;
	wifi_country_t country;
	int i, country_num;
	
	os_memset(&country, 0, sizeof(wifi_country_t));
	country_num = sizeof(country_code)/sizeof(wifi_country_t);
	for(i = 0; i < country_num; i++) {
		if(os_strcmp(country_code[i].cc, p_country_code) == 0) {
			country = country_code[i];
			break;
		}
	}
	if(i < country_num) {
		bk_wlan_set_country(&country);
		if(os_strcmp(p_country_code,"EU") == 0) {
            //enable
            bk_wlan_phy_open_cca();
		}else {
            //disable
            bk_wlan_phy_close_cca();
		}
	}else {
		ret = OPRT_INVALID_PARM;
	}
    return ret;
}

void hwl_wf_get_country_code(unsigned char *country_code)
{
	wifi_country_t country;
	
    bk_wlan_get_country(&country);
	os_strcpy((char *)country_code, country.cc);
}

static uint32_t lp_rcnt = 0;

int tuya_hal_wifi_lowpower_disable(void)
{
    if(!tuya_hal_get_lp_mode()) {
        //PR_ERR("it's in normal mode");
        return OPRT_COM_ERROR;
    }
    
    lp_mode = FALSE;
    bk_wlan_mcu_ps_mode_disable();
    lp_rcnt ++;

    return OPRT_OK;
}

int tuya_hal_wifi_lowpower_enable(void)
{
    if(!tuya_hal_get_lp_mode()) {
        //PR_ERR("it's in normal mode");
        return OPRT_COM_ERROR;
    }

    if(lp_rcnt > 0) {
        lp_rcnt--;
    }
    
    if(!lp_rcnt) {
        bk_wlan_mcu_ps_mode_enable();
        lp_mode = TRUE;
    }

    return OPRT_OK;
}

int tuya_hal_wifi_send_mgnt(const uint8_t *buf, const uint32_t len)
{
    int ret = bk_wlan_send_80211_raw_frame((uint8_t *)buf, len);
    if(ret < 0) {
        return OPRT_COM_ERROR;
    }
    
    return OPRT_OK; 
}

#include "uni_thread.h"
#include "uni_msg_queue.h"
#include "tuya_hal_semaphore.h"
#define WIFI_MGNT_FRAME_RX_MSG              (1 << 0)
#define WIFI_MGNT_FRAME_TX_MSG              (1 << 1)
static MSG_QUE_HANDLE  frameMsgQueue  = NULL;
static SEM_HANDLE      frameSendSem   = NULL;
static WIFI_REV_MGNT_CB mgnt_recv_cb = NULL;

static VOID wifi_mgnt_frame_rx_handler(const uint8_t *frame, int len)
{
    if (mgnt_recv_cb) {
        mgnt_recv_cb((uint8_t *)frame, len);
    }
}

void wifi_mgnt_frame_tx_notify(void *param)
{
    tuya_hal_semaphore_post(frameSendSem);
}

static OPERATE_RET wifi_mgnt_frame_tx_handler(uint8_t *frame, int len)
{
    OPERATE_RET op_ret = OPRT_OK;

    // PR_DEBUG_RAW("\r\n");
    // UINT_T i = 0;
    // for(i = 0; i < len; i++) {
    //     PR_DEBUG_RAW("%02x ",(UCHAR_T)frame[i]);
    // }
    // PR_DEBUG_RAW("\r\n");

    op_ret = bk_wlan_send_raw_frame_with_cb(frame, len, wifi_mgnt_frame_tx_notify, NULL);

    tuya_hal_semaphore_wait(frameSendSem);

    return op_ret;
}

static OPERATE_RET wifi_mgnt_frame_malloc_post(UCHAR_T *frame, UINT_T len)
{
    OPERATE_RET op_ret = OPRT_OK;
    
    UCHAR_T *frame_msg = NULL;

    frame_msg = (UCHAR_T *)Malloc(len);
    if (NULL == frame_msg) {
        PR_ERR("frame msg malloc failed");
        return OPRT_MALLOC_FAILED;
    }
    memset(frame_msg, 0, len);
    
    memcpy(frame_msg, frame, len);
    op_ret = PostMessage(frameMsgQueue, WIFI_MGNT_FRAME_RX_MSG, frame_msg, len);
    if(OPRT_OK != op_ret) {
        Free(frame_msg); frame_msg = NULL;
    }
    return op_ret;
}

static VOID ty_wf_mgnt_frame_thread(void *parameter)
{
	P_MSG_LIST      frame;
    OPERATE_RET     op_ret = OPRT_OK;

    while (1) {
		op_ret = WaitMessage(frameMsgQueue, &frame);
        if (OPRT_OK != op_ret) {
            continue;
        }

        switch (frame->msg.msgID) {

        case WIFI_MGNT_FRAME_RX_MSG:
            wifi_mgnt_frame_rx_handler((UCHAR_T *)frame->msg.pMsgData, frame->msg.msgDataLen);
            break;

        case WIFI_MGNT_FRAME_TX_MSG:
            wifi_mgnt_frame_tx_handler((UCHAR_T *)frame->msg.pMsgData, frame->msg.msgDataLen);
            // PR_NOTICE("WIFI_MGNT_FRAME_TX_MSG");
            break;
        }
        if (frame->msg.pMsgData) {
            Free(frame->msg.pMsgData);
        }
        DelAndFreeMsgNodeFromQueue(frameMsgQueue, frame);
    }
}

static OPERATE_RET ty_wf_mgnt_frame_thread_init(VOID)
{
    OPERATE_RET     op_ret = OPRT_OK;

    op_ret = tuya_hal_semaphore_create_init(&frameSendSem, 0, 1);
    if (OPRT_OK != op_ret) {
        PR_ERR("create sem failed");
        return op_ret;
    }

    op_ret = CreateMsgQueAndInit(&frameMsgQueue);
    if (OPRT_OK != op_ret) {
        tuya_hal_semaphore_release(frameSendSem); frameSendSem = NULL;
        return op_ret;
    }

    THRD_HANDLE     thrd;    
    THRD_PARAM_S thrd_param = {2*1024 + 256,TRD_PRIO_1,"wifi_frame_task"};

    op_ret = CreateAndStart(&thrd, 
                            NULL,
                            NULL,
                            ty_wf_mgnt_frame_thread,     
                            NULL, 
                            &thrd_param);
    if (OPRT_OK != op_ret) {
        tuya_hal_semaphore_release(frameSendSem); frameSendSem = NULL;
        ReleaseMsgQue(frameMsgQueue); frameMsgQueue = NULL;
        return op_ret;
    }   
    
    return OPRT_OK;
}

void ty_wifi_powersave_enable(void)
{
    bk_wlan_stop_ez_of_sta();
    rtos_delay_milliseconds(50);
    bk_wlan_dtim_rf_ps_mode_enable();
    bk_wlan_dtim_rf_ps_timer_start();
}

void ty_wifi_powersave_disable(void)
{
    bk_wlan_dtim_rf_ps_mode_disable();
    bk_wlan_dtim_rf_ps_timer_pause();
    rtos_delay_milliseconds(50);
    bk_wlan_start_ez_of_sta();
}


static VOID wifi_mgnt_frame_rx_notify(const uint8_t *frame, int len, void *param)
{
    wifi_mgnt_frame_malloc_post((UCHAR_T *)frame, len);
}

int tuya_hal_wifi_register_recv_mgnt_callback(bool enable, WIFI_REV_MGNT_CB recv_cb)
{
    if(enable) {
        WF_WK_MD_E mode;
        int ret = tuya_hal_wifi_get_work_mode(&mode);
        if(OPRT_OK != ret) {
            return OPRT_COM_ERROR;
        }

        if((mode == WWM_LOWPOWER) || (mode == WWM_SNIFFER)) {
            return OPRT_COM_ERROR;
        }
		bk_wlan_reg_rx_mgmt_cb((mgmt_rx_cb_t)wifi_mgnt_frame_rx_notify, 0);
        
        ty_wifi_powersave_disable();
        ty_wf_mgnt_frame_thread_init();
        mgnt_recv_cb = recv_cb;

    }else {
		bk_wlan_reg_rx_mgmt_cb(NULL, 0);
        mgnt_recv_cb = NULL;
    }
    return OPRT_OK;
}

int tuya_hal_wifi_set_socket_broadcast_all(const int socket_fd, const bool enable)
{
    return OPRT_OK;
}

bool tuya_hal_wifi_rf_calibrated(void)
{
    int stat = manual_cal_rfcali_status();

    if(stat)
        return true;

    return false;
}
