#include "include.h"
#include "common.h"
#include "rwnx_config.h"
#include "app.h"

#if (NX_POWERSAVE)
#include "ps.h"
#endif //(NX_POWERSAVE)

#include "sa_ap.h"
#include "sa_station.h"
#include "main_none.h"
#include "sm.h"

#include "sys_rtos.h"
#include "rtos_pub.h"
#include "rtos_error.h"

#include "wlan_ui_pub.h"
#include "uart_pub.h"
#include "mem_pub.h"
#include "str_pub.h"
#include "ieee802_11_demo.h"
#include "rw_pub.h"
#include "ethernet_intf.h"
#include "ieee802_11_defs.h"
#include "power_save_pub.h"

#include "port/net.h"

#if CFG_ROLE_LAUNCH
#include "role_launch.h"
#endif

static void scan_cb(void *ctxt, uint8_t param)
{
    struct scanu_rst_upload *scan_rst;
    ScanResult apList;
    int i;

    apList.ApList = NULL;
    scan_rst = sr_get_scan_results();
    if( scan_rst == NULL )
    {
        apList.ApNum = 0;
        return;
    }
    else
    {
        apList.ApNum = scan_rst->scanu_num;
    }
    if( apList.ApNum > 0 )
    {
        apList.ApList = (void *)os_malloc(sizeof(*apList.ApList) * apList.ApNum);
        for( i = 0; i < scan_rst->scanu_num; i++ )
        {
            os_memcpy(apList.ApList[i].ssid, scan_rst->res[i]->ssid, 32);
            apList.ApList[i].ApPower = scan_rst->res[i]->level;
        }
    }
    if( apList.ApList == NULL )
    {
        apList.ApNum = 0;
    }
	
	bk_printf("Got ap count: %d\r\n", apList.ApNum);
	for( i = 0; i < apList.ApNum; i++ ) 
	{
		if(os_strlen(apList.ApList[i].ssid) >= SSID_MAX_LEN)
		{
			char temp_ssid[33];
			os_memset(temp_ssid, 0, 33);
			os_memcpy(temp_ssid, apList.ApList[i].ssid, 32);
			bk_printf("    %s, RSSI=%d\r\n", temp_ssid, apList.ApList[i].ApPower);
		}
		else
		{
			bk_printf("    %s, RSSI=%d\r\n", apList.ApList[i].ssid, apList.ApList[i].ApPower);
		}
	}
	bk_printf("Get ap end.......\r\n\r\n");
	
    if( apList.ApList != NULL )
    {
        os_free(apList.ApList);
        apList.ApList = NULL;
    }

	
#if CFG_ROLE_LAUNCH
	rl_pre_sta_set_status(RL_STATUS_STA_LAUNCHED);
#endif

    sr_release_scan_results(scan_rst);
}

void demo_scan_app_init(void)
{
	mhdr_scanu_reg_cb(scan_cb, 0);
	bk_wlan_start_scan();
}

void demo_scan_adv_app_init(uint8_t *oob_ssid)
{
    uint8_t **ssid_array;

    ssid_array = &oob_ssid;
	mhdr_scanu_reg_cb(scan_cb, 0);
    
	bk_printf("scan for ssid:%s\r\n", oob_ssid);
	bk_wlan_start_assign_scan(ssid_array, 1);
}

void demo_softap_app_init(char *ap_ssid, char *ap_key)
{
    network_InitTypeDef_st wNetConfig;
    int len;
    
    os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

    len = os_strlen(ap_ssid);
    if(SSID_MAX_LEN < len)
    {
        bk_printf("ssid name more than 32 Bytes\r\n");
        return;
    }

    os_strcpy((char *)wNetConfig.wifi_ssid, ap_ssid);
    os_strcpy((char *)wNetConfig.wifi_key, ap_key);
    
    wNetConfig.wifi_mode = SOFT_AP;
    wNetConfig.dhcp_mode = DHCP_SERVER;
    wNetConfig.wifi_retry_interval = 100;
    os_strcpy((char *)wNetConfig.local_ip_addr, WLAN_DEFAULT_IP);
    os_strcpy((char *)wNetConfig.net_mask, WLAN_DEFAULT_MASK);
    os_strcpy((char *)wNetConfig.gateway_ip_addr, WLAN_DEFAULT_GW);
    os_strcpy((char *)wNetConfig.dns_server_ip_addr, WLAN_DEFAULT_IP);
    
    bk_printf("ssid:%s  key:%s\r\n", wNetConfig.wifi_ssid, wNetConfig.wifi_key);
	bk_wlan_start(&wNetConfig);
}

void demo_sta_app_init(char *oob_ssid,char *connect_key)
{
	network_InitTypeDef_st wNetConfig;
    int len;
	os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

    len = os_strlen(oob_ssid);
    if(SSID_MAX_LEN < len)
    {
        bk_printf("ssid name more than 32 Bytes\r\n");
        return;
    }
    
	os_strcpy((char *)wNetConfig.wifi_ssid, oob_ssid);
	os_strcpy((char *)wNetConfig.wifi_key, connect_key);
    
	wNetConfig.wifi_mode = STATION;
	wNetConfig.dhcp_mode = DHCP_CLIENT;
	wNetConfig.wifi_retry_interval = 100;
    
	bk_printf("ssid:%s key:%s\r\n", wNetConfig.wifi_ssid, wNetConfig.wifi_key);
	bk_wlan_start(&wNetConfig);
}

void demo_sta_adv_app_init(char *oob_ssid,char *connect_key)
{
	network_InitTypeDef_adv_st	wNetConfigAdv;

	os_memset( &wNetConfigAdv, 0x0, sizeof(network_InitTypeDef_adv_st) );
	
	os_strcpy((char*)wNetConfigAdv.ap_info.ssid, oob_ssid);
	hwaddr_aton("48:ee:0c:48:93:12", (u8*)wNetConfigAdv.ap_info.bssid);
	wNetConfigAdv.ap_info.security = SECURITY_TYPE_WPA2_MIXED;
	wNetConfigAdv.ap_info.channel = 11;
	
	os_strcpy((char*)wNetConfigAdv.key, connect_key);
	wNetConfigAdv.key_len = os_strlen(connect_key);
	wNetConfigAdv.dhcp_mode = DHCP_CLIENT;
	wNetConfigAdv.wifi_retry_interval = 100;

	bk_wlan_start_sta_adv(&wNetConfigAdv);
}

void demo_wlan_app_init(VIF_ADDCFG_PTR cfg)
{
    network_InitTypeDef_st network_cfg;

    if(cfg->wlan_role == STATION) {
        if(cfg->adv == 1) {
            network_InitTypeDef_adv_st	networkadv_cfg;

        	os_memset( &networkadv_cfg, 0x0, sizeof(network_InitTypeDef_adv_st) );
        	
        	os_strcpy((char*)networkadv_cfg.ap_info.ssid, cfg->ssid);
        	hwaddr_aton("48:ee:0c:48:93:12", (u8*)networkadv_cfg.ap_info.bssid);
        	networkadv_cfg.ap_info.security = SECURITY_TYPE_WPA2_MIXED;
        	networkadv_cfg.ap_info.channel = 11;
        	
        	os_strcpy((char*)networkadv_cfg.key, cfg->key);
        	networkadv_cfg.key_len = os_strlen(cfg->key);
        	networkadv_cfg.dhcp_mode = DHCP_CLIENT;
        	networkadv_cfg.wifi_retry_interval = 100;

        	bk_wlan_start_sta_adv(&networkadv_cfg);
            return;
        } else {
        	os_memset(&network_cfg, 0x0, sizeof(network_InitTypeDef_st));

        	os_strcpy((char *)network_cfg.wifi_ssid, cfg->ssid);
        	os_strcpy((char *)network_cfg.wifi_key, cfg->key);

        	network_cfg.wifi_mode = STATION;
        	network_cfg.dhcp_mode = DHCP_CLIENT;
        	network_cfg.wifi_retry_interval = 100;

        	bk_printf("ssid:%s key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);
        }
    } else if(cfg->wlan_role == SOFT_AP) {
    
        os_memset(&network_cfg, 0x0, sizeof(network_InitTypeDef_st));
        os_strcpy((char *)network_cfg.wifi_ssid, cfg->ssid);
        os_strcpy((char *)network_cfg.wifi_key, cfg->key);

        network_cfg.wifi_mode = SOFT_AP;
        network_cfg.dhcp_mode = DHCP_SERVER;
        network_cfg.wifi_retry_interval = 100;
        os_strcpy((char *)network_cfg.local_ip_addr, WLAN_DEFAULT_IP);
        os_strcpy((char *)network_cfg.net_mask, WLAN_DEFAULT_MASK);
        os_strcpy((char *)network_cfg.gateway_ip_addr, WLAN_DEFAULT_GW);
        os_strcpy((char *)network_cfg.dns_server_ip_addr, WLAN_DEFAULT_IP);

        bk_printf("ssid:%s  key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);
    }

    bk_wlan_start(&network_cfg);

}

void demo_state_app_init(void)
{
	LinkStatusTypeDef linkStatus;
    network_InitTypeDef_ap_st ap_info;
    char ssid[33] = {0};
    #if CFG_IEEE80211N
        bk_printf("sta: %d, softap: %d, b/g/n\r\n",sta_ip_is_start(),uap_ip_is_start());
    #else
        bk_printf("sta: %d, softap: %d, b/g\r\n",sta_ip_is_start(),uap_ip_is_start());
    #endif
    
    if( sta_ip_is_start() )
    {
    	os_memset(&linkStatus, 0x0, sizeof(LinkStatusTypeDef));
    	bk_wlan_get_link_status(&linkStatus);
        os_memcpy(ssid, linkStatus.ssid, 32);

    	bk_printf("sta:rssi=%d,ssid=%s,bssid=" MACSTR ",channel=%d,cipher_type:",
    		linkStatus.wifi_strength, ssid, MAC2STR(linkStatus.bssid), linkStatus.channel);
        switch(bk_sta_cipher_type())
        {
           case SECURITY_TYPE_NONE:
                bk_printf("OPEN\r\n");
                break;
            case SECURITY_TYPE_WEP :
                bk_printf("WEP\r\n");
                break;
            case SECURITY_TYPE_WPA_TKIP:
                bk_printf("TKIP\r\n");
                break;
            case SECURITY_TYPE_WPA2_AES:
                bk_printf("CCMP\r\n");
                break;
            case SECURITY_TYPE_WPA2_MIXED:
                bk_printf("MIXED\r\n");
                break;
            case SECURITY_TYPE_AUTO:
                bk_printf("AUTO\r\n");
                break;
            default:
                bk_printf("Error\r\n");
                break;
        }
    }
    
    if( uap_ip_is_start() )
    {
    	os_memset(&ap_info, 0x0, sizeof(network_InitTypeDef_ap_st));
    	bk_wlan_ap_para_info_get(&ap_info);
        os_memcpy(ssid, ap_info.wifi_ssid, 32);
        bk_printf("softap:ssid=%s,channel=%d,dhcp=%d,cipher_type:",
    		ssid, ap_info.channel,ap_info.dhcp_mode);
        switch(ap_info.security)
        {
           case SECURITY_TYPE_NONE:
                bk_printf("OPEN\r\n");
                break;
            case SECURITY_TYPE_WEP :
                bk_printf("WEP\r\n");
                break;
            case SECURITY_TYPE_WPA_TKIP:
                bk_printf("TKIP\r\n");
                break;
            case SECURITY_TYPE_WPA2_AES:
                bk_printf("CCMP\r\n");
                break;
            case SECURITY_TYPE_WPA2_MIXED:
                bk_printf("MIXED\r\n");
                break;
            case SECURITY_TYPE_AUTO:
                bk_printf("AUTO\r\n");
                break;
            default:
                bk_printf("Error\r\n");
                break;
        }
        bk_printf("ip=%s,gate=%s,mask=%s,dns=%s\r\n",
		    ap_info.local_ip_addr, ap_info.gateway_ip_addr, ap_info.net_mask, ap_info.dns_server_ip_addr);
    }
}

void demo_ip_app_init(void)
{
    IPStatusTypedef ipStatus;

	os_memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
	bk_wlan_get_ip_status(&ipStatus, STATION);
    
	bk_printf("dhcp=%d ip=%s gate=%s mask=%s mac=" MACSTR "\r\n",
				ipStatus.dhcp, ipStatus.ip, ipStatus.gate, 
				ipStatus.mask, MAC2STR((unsigned char*)ipStatus.mac));
}

// eof

