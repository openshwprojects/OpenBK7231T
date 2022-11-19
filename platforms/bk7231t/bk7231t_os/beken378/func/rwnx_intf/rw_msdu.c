#include "include.h"
#include "rw_msdu.h"
#include "rw_pub.h"
#include "str_pub.h"
#include "mem_pub.h"
#include "txu_cntrl.h"
#include "lwip/pbuf.h"
#include "prot/ip4.h"
#include "prot/ip6.h"
#include "prot/ethernet.h"

#include "arm_arch.h"

#if CFG_GENERAL_DMA
#include "general_dma_pub.h"
#endif

#include "param_config.h"
#include "fake_clock_pub.h"
#include "power_save_pub.h"

void ethernetif_input(int iface, struct pbuf *p);
UINT32 rwm_transfer_node(MSDU_NODE_T *node, u8 flag);
int bmsg_ps_handler_rf_ps_mode_real_wakeup(void);

LIST_HEAD_DEFINE(msdu_rx_list);

#if CFG_USE_AP_PS
#include "ps.h"
#include "app.h"
#define MAX_PS_STA_NUM          BROADCAST_STA_IDX_MIN   // CFG_STA_MAX
#define MAX_BUFFER_TIME         10000       // 10S

typedef struct sta_ps_st {
    beken_timer_t timer;
    struct list_head txing;
} STA_PS_ST, *STA_PS_PTR;

typedef struct rwm_ap_ps_st {
    BOOL active;
    STA_PS_ST sta_ps[MAX_PS_STA_NUM];
} AP_PS_ST, *AP_PS_PTR;

AP_PS_ST g_ap_ps = {0};
#endif

void rwm_push_rx_list(MSDU_NODE_T *node)
{
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    list_add_tail(&node->hdr, &msdu_rx_list);
    GLOBAL_INT_RESTORE();
}

MSDU_NODE_T *rwm_pop_rx_list(void)
{
    LIST_HEADER_T *tmp;
    LIST_HEADER_T *pos;
    MSDU_NODE_PTR node;

    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();

    node = NULLPTR;
    list_for_each_safe(pos, tmp, &msdu_rx_list)
    {
        list_del(pos);
        node = list_entry(pos, MSDU_NODE_T, hdr);

        break;
    }

    GLOBAL_INT_RESTORE();

    return node;
}

void rwm_flush_rx_list(void)
{
    MSDU_NODE_T *node_ptr;

    while(1)
    {
        node_ptr = rwm_pop_rx_list();
        if(node_ptr)
        {
            os_free(node_ptr);
        }
        else
        {
            break;
        }
    }
}

void rwm_tx_confirm(void *param)
{
	struct txdesc *txdesc = (struct txdesc *)param;
			
	if(txdesc && txdesc->host.msdu_node)
	{
		os_null_printf("flush_desc:0x%x\r\n", txdesc->host.msdu_node);

		if(txdesc->host.callback)
		{
			(*txdesc->host.callback)(txdesc->host.param);
		}
		
		os_free(txdesc->host.msdu_node);
		txdesc->host.msdu_node = NULL;
	}
}

void rwm_tx_msdu_renew(UINT8 *buf, UINT32 len, UINT8 *orig_addr)
{
#if CFG_GENERAL_DMA
    gdma_memcpy((void *)((UINT32)orig_addr + CFG_MSDU_RESV_HEAD_LEN), buf, len);
#else
    os_memmove((void *)((UINT32)orig_addr + CFG_MSDU_RESV_HEAD_LEN), buf, len);
#endif
}

UINT8 *rwm_get_msdu_content_ptr(MSDU_NODE_T *node)
{
    return (UINT8 *)((UINT32)node->msdu_ptr + CFG_MSDU_RESV_HEAD_LEN);
}

void rwm_txdesc_copy(struct txdesc *dst_local, ETH_HDR_PTR eth_hdr_ptr)
{
    struct hostdesc *host_ptr;

    host_ptr = &dst_local->host;

    os_memcpy(&host_ptr->eth_dest_addr, &eth_hdr_ptr->e_dest, sizeof(host_ptr->eth_dest_addr));
    os_memcpy(&host_ptr->eth_src_addr, &eth_hdr_ptr->e_src, sizeof(host_ptr->eth_src_addr));
}

int rwm_raw_frame_with_cb(uint8_t *buffer, int len, void *cb, void *param)
{
	int ret = 0;
	uint8_t *pkt = buffer;
	MSDU_NODE_T *node;
	UINT8 *content_ptr;
	UINT32 queue_idx = AC_VI;
	struct txdesc *txdesc_new;
	struct umacdesc *umac;

	node = rwm_tx_node_alloc(len);
	if (node == NULL) {
		goto exit;
	}

	rwm_tx_msdu_renew(pkt, len, node->msdu_ptr);
	content_ptr = rwm_get_msdu_content_ptr(node);

	txdesc_new = tx_txdesc_prepare(queue_idx);
	if(txdesc_new == NULL || TXDESC_STA_USED == txdesc_new->status) {
		rwm_node_free(node);
		goto exit;
	}

	txdesc_new->status = TXDESC_STA_USED;
	txdesc_new->host.flags = TXU_CNTRL_MGMT;
	txdesc_new->host.orig_addr = (UINT32)node->msdu_ptr;
	txdesc_new->host.packet_addr = (UINT32)content_ptr;
	txdesc_new->host.packet_len = len;
	txdesc_new->host.status_desc_addr = (UINT32)content_ptr;
	txdesc_new->host.tid = 0xff;
	txdesc_new->host.callback = (mgmt_tx_cb_t)cb;
	txdesc_new->host.param = param;
	txdesc_new->host.msdu_node = (void *)node;

	umac = &txdesc_new->umac;
	umac->payl_len = len;
	umac->head_len = 0;
	umac->tail_len = 0;
	umac->hdr_len_802_2 = 0;

	umac->buf_control = &txl_buffer_control_24G;

	txdesc_new->lmac.agg_desc = NULL;
	txdesc_new->lmac.hw_desc->cfm.status = 0;

	ps_set_data_prevent();
	nxmac_pwr_mgt_setf(0);
#if CFG_USE_STA_PS
	bmsg_ps_handler_rf_ps_mode_real_wakeup();
#endif

	txl_cntrl_push(txdesc_new, queue_idx);
	
	ret = len;

exit:
	return ret;
}

MSDU_NODE_T *rwm_tx_node_alloc(UINT32 len)
{
    UINT8 *buff_ptr;
    MSDU_NODE_T *node_ptr = 0;

    node_ptr = (MSDU_NODE_T *)os_malloc(sizeof(MSDU_NODE_T)
                                        + CFG_MSDU_RESV_HEAD_LEN
                                        + len
                                        + CFG_MSDU_RESV_TAIL_LEN);

    if(NULL == node_ptr)
    {
        goto alloc_exit;
    }

    buff_ptr = (UINT8 *)((UINT32)node_ptr + sizeof(MSDU_NODE_T));

    node_ptr->msdu_ptr = buff_ptr;
    node_ptr->len = len;

alloc_exit:
    return node_ptr;
}

void rwm_node_free(MSDU_NODE_T *node)
{
    ASSERT(node);
    os_free(node);
}

UINT8 *rwm_rx_buf_alloc(UINT32 len)
{
    return (UINT8 *)os_malloc(len);
}

UINT32 rwm_get_rx_valid(void)
{
    UINT32 count = 0;
    LIST_HEADER_T *tmp;
    LIST_HEADER_T *pos;
    LIST_HEADER_T *head = &msdu_rx_list;

    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    list_for_each_safe(pos, tmp, head)
    {
        count ++;
    }
    GLOBAL_INT_RESTORE();

    return ((count >= MSDU_RX_MAX_CNT) ? 0 : 1);
}

UINT32 rwm_get_rx_valid_node_len(void)
{
    UINT32 len = 0;
    LIST_HEADER_T *tmp;
    LIST_HEADER_T *pos;
    MSDU_NODE_PTR node;

    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();

    node = NULLPTR;
    list_for_each_safe(pos, tmp, &msdu_rx_list)
    {
        node = list_entry(pos, MSDU_NODE_T, hdr);
        len = node->len;
        break;
    }

    GLOBAL_INT_RESTORE();

    return len;
}

#if CFG_USE_AP_PS
extern void bmsg_txing_sender(uint8_t sta_idx);
void rwm_push_txing_list(MSDU_NODE_T *node, UINT8 sta_idx)
{
    GLOBAL_INT_DECLARATION();

    if(sta_idx >= MAX_PS_STA_NUM)
        return;

    GLOBAL_INT_DISABLE();
    list_add_tail(&node->hdr, &g_ap_ps.sta_ps[sta_idx].txing);
    GLOBAL_INT_RESTORE();
}

MSDU_NODE_T *rwm_pop_txing_list(UINT8 sta_idx)
{
    LIST_HEADER_T *tmp;
    LIST_HEADER_T *pos;
    MSDU_NODE_PTR node;

    GLOBAL_INT_DECLARATION();

    if(sta_idx >= MAX_PS_STA_NUM)
        return NULL;

    GLOBAL_INT_DISABLE();

    node = NULLPTR;
    list_for_each_safe(pos, tmp, &g_ap_ps.sta_ps[sta_idx].txing)
    {
        list_del(pos);
        node = list_entry(pos, MSDU_NODE_T, hdr);
        break;
    }

    GLOBAL_INT_RESTORE();

    return node;
}

UINT32 rwm_txling_list_node_count(UINT8 sta_idx)
{
    if(sta_idx >= MAX_PS_STA_NUM)
        return 0;

    return list_size(&g_ap_ps.sta_ps[sta_idx].txing);
}

void rwm_flush_txing_list(UINT8 sta_idx)
{
    MSDU_NODE_T *node_ptr;
    int ret;

    if(sta_idx >= MAX_PS_STA_NUM)
        return;     

    if(rwm_txling_list_node_count(sta_idx)) 
    {
        os_printf("flush buffered node, staid:%d\r\n", sta_idx);
        while(1) {
            node_ptr = rwm_pop_txing_list(sta_idx);
            if(node_ptr)
                os_free(node_ptr);     
            else
                break;
        }
    }

    if(rtos_is_timer_running(&g_ap_ps.sta_ps[sta_idx].timer)) 
    {
        os_printf("stop ap ps timer, staid:%d\r\n", sta_idx);
        ret = rtos_stop_timer(&g_ap_ps.sta_ps[sta_idx].timer);
        ASSERT(0 == ret);   
    }
}

void rwm_ps_tranfer_node(MSDU_NODE_T *node)
{
    UINT8 vif_idx, sta_idx;
    BOOL need_buffer = false;

    if(!node)
        return;

    vif_idx = node->vif_idx;
    sta_idx = node->sta_idx;
    
    if(!rwm_mgmt_is_ap_inface(vif_idx)) 
    {
        #if NX_POWERSAVE
        txl_cntrl_inc_pck_cnt();
        #endif    
        // normal transfer
        rwm_transfer_node(node, 0);
    }
    else 
    {
        // only ap mode need check peer stations in ps mode
        if(!g_ap_ps.active) 
        {
            #if NX_POWERSAVE
            txl_cntrl_inc_pck_cnt();
            #endif        
            // normal transfer
            rwm_transfer_node(node, 0);
        }
        else
        {
             if((sta_mgmt_is_in_ps(sta_idx) || rwm_txling_list_node_count(sta_idx)) 
                && (sta_idx < MAX_PS_STA_NUM))
             {
                if(!rwm_txling_list_node_count(sta_idx)) 
                {
                    u8 vif_idx = sta_mgmt_get_vif_idx(sta_idx);
                    u16 aid = sta_mgmt_get_aid(sta_idx);
                    int ret;
                    
                    //os_printf("on bcn tim: vif:%d, aid:%d, sta:%d\r\n", vif_idx, aid, sta_idx);
                    
                    //rwnx_send_me_uapsd_traffic_ind(sta_idx, 1);
                    rw_msg_send_tim_update(vif_idx, aid, 1);
                    if(!rtos_is_timer_running(&g_ap_ps.sta_ps[sta_idx].timer)) {
                        ret = rtos_start_timer(&g_ap_ps.sta_ps[sta_idx].timer);
	                    ASSERT(0 == ret);
                    }
                }
                
                // add this node to txing list
                //os_printf("addto txing list node:%p, sta:%d\r\n", node, sta_idx);
                rwm_push_txing_list(node, sta_idx);
             } 
             else 
             {
                #if NX_POWERSAVE
                txl_cntrl_inc_pck_cnt();
                #endif
                // normal transfer
                rwm_transfer_node(node, 0);
             }   
        }
    }   
}

void rwm_msdu_send_txing_node(UINT8 sta_idx)
{
    MSDU_NODE_T *node = NULL;
    struct txdesc *txdesc_new = NULL;
    UINT32 node_left;
    
    node = rwm_pop_txing_list(sta_idx);
    if(!node)
        return;
    
    node_left = rwm_txling_list_node_count(sta_idx);
    
    #if NX_POWERSAVE
    txl_cntrl_inc_pck_cnt();
    #endif

    //os_printf("pop txing list node:%p, sta:%d\r\n",node, sta_idx);
    rwm_transfer_node(node, TXU_CNTRL_MORE_DATA);

    if(node_left && !sta_mgmt_is_in_ps(sta_idx)) {
        // trigger sending txing again
        bmsg_txing_sender(sta_idx);
    } 
    else if(!node_left) 
    {
        u8 vif_idx = sta_mgmt_get_vif_idx(sta_idx);
        u16 aid = sta_mgmt_get_aid(sta_idx);
        
        //rwnx_send_me_uapsd_traffic_ind(sta_idx, 0);
        rw_msg_send_tim_update(vif_idx, aid, 0);
    }
}

void rwm_msdu_ps_change_ind_handler(void *msg)  
{
    struct ke_msg *msg_ptr = (struct ke_msg *)msg;
    struct mm_ps_change_ind *ind;
    UINT32 node_left;
    int ret;

    if(!msg_ptr || !msg_ptr->param)
        return;
   
    ind = (struct mm_ps_change_ind *)msg_ptr->param;
    node_left = rwm_txling_list_node_count(ind->sta_idx);
    
    if((ind->ps_state == PS_MODE_OFF) && node_left) {
        // trigger txing sending
        bmsg_txing_sender(ind->sta_idx);
        
        if(rtos_is_timer_running(&g_ap_ps.sta_ps[ind->sta_idx].timer)) {
            ret = rtos_stop_timer(&g_ap_ps.sta_ps[ind->sta_idx].timer);
            ASSERT(0 == ret);
        }
    } else if((ind->ps_state == PS_MODE_ON) && node_left)  {
        // do something
        if(!rtos_is_timer_running(&g_ap_ps.sta_ps[ind->sta_idx].timer)) {
            ret = rtos_start_timer(&g_ap_ps.sta_ps[ind->sta_idx].timer);
	        ASSERT(0 == ret);
        }
    }
}

void rwm_msdu_ap_ps_timeout(void *data)
{
    u8 sta_idx = (u32)data;

    rwm_flush_txing_list(sta_idx);
}
#endif

void rwm_msdu_init(void)
{
    #if CFG_USE_AP_PS
    g_ap_ps.active = true;

    for(int i=0; i<MAX_PS_STA_NUM; i++) 
    {
        int ret; 
        INIT_LIST_HEAD(&g_ap_ps.sta_ps[i].txing);
        
    	ret = rtos_init_timer(&g_ap_ps.sta_ps[i].timer, 
			                   MAX_BUFFER_TIME,  
			                   rwm_msdu_ap_ps_timeout, 
			                   (void *)i);  
        ASSERT(0 == ret);  
    }
    #endif
}

/*
 * IEEE802.11-2016: Table 10-1—UP-to-AC mappings
 */
uint8_t ipv4_ieee8023_dscp(UINT8 *buf)
{
	uint8_t tos;
	struct ip_hdr *hdr = (struct ip_hdr *)buf;

	tos = IPH_TOS(hdr);

	return (tos & 0xfc) >> 5;
}

/* extract flow control field */
uint8_t ipv6_ieee8023_dscp(UINT8 *buf)
{
	uint8_t tos;
	struct ip6_hdr *hdr = (struct ip6_hdr *)buf;

	tos = IP6H_FL(hdr);

	return (tos & 0xfc) >> 5;
}

/*
 * get user priority from @buf.
 * ipv4 dscp/tos, ipv6 flow control. for eapol packets, disable qos.
 */
uint8_t classify8021d(UINT8 *buf)
{
#ifdef CFG_WFA_CERTIFICATION
	struct eth_hdr *ethhdr = (struct eth_hdr *)buf;

	switch (PP_HTONS(ethhdr->type)) {
	case ETHTYPE_IP:
		return ipv4_ieee8023_dscp(ethhdr + 1);
	case ETHTYPE_IPV6:
		return ipv6_ieee8023_dscp(ethhdr + 1);
	case ETH_P_PAE:
		return 7;	/* TID7 highest user priority */
	default:
		return 0;
	}
#else
	return 4;		// TID4: mapped to AC_VI
#endif
}

UINT32 rwm_transfer(UINT8 vif_idx, UINT8 *buf, UINT32 len, int sync, void *args)
{
    UINT32 ret = 0;
    MSDU_NODE_T *node;
    ETH_HDR_PTR eth_hdr_ptr;

    ret = RW_FAILURE;
    node = rwm_tx_node_alloc(len);
    if(NULL == node)
    {
        #if NX_POWERSAVE
        txl_cntrl_dec_pck_cnt();
        #endif

        os_printf("rwm_transfer no node\r\n");
        goto tx_exit;
    }
    rwm_tx_msdu_renew(buf, len, node->msdu_ptr);

    eth_hdr_ptr = (ETH_HDR_PTR)buf;
    node->vif_idx = vif_idx;
	node->sync = sync;
	node->args = args;
    node->sta_idx = rwm_mgmt_tx_get_staidx(vif_idx,
                             &eth_hdr_ptr->e_dest);

#if CFG_USE_AP_PS
    ret = rwm_ps_tranfer_node(node);
#else
    ret = rwm_transfer_node(node, 0);
#endif

tx_exit:
    return ret;
}

void ieee80211_data_tx_cb(void *param)
{
	struct txdesc *txdesc_new = (struct txdesc *)param;
	MSDU_NODE_T *node = (MSDU_NODE_T *)txdesc_new->host.msdu_node;
	struct tx_hd *txhd = &txdesc_new->lmac.hw_desc->thd;
	struct ieee80211_tx_cb *cb = (struct ieee80211_tx_cb *)node->args;
	uint32_t status = txhd->statinfo;

	if(0 == node)
	{
		os_printf("zero_node\r\n");
		return;
	}
	
	if (status & FRAME_SUCCESSFUL_TX_BIT /*DESC_DONE_SW_TX_BIT*/) {
		cb->result = RW_SUCCESS;
	} else {
		cb->result = RW_FAILURE;
	}

	rtos_set_semaphore(&cb->sema);
}

int sta_11n_nss(uint8_t *mcs_set)
{
	int i;
	int nss = 0;	/* spartial stream num */

	// Go through the MCS map to find out one valid mcs
	for (i = 0; i < 4; i++) {	// 4 == sizeof(rc_ss->rate_map.ht)
		if (mcs_set[i] != 0)
			nss++;
	}

	return nss;
}

int qos_need_enabled(struct sta_info_tag *sta)
{
	int i;
	struct mac_rateset *rate_set;
	int rate_22mbps_found = 0;
	int nss = 0;

	if (!sta)
		return 0;
	if (!(sta->info.capa_flags & STA_QOS_CAPA))
		return 0;

#define CONFIG_WRT3200_ISSUE
#ifdef CONFIG_WRT3200_ISSUE
	/* workaround for WRT3200 */

	/* find rate 22mbps */
	rate_set = &sta->info.rate_set;
	for (i = 0; i < rate_set->length; i++)	 {
		if (rate_set->array[i] == 0x2c) {
			rate_22mbps_found = 1;
			break;
		}
	}

	/* ignore rate doesn't contain 22Mbps */
	if (!rate_22mbps_found)
		return 0;

	/* ignore 11AC */
	if (sta->info.capa_flags & STA_VHT_CAPA)
		return 0;

	/* ignore non-HT */
	if (!(sta->info.capa_flags & STA_HT_CAPA))
		return 0;

	/* ingore nss != 3*/
	nss = sta_11n_nss(sta->info.ht_cap.mcs_rate);
	if (nss != 3)
		return 0;

	/* ignore LDPC and DSSS/CCK in 40M not both enabled */
	if (!((sta->info.ht_cap.ht_capa_info & MAC_HTCAPA_LDPC) && 
		(sta->info.ht_cap.ht_capa_info & MAC_HTCAPA_DSSS_CCK_40)))
		return 0;
#endif

	return 1;
}

UINT32 rwm_transfer_node(MSDU_NODE_T *node, u8 flag)
{
    UINT8 tid;
    UINT32 ret = RW_FAILURE;
    UINT8 *content_ptr;

    UINT32 queue_idx;

    ETH_HDR_PTR eth_hdr_ptr;
    struct txdesc *txdesc_new;
	struct sta_info_tag *sta;
	struct vif_info_tag *vif;

    if(!node) {
        #if NX_POWERSAVE
        txl_cntrl_dec_pck_cnt();
        #endif
        
        goto tx_exit;
    }
    
    content_ptr = rwm_get_msdu_content_ptr(node);
    eth_hdr_ptr = (ETH_HDR_PTR)content_ptr;

#if CFG_RWNX_QOS_MSDU
	vif = rwm_mgmt_vif_idx2ptr(node->vif_idx);
	if (likely(vif->active)) {
		sta = &sta_info_tab[vif->u.sta.ap_id];
		if (qos_need_enabled(sta)) {
			int i;
			tid = classify8021d((UINT8*)eth_hdr_ptr);
			/* check admission ctrl */
			for (i = mac_tid2ac[tid]; i >= 0; i--)
				if (!(vif->bss_info.edca_param.acm & BIT(i)))
					break;
			if (i < 0)
				goto tx_exit;
			queue_idx = i;	/* AC_* */
		} else {
			/*
			 * non-WMM STA
			 *
			 * CWmin 15, CWmax 1023, AIFSN 2, TXOP 0. set these values when joining with this BSS.
			 */
			tid = 0xFF;
			queue_idx = AC_VI;
		}
	} else {
		tid = 0xFF;
	    queue_idx = AC_VI;
	}
#else
	tid = 0xff;
    queue_idx = AC_VI;
#endif

    txdesc_new = tx_txdesc_prepare(queue_idx);
    if(TXDESC_STA_USED == txdesc_new->status)
    {
        rwm_node_free(node);
        #if NX_POWERSAVE
        txl_cntrl_dec_pck_cnt();
        #endif
        
        os_printf("rwm_transfer no txdesc \r\n");
        goto tx_exit;
    }

    txdesc_new->status = TXDESC_STA_USED;
    rwm_txdesc_copy(txdesc_new, eth_hdr_ptr);

    txdesc_new->host.flags            = flag;
#if NX_AMSDU_TX
    txdesc_new->host.orig_addr[0]     = (UINT32)node->msdu_ptr;
    txdesc_new->host.packet_addr[0]   = (UINT32)content_ptr + 14;
    txdesc_new->host.packet_len[0]    = node->len - 14;
    txdesc_new->host.packet_cnt       = 1;
#else
    txdesc_new->host.orig_addr        = (UINT32)node->msdu_ptr;
    txdesc_new->host.packet_addr      = (UINT32)content_ptr + 14;
    txdesc_new->host.packet_len       = node->len - 14;
#endif
    txdesc_new->host.status_desc_addr = (UINT32)content_ptr + 14;
    txdesc_new->host.ethertype        = eth_hdr_ptr->e_proto;
    txdesc_new->host.tid              = tid;

    txdesc_new->host.vif_idx          = node->vif_idx;
    txdesc_new->host.staid            = node->sta_idx;   
	txdesc_new->host.msdu_node		  = (void *)node;

	if (node->sync) 
	{
		txdesc_new->host.callback		  = (mgmt_tx_cb_t)ieee80211_data_tx_cb;
		txdesc_new->host.param			  = (void *)txdesc_new;
	} 
	else 
	{
		txdesc_new->host.callback = 0;
	}

    txdesc_new->lmac.agg_desc = NULL;
    txdesc_new->lmac.hw_desc->cfm.status = 0;
	
    txu_cntrl_push(txdesc_new, queue_idx);

tx_exit:
    return ret;
}

UINT32 rwm_get_rx_free_node(struct pbuf **p_ret, UINT32 len)
{
    struct pbuf *p;

    p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    *p_ret = p;

    return RW_SUCCESS;
}

UINT32 rwm_upload_data(RW_RXIFO_PTR rx_info)
{
    struct pbuf *p = (struct pbuf *)rx_info->data;

    os_null_printf("s:%d, v:%d, d:%d, r:%d, c:%d, l:%d, %p\r\n",
                   rx_info->sta_idx,
                   rx_info->vif_idx,
                   rx_info->dst_idx,
                   rx_info->rssi,
                   rx_info->center_freq,
                   rx_info->length,
                   rx_info->data);

    ethernetif_input(rx_info->vif_idx, p);

    return RW_SUCCESS;
}

UINT32 rwm_uploaded_data_handle(UINT8 *upper_buf, UINT32 len)
{
    UINT32 count;
    UINT32 ret = RW_FAILURE;
    MSDU_NODE_T *node_ptr;

    node_ptr = rwm_pop_rx_list();
    if(node_ptr)
    {
        count = MIN(len, node_ptr->len);
#if CFG_GENERAL_DMA
        gdma_memcpy(upper_buf, node_ptr->msdu_ptr, count);
#else
        os_memcpy(upper_buf, node_ptr->msdu_ptr, count);
#endif
        ret = count;

        os_free(node_ptr);
        node_ptr = NULL;
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
VIF_INF_PTR rwm_mgmt_vif_idx2ptr(UINT8 vif_idx)
{
    VIF_INF_PTR vif_entry = NULL;

    if(vif_idx < NX_VIRT_DEV_MAX)
        vif_entry = &vif_info_tab[vif_idx];

    return vif_entry;
}

VIF_INF_PTR rwm_mgmt_vif_type2ptr(UINT8 vif_type)
{
    VIF_INF_PTR vif_entry = NULL;
    UINT32 i;

    for(i = 0; i < NX_VIRT_DEV_MAX; i++)
    {
        vif_entry = &vif_info_tab[i];
        if(vif_entry->type == vif_type)
            break;
    }

    if(i == NX_VIRT_DEV_MAX)
        vif_entry = NULL;

    return vif_entry;
}

STA_INF_PTR rwm_mgmt_sta_idx2ptr(UINT8 staid)
{
    STA_INF_PTR sta_entry = NULL;

    if(staid < NX_REMOTE_STA_MAX)
        sta_entry = &sta_info_tab[staid];

    return sta_entry;
}

STA_INF_PTR rwm_mgmt_sta_mac2ptr(void *mac)
{
    UINT32 i;
    STA_INF_PTR sta_entry = NULL;

    for(i = 0; i < NX_REMOTE_STA_MAX; i++)
    {
        sta_entry = &sta_info_tab[i];
        if(MAC_ADDR_CMP((void *)&sta_entry->mac_addr, mac))
            break;
    }

    return sta_entry;
}

UINT8 rwm_mgmt_sta_mac2idx(void *mac)
{
    UINT32 i;
    UINT8 staid = 0xff;
    STA_INF_PTR sta_entry = NULL;

    for(i = 0; i < NX_REMOTE_STA_MAX; i++)
    {
        sta_entry = &sta_info_tab[i];
        if(MAC_ADDR_CMP((void *)&sta_entry->mac_addr, mac))
            break;
    }
    if(i < NX_REMOTE_STA_MAX)
        staid = i;

    return staid;
}

UINT8 rwm_mgmt_vif_mac2idx(void *mac)
{
    VIF_INF_PTR vif_entry = NULL;
    UINT8 vif_idx = 0xff;
    UINT32 i;

    for(i = 0; i < NX_VIRT_DEV_MAX; i++)
    {
        vif_entry = &vif_info_tab[i];
        if(MAC_ADDR_CMP((void *)&vif_entry->mac_addr, mac))
            break;
    }

    if(i < NX_VIRT_DEV_MAX)
        vif_idx = i;

    return vif_idx;
}

UINT8 rwm_mgmt_vif_name2idx(char *name)
{
    VIF_INF_PTR vif_entry = NULL;
    struct netif *lwip_if;
    UINT8 vif_idx = 0xff;
    UINT32 i;

    for(i = 0; i < NX_VIRT_DEV_MAX; i++)
    {
        vif_entry = &vif_info_tab[i];
        if(vif_entry->priv)
        {
            lwip_if = (struct netif *)vif_entry->priv;
            if (!os_strncmp(lwip_if->hostname, name, os_strlen(lwip_if->hostname)))
            {
                break;
            }
        }
    }

    if(i < NX_VIRT_DEV_MAX)
        vif_idx = i;

    return vif_idx;
}

UINT8 rwm_mgmt_get_hwkeyidx(UINT8 vif_idx, UINT8 staid)
{
    UINT8 hw_key_idx = MM_SEC_MAX_KEY_NBR + 1;
    struct key_info_tag *key = NULL;

    VIF_INF_PTR vif_entry = NULL;
    STA_INF_PTR sta_entry = NULL;

    if(staid == 0xff)   // group key
    {
        vif_entry = rwm_mgmt_vif_idx2ptr(vif_idx);
        if(vif_entry)
            key = vif_entry->default_key;
    }
    else
    {
        sta_entry = rwm_mgmt_sta_idx2ptr(staid);
        if(sta_entry)
            key = *(sta_entry->sta_sec_info.cur_key);
    }

    if(key)
    {
        hw_key_idx = key->hw_key_idx;
    }

    return hw_key_idx;
}

void rwm_mgmt_set_vif_netif(struct netif *net_if)
{
    VIF_INF_PTR vif_entry = NULL;
    UINT8 vif_idx;

    if(!net_if)
        return;

    vif_idx = rwm_mgmt_vif_mac2idx(net_if->hwaddr);
    vif_entry = rwm_mgmt_vif_idx2ptr(vif_idx);

    if(vif_entry)
    {
        vif_entry->priv = net_if;
        net_if->state = (void *)vif_entry;
    }
    else
    {
        os_printf("warnning: set_vif_netif failed\r\n");
    }
}

struct netif *rwm_mgmt_get_vif2netif(UINT8 vif_idx)
{
    VIF_INF_PTR vif_entry = NULL;
    struct netif *netif = NULL;

    vif_entry = rwm_mgmt_vif_idx2ptr(vif_idx);

    if(vif_entry)
        netif = (struct netif *)vif_entry->priv;

    return netif;
}

UINT8 rwm_mgmt_get_netif2vif(struct netif *netif)
{
    UINT8 vif_idx = 0xff;
    VIF_INF_PTR vif_entry = NULL;

    if(netif && netif->state)
    {
        vif_entry = (VIF_INF_PTR)netif->state;
        vif_idx = vif_entry->index;
    }

    return vif_idx;
}

UINT8 rwm_mgmt_tx_get_staidx(UINT8 vif_idx, void *dstmac)
{
    UINT8 staid = 0xff;
    VIF_INF_PTR vif_entry = NULL;

    vif_entry = rwm_mgmt_vif_idx2ptr(vif_idx);

    if(vif_entry)
    {
        if(vif_entry->type == VIF_STA)
        {
            staid = vif_entry->u.sta.ap_id;
        }
        else if(vif_entry->type == VIF_AP)
        {
            staid = rwm_mgmt_sta_mac2idx(dstmac);
        }
    }

    if(staid == 0xff)
    {
        staid = VIF_TO_BCMC_IDX(vif_idx);
    }

    return staid;
}

u8 rwn_mgmt_is_only_sta_role_add(void)
{
    VIF_INF_PTR vif_entry = (VIF_INF_PTR)rwm_mgmt_is_vif_first_used();

    if(!vif_entry)
        return 0;

    if(vif_entry->type == VIF_STA)
        return 1;

    return 0;
}

#include "lwip/sockets.h"
extern uint8_t* dhcp_lookup_mac(uint8_t *chaddr);

void rwn_mgmt_show_vif_peer_sta_list(UINT8 role)
{
    struct vif_info_tag *vif = (VIF_INF_PTR)rwm_mgmt_is_vif_first_used();
    struct sta_info_tag *sta;
    UINT8 num = 0;

    while(vif) {
        if ( vif->type == role) { 
            sta = (struct sta_info_tag *)co_list_pick(&vif->sta_list);
            while (sta != NULL) 
            {
                UINT8 *macptr = (UINT8*)sta->mac_addr.array;
                UINT8 *ipptr = NULL;

                if(role == VIF_AP) {
                    ipptr = dhcp_lookup_mac(macptr);
                } else if (role == VIF_STA){
                    struct netif *netif = (struct netif *)vif->priv;
                    ipptr = (UINT8*)inet_ntoa(netif->gw);
                }
                
                os_printf("%d: mac:%02x-%02x-%02x-%02x-%02x-%02x, ip:%s\r\n", num++,
                    macptr[0], macptr[1], macptr[2], 
                    macptr[3], macptr[4], macptr[5],ipptr);
                
                sta = (struct sta_info_tag *)co_list_next(&sta->list_hdr);
            }
        }
        vif = (VIF_INF_PTR) rwm_mgmt_next(vif);
    } 
}

// eof

