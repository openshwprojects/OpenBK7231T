/**
 ****************************************************************************************
 *
 * @file appm_task.c
 *
 * @brief RW APP Task implementation
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"          // SW configuration

#if (BLE_APP_PRESENT)
#include <string.h>
#include "app_task.h"              // Application Manager Task API
#include "application.h"                      // Application Manager Definition
#include "gapc_task.h"            // GAP Controller Task API
#include "gapm_task.h"          // GAP Manager Task API
#include "gattc_task.h"
#include "architect.h"                    // Platform Definitions

#include "kernel_timer.h"             // Kernel timer
//#include "kernel_task.h"

#if (BLE_APP_FFF0)
#include "app_fff0.h"              // fff0 Module Definition
#include  "fff0s_task.h"
#endif //(BLE_APP_FFF0)


#if (BLE_APP_FFE0)
#include "app_ffe0.h"              // ffe0 Module Definition
#endif //(BLE_APP_FFE0)

#if (BLE_APP_COMM)
#include "app_comm.h"
#include "comm_task.h"
#endif

#if BLE_APP_SDP
#include "sdp_service.h"
#include "sdp_service_task.h"
#include "app_sdp.h"
#endif

#if (BLE_APP_ANCSC)
#include "app_ancsc.h"
#include "ancsc_task.h"
#endif //(BLE_APP_ANCSC)

#if (BLE_APP_SEC)
#include "app_sec.h"              // Security Module Definition
#endif //(BLE_APP_SEC)

#if (BLE_APP_HT)
#include "app_ht.h"               // Health Thermometer Module Definition
#include "htpt_task.h"
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
#include "app_dis.h"              // Device Information Module Definition
#include "diss_task.h"
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"             // Battery Module Definition
#include "bass_task.h"
#endif //(BLE_APP_BATT)

#if (BLE_APP_HID)
#include "app_hid.h"              // HID Module Definition
#include "hogpd_task.h"
#endif //(BLE_APP_HID)

#ifdef BLE_APP_AM0
#include "am0_application.h"             // Audio Mode 0 Application
#endif //defined(BLE_APP_AM0)

#if (BLE_APP_OADS)
#include "app_oads.h"             
#include "oads_task.h"              
#endif //(BLE_APP_OADS)

#if (DISPLAY_SUPPORT)
#include "app_display.h"         // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#include "BK3435_reg.h"

#include "reg_ble_em_cs.h"
#include "lld.h"
#include "ble.h"
#include "ble_pub.h"
#include "ble_api.h"

//this flag use indicate adv timeout
static bool adv_timeout_flag = false;
/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
extern char *dev_str_name;
extern struct gap_bdaddr  con_bdaddr;

static uint8_t appm_get_handler(const struct kernel_state_handler *handler_list,
                                kernel_msg_id_t msgid,
                                void *param,
                                kernel_task_id_t src_id)
{
    // Counter
    uint8_t counter;

    // Get the message handler function by parsing the message table
    for (counter = handler_list->msg_cnt; 0 < counter; counter--)
    {
			
        struct kernel_msg_handler handler = (struct kernel_msg_handler)(*(handler_list->msg_table + counter - 1));
			
        if ((handler.id == msgid) ||
            (handler.id == KERNEL_MSG_DEFAULT_HANDLER))
        {
            // If handler is NULL, message should not have been received in this state
            ASSERT_ERR(handler.func);

            return (uint8_t)(handler.func(msgid, param, TASK_APP, src_id));
        }
    }

    // If we are here no handler has been found, drop the message
    return (KERNEL_MSG_CONSUMED);
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_adv_timeout_handler(kernel_msg_id_t const msgid,
                                   void const *param,
                                   kernel_task_id_t const dest_id,
                                   kernel_task_id_t const src_id)
{
//	UART_PRINTF("%s\r\n", __func__);

	adv_timeout_flag = true;
	
	// Stop advertising
    appm_stop_advertising();

	    
    return (KERNEL_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles ready indication from the GAP. - Reset the stack
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_device_ready_ind_handler(kernel_msg_id_t const msgid,
                                         void const *param,
                                         kernel_task_id_t const dest_id,
                                         kernel_task_id_t const src_id)
{
    // Application has not been initialized
    ASSERT_ERR(kernel_state_get(dest_id) == APPM_INIT);

    // Reset the stack
    struct gapm_reset_cmd* cmd = KERNEL_MSG_ALLOC(GAPM_RESET_CMD,
                                              TASK_GAPM, TASK_APP,
                                              gapm_reset_cmd);

    cmd->operation = GAPM_RESET;

    kernel_msg_send(cmd);

    return (KERNEL_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP manager command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_cmp_evt_handler(kernel_msg_id_t const msgid,
                                struct gapm_cmp_evt const *param,
                                kernel_task_id_t const dest_id,
                                kernel_task_id_t const src_id)
{
    bk_printf("gapm_cmp_evt_handler operation = 0x%x, status = 0x%x \r\n", param->operation, param->status);
    switch(param->operation)
    {
        // Reset completed
        case (GAPM_RESET):
        {
            if(param->status == GAP_ERR_NO_ERROR)
            {			
                // Set Device configuration
                struct gapm_set_dev_config_cmd* cmd = KERNEL_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD,
	                                                                   TASK_GAPM, TASK_APP,
                                                                   gapm_set_dev_config_cmd);
                // Set the operation
                cmd->operation = GAPM_SET_DEV_CONFIG;
                // Set the device role - Central
                cmd->role      = GAP_ROLE_ALL;
                // Set Data length parameters
                cmd->sugg_max_tx_octets = BLE_MIN_OCTETS;
                cmd->sugg_max_tx_time   = BLE_MIN_TIME;
								
                //cmd->max_mtu = 256;
                //Do not support secure connections
                cmd->pairing_mode = GAPM_PAIRING_LEGACY;
                #if (BLE_APP_HID)
                // Enable Slave Preferred Connection Parameters present 
                cmd->att_cfg = GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN;
                #endif //(BLE_APP_HID)

                #ifdef BLE_APP_AM0
                cmd->addr_type   = GAPM_CFG_ADDR_HOST_PRIVACY;
                cmd->audio_cfg   = GAPM_MASK_AUDIO_AM0_SUP;
                #endif // BLE_APP_AM0

                // load IRK
                memcpy(cmd->irk.key, app_env.loc_irk, KEY_LEN);

				// Send message
				kernel_msg_send(cmd);				
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        break;
        case (GAPM_PROFILE_TASK_ADD):
        {
#if (BLE_APP_CLIENT)
			if((BLE_ROLE_MASTER == ble_get_role_mode()) && (kernel_state_get(dest_id) == APPM_CONNECTED))
			{
				uint8_t find;
				find = sdp_enable_all_server_ntf_ind(1);
				bk_printf("find = %d\r\n",find);
			}
#endif            

            // Add the next requested service
            if (BLE_ROLE_MASTER != ble_get_role_mode())
            {
                // Go to the ready state
                kernel_state_set(TASK_APP, APPM_READY);
							
                if(param->status == GAP_ERR_NO_ERROR)
                {
                    if(ble_event_cb)
                    {
                        ble_event_cb(BLE_CREATE_DB_OK, NULL);
                    }
                }
                else
                {
                    if(ble_event_cb)
                    {
                        ble_event_cb(BLE_CREATE_DB_FAIL, NULL);
                    }					
                }
            }
        }
	
        break;
        // Device Configuration updated
        case (GAPM_SET_DEV_CONFIG):
        {
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            // Go to the create db state
            // Add the first required service in the database
            // and wait for the PROFILE_ADDED_IND

            kernel_state_set(TASK_APP, APPM_READY);

            if(ble_event_cb)
            {
                ble_event_cb(BLE_STACK_OK, NULL);
            }					

        }
        break;	

		case (GAPM_SCAN_ACTIVE):
		case (GAPM_SCAN_PASSIVE):
		{
			if (param->status == GAP_ERR_CANCELED)
			{
				if (kernel_state_get(TASK_APP) == APPM_READY)
				{
					bk_printf("SCAN STOP CANCELED GAPM_SCAN_PASSIVE  OK\r\n");
				}

			}
		    /*
			if (kernel_state_get(TASK_APP) == APPM_WAIT_SCANNEND)
			{
				UART_PRINTF("CON CANCELED GAPM_SCAN_PASSIVE  OK\r\n");
				kernel_state_set(TASK_APP, APPM_READY);
				appm_start_connencting(con_bdaddr);
			}
			*/
		}
		break;

		case (GAPM_CONNECTION_DIRECT):
		{
			if (param->status == GAP_ERR_CANCELED)
			{
				if (kernel_state_get(TASK_APP) == APPM_CONNECTING)
				{
					kernel_state_set(TASK_APP, APPM_READY);
					bk_printf("CANCELED GAPM_START_CONNECTION_CMD \r\n");
				}
			}
		}
		break;

        case (GAPM_ADV_NON_CONN):
        case (GAPM_ADV_UNDIRECT):
        case (GAPM_ADV_DIRECT):
        case (GAPM_UPDATE_ADVERTISE_DATA):
        case (GAPM_ADV_DIRECT_LDC):
        {
            if (((param->status == GAP_ERR_CANCELED) || (param->status == GAP_ERR_TIMEOUT)) && (!adv_timeout_flag))
            {
                kernel_state_set(TASK_APP, APPM_READY);
				
            }
            //if device cancel advtising is adv timeout,then reset this flag
            if((param->status == GAP_ERR_CANCELED) && (adv_timeout_flag == true))
            {
                adv_timeout_flag = false;
            }
        }
     	break;	
		
		
		case (GAPM_RESOLV_ADDR):
        {
            //	 UART_PRINTF("GAPM_RESOLV_ADDR CMP status = %x\r\n",param->status);
            if(param->status == GAP_ERR_NO_ERROR)
            {
                struct gapc_connection_cfm *cfm = KERNEL_MSG_ALLOC(GAPC_CONNECTION_CFM,
                KERNEL_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                gapc_connection_cfm);

#if(BLE_APP_SEC)
                cfm->auth = app_sec_get_bond_status() ? GAP_AUTH_REQ_NO_MITM_BOND : GAP_AUTH_REQ_NO_MITM_NO_BOND; 
#else // !(BLE_APP_SEC)
                cfm->auth = GAP_AUTH_REQ_NO_MITM_NO_BOND;
#endif // (BLE_APP_SEC)
            }
		}
        break;

        default:
        {
            // Drop the message
        }
        break;
    }

    return (KERNEL_MSG_CONSUMED);
}


static int gapc_get_dev_info_req_ind_handler(kernel_msg_id_t const msgid,
        struct gapc_get_dev_info_req_ind const *param,
        kernel_task_id_t const dest_id,
        kernel_task_id_t const src_id)
{
    switch(param->req)
    {
        case GAPC_DEV_NAME:
        {
            struct gapc_get_dev_info_cfm * cfm = KERNEL_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                    src_id, dest_id,
                                                    gapc_get_dev_info_cfm, APP_DEVICE_NAME_MAX_LEN);
            cfm->req = param->req;
            cfm->info.name.length = appm_get_dev_name(cfm->info.name.value);

            // Send message
            kernel_msg_send(cfm);
        } break;

        case GAPC_DEV_APPEARANCE:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KERNEL_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                                                    src_id, dest_id,
                                                    gapc_get_dev_info_cfm);
            cfm->req = param->req;
            // Set the device appearance
            #if (BLE_APP_HT)
            // Generic Thermometer - TODO: Use a flag
            cfm->info.appearance = 728;
            #elif (BLE_APP_HID)
            // HID keyboard
            cfm->info.appearance = 961;
            #else
            // No appearance
            cfm->info.appearance = 0;
            #endif
			
            // Send message
            kernel_msg_send(cfm);
        } break;

        case GAPC_DEV_SLV_PREF_PARAMS:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KERNEL_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                    								src_id, dest_id,
                                                    gapc_get_dev_info_cfm);
            cfm->req = param->req;
            // Slave preferred Connection interval Min
            cfm->info.slv_params.con_intv_min = 8;
            // Slave preferred Connection interval Max
            cfm->info.slv_params.con_intv_max = 10;
            // Slave preferred Connection latency
			if (BLE_ROLE_MASTER == ble_get_role_mode())
			{
                cfm->info.slv_params.slave_latency = 0;
			}
			else
			{
                cfm->info.slv_params.slave_latency = 180;
			}
            // Slave preferred Link supervision timeout
            cfm->info.slv_params.conn_timeout  = 600;  // 6s (600*10ms)

            // Send message
            kernel_msg_send(cfm);
        } break;

        default: /* Do Nothing */
			break;
    }


    return (KERNEL_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles GAPC_SET_DEV_INFO_REQ_IND message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_set_dev_info_req_ind_handler(kernel_msg_id_t const msgid,
        struct gapc_set_dev_info_req_ind const *param,
        kernel_task_id_t const dest_id,
        kernel_task_id_t const src_id)
{
	// Set Device configuration
	struct gapc_set_dev_info_cfm* cfm = KERNEL_MSG_ALLOC(GAPC_SET_DEV_INFO_CFM, src_id, dest_id,
                                                 gapc_set_dev_info_cfm);
	// Reject to change parameters
	cfm->status = GAP_ERR_REJECTED;
	cfm->req = param->req;
	// Send message
	kernel_msg_send(cfm);

	return (KERNEL_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_connection_req_ind_handler(kernel_msg_id_t const msgid,
                                           struct gapc_connection_req_ind const *param,
                                           kernel_task_id_t const dest_id,
                                            kernel_task_id_t const src_id)
{	
	bk_printf("%s\r\n", __func__);
	adv_timeout_flag = false;
	
    app_env.conidx = KERNEL_IDX_GET(src_id);
    // Check if the received Connection Handle was valid
    if (app_env.conidx != GAP_INVALID_CONIDX)
    {
        // Retrieve the connection info from the parameters
        app_env.conhdl = param->conhdl;

#if (BLE_APP_CLIENT)					
        if (BLE_ROLE_MASTER == ble_get_role_mode())
        {
            if (!sdp_need_dis_flag)
            {
                kernel_state_set(dest_id, APPM_SDP_DISCOVERING);
                sdp_discover_all_service();
            }
            else
            {
                kernel_state_set(dest_id, APPM_CONNECTED);

                sdp_prf_register_all_atthdl2gatt();

                sdp_enable_all_server_ntf_ind(1);
            }
        }
        else
#endif
        {
            if (kernel_timer_active(APP_ADV_TIMEOUT_TIMER, TASK_APP))
            {
                kernel_timer_clear(APP_ADV_TIMEOUT_TIMER, TASK_APP);
            }

#if 1
            // Send connection confirmation
            struct gapc_connection_cfm *cfm = KERNEL_MSG_ALLOC(GAPC_CONNECTION_CFM,
                    KERNEL_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                    gapc_connection_cfm);

            #if(BLE_APP_SEC)
            cfm->auth = app_sec_get_bond_status() ? GAP_AUTH_REQ_NO_MITM_BOND : GAP_AUTH_REQ_NO_MITM_NO_BOND; 
            #else // !(BLE_APP_SEC)
            cfm->auth = GAP_AUTH_REQ_NO_MITM_NO_BOND;
            #endif // (BLE_APP_SEC)
            // Send the message
            kernel_msg_send(cfm);
#endif
            #if DISPLAY_SUPPORT
            // Update displayed information
            app_display_set_adv(false);
            app_display_set_con(true);
            #endif //(DISPLAY_SUPPORT)

            /*--------------------------------------------------------------
             * ENABLE REQUIRED PROFILES
             *--------------------------------------------------------------*/
            #if (BLE_APP_BATT)
            // Enable Battery Service
            app_batt_enable_prf(app_env.conhdl);
            #endif //(BLE_APP_BATT)
            #if (BLE_APP_HID)
            // Enable HID Service
            app_hid_enable_prf(app_env.conhdl);
            #endif //(BLE_APP_HID)
    				
    		//		UART_PRINTF("peer_addr_type = 0x%x\r\n",param->peer_addr_type);
    			//	UART_PRINTF("peer_addr = ");
    			//	for(uint8_t i = 0; i < sizeof(bd_addr_t);i ++)
    			//	{
    			//		UART_PRINTF("%02x",param->peer_addr.addr[i]);
    			//	}
    			//	UART_PRINTF("\r\n");
    				
            // We are now in connected State
            kernel_state_set(dest_id, APPM_CONNECTED);


    		#if (BLE_APP_SEC)
    		//if device not bond with local, send bond request
    		if(!app_sec_get_bond_status())
    		{
    			//UART_PRINTF("app_sec_get_bond_status = 0x0\r\n");
    			//kernel_timer_set(APP_SEND_SECURITY_REQ,TASK_APP,80);  // 80
    		}
    		else
    		{
    		;//	UART_PRINTF("app_sec_get_bond_status = 0x1\r\n");
    		}
    		
    		#endif
    		
    		#if UPDATE_CONNENCT_PARAM
    		//kernel_timer_set(APP_PARAM_UPDATE_REQ_IND,TASK_APP,100); 
    		#endif	

    		//test timer
    		//kernel_timer_set(APP_PERIOD_TEST_TIMER, TASK_APP, 2000); 
    		
    		//set short connect
    //		if(app_get_mode().target_mode == CONN_MODE_SHORT)
    //		{
    //			kernel_timer_set(APP_DISCONNECT_TIMER, TASK_APP,app_get_mode().target_timeout);
    //		}
        } 				

        if(ble_event_cb)
        {
            ble_event_cb(BLE_CONNECT, (void *)param);
        }

    //    ble_update_connection();
		
    }
    // jiangji master continue scan
    else if (BLE_ROLE_SLAVE == ble_get_role_mode())
    {
#if (BLE_APP_SEC)
        // No connection has been establish, restart advertising
        if(app_sec_get_bond_status())
        {
            //if device has bonded, then start direct adv
            appm_start_direct_dvertising();
        }
        else
#endif
        {
            //device not bonded, start general adv
            appm_start_advertising();	
        }
    }
    else
    {
		bk_printf("connect fail\r\n");
    }
		
		

    return (KERNEL_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP controller command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_cmp_evt_handler(kernel_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                kernel_task_id_t const dest_id,
                                kernel_task_id_t const src_id)
{
	bk_printf("gapc_cmp_evt_handler operation = 0x%x, status = 0x%x\r\n",param->operation, param->status);
	switch(param->operation)
	{
    	case (GAPC_UPDATE_PARAMS):
    	{
			if (param->status != GAP_ERR_NO_ERROR)
        	{
            	//appm_disconnect();
			}
    	} break;
    	case (GAPC_BOND): // 0xa
		{
			if (param->status != GAP_ERR_NO_ERROR)
			{
			;//UART_PRINTF("GAPC_BOND fail !\r\n");
			}
			else if (BLE_ROLE_SLAVE == ble_get_role_mode())
			{
			//UART_PRINTF("GAPC_BOND ok !\r\n");
				#if (BLE_APP_ANCSC)
						// Enable ANCS Service
				app_ancsc_enable_prf(app_env.conhdl);
				#endif //(BLE_APP_ANCSC)
			}
		}break;
    	case (GAPC_ENCRYPT): // 0xb
		{
			if (param->status != GAP_ERR_NO_ERROR)
			{
				;//UART_PRINTF("GAPC_ENCRYPT Start fail !\r\n");
			}
			else if (BLE_ROLE_SLAVE == ble_get_role_mode())
			{
				#if (BLE_APP_ANCSC)
						// Enable ANCS Service
				app_ancsc_enable_prf(app_env.conhdl);
				#endif //(BLE_APP_ANCSC)
			}
		}break;
    	default:
    	  break;
    }

    return (KERNEL_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disconnection complete event from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(kernel_msg_id_t const msgid,
                                      struct gapc_disconnect_ind const *param,
                                      kernel_task_id_t const dest_id,
                                      kernel_task_id_t const src_id)
{

    bk_printf("disconnect link reason = 0x%x, conhdl:0x%x\r\n",param->reason, param->conhdl);
	if (BLE_ROLE_SLAVE == ble_get_role_mode())
	{
#if UPDATE_CONNENCT_PARAM
        if(kernel_timer_active(APP_PARAM_UPDATE_REQ_IND, TASK_APP))
        {
            kernel_timer_clear(APP_PARAM_UPDATE_REQ_IND, TASK_APP);
        }
#endif
        //test
        if(kernel_timer_active(APP_PERIOD_TEST_TIMER, TASK_APP))
        {
            kernel_timer_clear(APP_PERIOD_TEST_TIMER,TASK_APP);
        }
	}

    // Go to the ready state
    kernel_state_set(TASK_APP, APPM_READY);

#if (BLE_APP_HT)
    // Stop interval timer
    app_stop_timer();
#endif 

#if (DISPLAY_SUPPORT)
    // Update Connection State screen
    app_display_set_con(false);
#endif 

#if (BLE_APP_SEC)
    app_sec_init();
#endif

#if (BLE_APP_HID)
    app_hid_init();
#endif

    if (BLE_ROLE_SLAVE == ble_get_role_mode())
    {
        ble_set_role_mode(BLE_ROLE_NONE);
        adv_timeout_flag = false;
        //device not bonded, start general adv
        //appm_start_advertising();
    }
    else if(BLE_ROLE_MASTER == ble_get_role_mode())
    {
        ble_set_role_mode(BLE_ROLE_NONE);
    }

    if(ble_event_cb)
    {
        ble_event_cb(BLE_DISCONNECT, (void*)(&(param->reason)));
    }
	
    return (KERNEL_MSG_CONSUMED);
}


/*******************************************************************************
 * Function: app_period_timer_handler
 * Description: app period timer process
 * Input: msgid -Id of the message received.
 *		  param -Pointer to the parameters of the message.
 *		  dest_id -ID of the receiving task instance (TASK_GAP).
 *		  ID of the sending task instance.
 * Return: If the message was consumed or not.
 * Others: void
*******************************************************************************/
static int app_period_timer_handler(kernel_msg_id_t const msgid,
                                          void const *param,
                                          kernel_task_id_t const dest_id,
                                          kernel_task_id_t const src_id)
{
	kernel_timer_set(APP_PERIOD_TIMER,TASK_APP,20);
	
	//gpio_triger(0x15);


    return KERNEL_MSG_CONSUMED;
	
}



static int app_period_test_handler(kernel_msg_id_t const msgid,
                                          void const *param,
                                          kernel_task_id_t const dest_id,
                                          kernel_task_id_t const src_id)
{
#if 0
	uint8_t buf[8];
	memset(buf, 0x0, 8);
	
	switch(send_count)
	{
		case 0:
			buf[2] = 0x4f;
			send_count++;
			app_hid_send_report(buf, 8);
			printf_val(buf);
			kernel_timer_set(APP_PERIOD_TEST_TIMER,TASK_APP,10);
			break;
		case 1:
			buf[2] = 0x50;
			send_count++;
			app_hid_send_report(buf, 8);
			printf_val(buf);
			kernel_timer_set(APP_PERIOD_TEST_TIMER,TASK_APP,16);
			break;
			
		case 2:
			send_count++;
			buf[2] = 0x4f;
			buf[3] = 0x50;
			app_hid_send_report(buf, 8);
			printf_val(buf);
			buf[3] = 0x0;
			app_hid_send_report(buf, 8);
			printf_val(buf);
			kernel_timer_set(APP_PERIOD_TEST_TIMER,TASK_APP,46);
			break;

		case 3:
			memset(buf, 0x0, 8);
			app_hid_send_report(buf, 8);
			printf_val(buf);
			buf[2] = 0x50;
			app_hid_send_report(buf, 8);
			printf_val(buf);
			send_count++;
			kernel_timer_set(APP_PERIOD_TEST_TIMER,TASK_APP,45);
			break;

		case 4:
			memset(buf, 0x0, 8);
			app_hid_send_report(buf, 8);
			printf_val(buf);
			buf[2] = 0x4f;
			app_hid_send_report(buf, 8);
			printf_val(buf);
			send_count++;
			kernel_timer_set(APP_PERIOD_TEST_TIMER,TASK_APP,130);
			break;

		case 5:
			memset(buf, 0x0, 8);
			app_hid_send_report(buf, 8);
			printf_val(buf);
			buf[2] = 0x50;
			app_hid_send_report(buf, 8);
			printf_val(buf);
			send_count++;
			kernel_timer_set(APP_PERIOD_TEST_TIMER,TASK_APP,3);
			break;

		case 6:
			send_count= 0;
			memset(buf, 0x0, 8);
			app_hid_send_report(buf, 8);
			printf_val(buf);
			kernel_timer_clear(APP_PERIOD_TEST_TIMER, TASK_APP);
			break;
			
		default:
			break;
	}


/*	
	if(send_count)
	{
		buf[2] = 0x4f;
		app_hid_send_report(buf, 8);
	}
	else
	{
		buf[2] = 0x0;
		send_count = true;
		app_hid_send_report(buf, 8);
	}
	kernel_timer_set(APP_PERIOD_TEST_TIMER,TASK_APP,20);
*/
#endif //
	return KERNEL_MSG_CONSUMED;
}


/**
 ****************************************************************************************
 * @brief Handles profile add indication from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_profile_added_ind_handler(kernel_msg_id_t const msgid,
                                          struct gapm_profile_added_ind *param,
                                          kernel_task_id_t const dest_id,
                                          kernel_task_id_t const src_id)
{
    // Current State
    uint8_t state = kernel_state_get(dest_id);

    if (state == APPM_CREATE_DB)
    {
        switch (param->prf_task_id)
        {
            #if defined (BLE_APP_AM0)
            case (TASK_ID_AM0_HAS):
            {
                am0_app_set_prf_task(param->prf_task_nb);
            } break;
            #endif 
            default: 
			break;
        }
    }
    else
    {
        ASSERT_INFO(0, state, src_id);
    }

    return KERNEL_MSG_CONSUMED;
}



/**
 ****************************************************************************************
 * @brief Handles reception of all messages sent from the lower layers to the application
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int appm_msg_handler(kernel_msg_id_t const msgid,
                            void *param,
                            kernel_task_id_t const dest_id,
                            kernel_task_id_t const src_id)
{
    // Retrieve identifier of the task from received message
    kernel_task_id_t src_task_id = BLE_MSG_T(msgid);
    // Message policy
    uint8_t msg_pol  = KERNEL_MSG_CONSUMED;

   switch (src_task_id)
    {
        case (TASK_ID_GAPC):
        {
            #if (BLE_APP_SEC)
            if ((msgid >= GAPC_BOND_CMD) &&
                (msgid <= GAPC_SECURITY_IND))
            {
                // Call the Security Module
                msg_pol = appm_get_handler(&app_sec_table_handler, msgid, param, src_id);
            }
            #endif //(BLE_APP_SEC)
            // else drop the message
        } break;

        case (TASK_ID_GATTC):
        {
            // Service Changed - Drop
        } break;

		#if (BLE_APP_FFF0)
        case (TASK_ID_FFF0S):
        {
            // Call the Health Thermometer Module
            msg_pol = appm_get_handler(&app_fff0_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_FFF0)
				
		#if (BLE_APP_FFE0)
        case (TASK_ID_FFE0S):
        {
            // Call the Health Thermometer Module
            msg_pol = appm_get_handler(&app_ffe0_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_FFE0)

        #if (BLE_APP_COMM)
        case (TASK_ID_COMMON):
        {
            // Call the Health Thermometer Module
            msg_pol = appm_get_handler(&app_comm_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_COMM)

		#if (BLE_APP_ANCSC)
        case (TASK_ID_ANCSC):
        {
            // Call the Health Thermometer Module
            msg_pol = appm_get_handler(&app_ancsc_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_ANCSC)
				
        #if (BLE_APP_HT)
        case (TASK_ID_HTPT):
        {
            // Call the Health Thermometer Module
            msg_pol = appm_get_handler(&app_ht_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HT)

        #if (BLE_APP_DIS)
        case (TASK_ID_DISS):
        {
            // Call the Device Information Module
            msg_pol = appm_get_handler(&app_dis_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_DIS)

	
        #if (BLE_APP_HID)
        case (TASK_ID_HOGPD):
        {
            // Call the HID Module
            msg_pol = appm_get_handler(&app_hid_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HID)

        #if (BLE_APP_BATT)
        case (TASK_ID_BASS):
        {
            // Call the Battery Module
            msg_pol = appm_get_handler(&app_batt_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_BATT)

        #if defined(BLE_APP_AM0)
        case (TASK_ID_AM0):
        {
            // Call the Audio Mode 0 Module
            msg_pol = appm_get_handler(&am0_app_table_handler, msgid, param, src_id);
        } break;
        case (TASK_ID_AM0_HAS):
        {
            // Call the Audio Mode 0 Module
            msg_pol = appm_get_handler(&am0_app_has_table_handler, msgid, param, src_id);
        } break;
        #endif // defined(BLE_APP_AM0)
		
		#if (BLE_APP_OADS)
        case (TASK_ID_OADS):
        {
            // Call the Health Thermometer Module
            msg_pol = appm_get_handler(&app_oads_table_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_OADS)
		#if (BLE_APP_SDP)
		case (TASK_ID_SDP):
		{
			// Call the SDP Module
			msg_pol = appm_get_handler(&sdp_default_handler, msgid, param, src_id);
		}
		break;
		#endif //(TASK_ID_SDP)	

        default:
		if (BLE_ROLE_SLAVE == ble_get_role_mode())
        {
            #if (BLE_APP_HT)
            if (msgid == APP_HT_MEAS_INTV_TIMER)
            {
                msg_pol = appm_get_handler(&app_ht_table_handler, msgid, param, src_id);
            }
            #endif //(BLE_APP_HT)

            #if (BLE_APP_HID)
            if (msgid == APP_HID_MOUSE_TIMEOUT_TIMER)
            {
                msg_pol = appm_get_handler(&app_hid_table_handler, msgid, param, src_id);
            }
            #endif //(BLE_APP_HID)
        } break;
    }

    return (msg_pol);
}


/*******************************************************************************
 * Function: gapc_update_conn_param_req_ind_handler
 * Description: Update request command processing from slaver connection parameters
 * Input: msgid   -Id of the message received.
 *		  param   -Pointer to the parameters of the message.
 *		  dest_id -ID of the receiving task instance
 *		  src_id  -ID of the sending task instance.
 * Return: If the message was consumed or not.
 * Others: void
*******************************************************************************/
static int gapc_update_conn_param_req_ind_handler (kernel_msg_id_t const msgid, 
									const struct gapc_param_update_req_ind  *param,
                 					kernel_task_id_t const dest_id, kernel_task_id_t const src_id)
{
	//UART_PRINTF("slave send param_update_req\r\n");
	struct gapc_conn_param  up_param;

	up_param.intv_min   = BLE_UAPDATA_MIN_INTVALUE;
	up_param.intv_max   = BLE_UAPDATA_MAX_INTVALUE;
	up_param.latency    = BLE_UAPDATA_LATENCY;
	up_param.time_out   = BLE_UAPDATA_TIMEOUT;

	appm_update_param(&up_param);
    
	return KERNEL_MSG_CONSUMED;
}

 
/*******************************************************************************
 * Function: gapc_le_pkt_size_ind_handler
 * Description: GAPC_LE_PKT_SIZE_IND
 * Input: msgid   -Id of the message received.
 *		  param   -Pointer to the parameters of the message.
 *		  dest_id -ID of the receiving task instance
 *		  src_id  -ID of the sending task instance.
 * Return: If the message was consumed or not.
 * Others: void
*******************************************************************************/
static int gapc_le_pkt_size_ind_handler (kernel_msg_id_t const msgid, 
									const struct gapc_le_pkt_size_ind  *param,
                 					kernel_task_id_t const dest_id, kernel_task_id_t const src_id)
{
  // 	UART_PRINTF("%s \r\n", __func__);
	
	//UART_PRINTF("1max_rx_octets = %d\r\n",param->max_rx_octets);
	//UART_PRINTF("1max_rx_time = %d\r\n",param->max_rx_time);
	//UART_PRINTF("1max_tx_octets = %d\r\n",param->max_tx_octets);
	//UART_PRINTF("1max_tx_time = %d\r\n",param->max_tx_time);
	
	return KERNEL_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief  GAPC_PARAM_UPDATED_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_param_updated_ind_handler (kernel_msg_id_t const msgid, 
									const struct gapc_param_updated_ind  *param,
                 					kernel_task_id_t const dest_id, kernel_task_id_t const src_id)
{
   // UART_PRINTF("%s \r\n", __func__);
	
	//UART_PRINTF("con_interval = %d\r\n",param->con_interval);
	//UART_PRINTF("con_latency = %d\r\n",param->con_latency);
//	UART_PRINTF("sup_to = %d\r\n",param->sup_to);
	
	return KERNEL_MSG_CONSUMED;
}




/**
 ****************************************************************************************
 * @brief  GATTC_MTU_CHANGED_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_mtu_changed_ind_handler(kernel_msg_id_t const msgid,
                                     struct gattc_mtu_changed_ind const *ind,
                                     kernel_task_id_t const dest_id,
                                     kernel_task_id_t const src_id)
{
    //UART_PRINTF("%s \r\n",__func__);
    //UART_PRINTF("ind->mtu = %d,seq = %d\r\n",ind->mtu,ind->seq_num);

    if(ble_event_cb)
    {
        ble_event_cb(BLE_MTU_CHANGE, (void*)(&(ind->mtu)));
    }

    return (KERNEL_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief  GATTC_EXC_MTU_CMD
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_mtu_exchange_req_handler(kernel_msg_id_t const msgid,
        struct gattc_exc_mtu_cmd const *req,
        kernel_task_id_t const dest_id,
        kernel_task_id_t const src_id)
{
	bk_printf("gattc_mtu_exchange_req_handler\r\n");
	struct gattc_exc_mtu_cmd *cmd = KERNEL_MSG_ALLOC(GATTC_EXC_MTU_CMD,
	                                KERNEL_BUILD_ID(TASK_GATTC, app_env.conidx),
	                                TASK_APP,gattc_exc_mtu_cmd);
	cmd->operation = GATTC_MTU_EXCH;
	cmd->seq_num = 0;
	kernel_msg_send(cmd);

	return (KERNEL_MSG_CONSUMED);
}


ble_scan_list_t ble_scan_list;
uint8_t adv_count = 0;

static int gapm_adv_report_ind_handler(kernel_msg_id_t const msgid,
                                     struct adv_report const *param,
                                     kernel_task_id_t const dest_id,
                                     kernel_task_id_t const src_id)
{
    //jiangji update
    int i = 0;
#if (BLE_APP_CLIENT)
    if(adv_count < appm_get_max_scan_nums())
    {
        for (i = 0; i < adv_count; i++)
        {
            if(0 == memcmp(&ble_scan_list.info[i].adv_addr.addr[0], param->adv_addr.addr, 6))
            {
                break;
            }
        }

        if(i == adv_count)
        {
            bk_printf("\r\n\r\n##################################\r\n");
            ble_scan_list.info[adv_count].addr_type = param->adv_addr_type;
            ble_scan_list.info[adv_count].adv_type = param->evt_type;
            ble_scan_list.info[adv_count].number = adv_count;
            ble_scan_list.info[adv_count].rssi = param->rssi;
            memcpy(&ble_scan_list.info[adv_count].adv_addr.addr[0],param->adv_addr.addr, 6);
            bk_printf("adv number = %d\r\n", ble_scan_list.info[adv_count].number);
            bk_printf("adv type = %x  :%s\r\n",ble_scan_list.info[adv_count].adv_type, 
                (param->evt_type == ADV_CONN_UNDIR) ? "ADV_CONN_UNDIR" :(param->evt_type == ADV_CONN_DIR)? "ADV_CONN_DIR" : 
                (param->evt_type == ADV_DISC_UNDIR)? "ADV_DISC_UNDIR" : (param->evt_type == ADV_NONCONN_UNDIR)? "ADV_NONCONN_UNDIR": "Unknow");
            bk_printf("adv rssi = %d\r\n", (int8_t)ble_scan_list.info[adv_count].rssi);
            bk_printf("adv addr = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                ble_scan_list.info[adv_count].adv_addr.addr[0],ble_scan_list.info[adv_count].adv_addr.addr[1],ble_scan_list.info[adv_count].adv_addr.addr[2],
                ble_scan_list.info[adv_count].adv_addr.addr[3],ble_scan_list.info[adv_count].adv_addr.addr[4],ble_scan_list.info[adv_count].adv_addr.addr[5]);
            bk_printf("adv addr type = %02x\r\n",ble_scan_list.info[adv_count].addr_type);

            adv_count++;
            bk_printf("\r\n##################################\r\n");
        }
    }
    if (ble_recv_adv_cb)
    {
        
        (*ble_recv_adv_cb)((unsigned char*)param->data, param->data_len);
    }
        
#endif

    return (KERNEL_MSG_CONSUMED);
}

static int gapm_gen_dh_key_ind_handler(kernel_msg_id_t const msgid,
                                            struct gapm_gen_dh_key_ind const *param,
                                            kernel_task_id_t const dest_id,
                                            kernel_task_id_t const src_id)
{
    int msg_status = KERNEL_MSG_CONSUMED;
    struct ble_gen_dh_key_ind ind;

    ind.result =  (uint8_t*)&(param->result[0]);
    ind.len =  GAP_P256_KEY_LEN;

    if(ble_event_cb)
    {
        ble_event_cb(BLE_GEN_DH_KEY, (void *)&ind);
    }

    return (msg_status);
}

static int gapm_get_key_ind_handler(kernel_msg_id_t const msgid,
                                            struct gapm_get_key_ind const *param,
                                            kernel_task_id_t const dest_id,
                                            kernel_task_id_t const src_id)
{
    int msg_status = KERNEL_MSG_CONSUMED;
    struct ble_get_key_ind ind;

    ind.pri_key =  (uint8_t *)&(param->pri_key[0]);
    ind.pri_len =  GAP_P256_KEY_LEN;
    ind.pub_key_x =  (uint8_t *)&(param->pub_key_x[0]);
    ind.pub_x_len =  GAP_P256_KEY_LEN;
    ind.pub_key_y =  (uint8_t *)&(param->pub_key_y[0]);
    ind.pub_y_len =  GAP_P256_KEY_LEN;

    if(ble_event_cb)
    {
        ble_event_cb(BLE_GET_KEY, (void *)&ind);
    }
    
    return (msg_status);
}

/**
 ****************************************************************************************
 * @brief   GAPC_PARAM_UPDATE_REQ_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_param_update_req_ind_handler(kernel_msg_id_t const msgid,
                                struct gapc_param_update_req_ind const *param,
                                kernel_task_id_t const dest_id,
                                kernel_task_id_t const src_id)
{
	//UART_PRINTF("%s \r\n", __func__);
	// Prepare the GAPC_PARAM_UPDATE_CFM message
    struct gapc_param_update_cfm *cfm = KERNEL_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM,
                                             src_id, dest_id,
                                             gapc_param_update_cfm);
	 
	//cfm->accept = false;
	cfm->ce_len_max = 0xffff;
	cfm->ce_len_min = 0xffff;
	cfm->accept = true; 

	// Send message
    kernel_msg_send(cfm);
	 
	return (KERNEL_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief   app_adv_enable_handler
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_adv_enable_handler(kernel_msg_id_t const msgid,
                         		void const *param,
                         		kernel_task_id_t const dest_id,
                         		kernel_task_id_t const src_id)	
{
	if(kernel_state_get(TASK_APP) == APPM_CONNECTED)
	{
		return KERNEL_MSG_CONSUMED;
	}
	//UART_PRINTF("%s\r\n", __func__);
	
	// Restart Advertising

    {
        //device not bonded, start general adv
        appm_start_advertising();	
    }
	
	return (KERNEL_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief   app_disconnect_timer_handler
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_disconnect_timer_handler(kernel_msg_id_t const msgid,
                         void const *param,
                         kernel_task_id_t const dest_id,
                         kernel_task_id_t const src_id)
{
	//UART_PRINTF("%s\r\n", __func__);
	
	if(kernel_state_get(TASK_APP) == APPM_CONNECTED)
	{
		appm_disconnect(0x13);
	}
	
	return (KERNEL_MSG_CONSUMED);
}






/**
 ****************************************************************************************
 * @brief   APP_ENCODE_IND
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_sbc_encode_handler(kernel_msg_id_t const msgid,
                                     struct sbc_encode const *param,
                                     kernel_task_id_t const dest_id,
                                     kernel_task_id_t const src_id)																			 
{ 

		/*
		sbc_env.stream[3] = app_hid_env.encode_cnt++; //for test
		
		//Store the data to be encoded into the loop buffer
		store_encode_data(&sbc_env.stream[3]);
		*/



	return (KERNEL_MSG_CONSUMED);
}

#if (BLE_APP_SEC)
/**
 ****************************************************************************************
 * @brief  APP_SEND_SECURITY_REQ
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_send_security_req_handler( const kernel_msg_id_t  msgid, const void  *param,
		const kernel_task_id_t  dest_id, const kernel_task_id_t  src_id)
{
    appm_send_seurity_req();

    return KERNEL_MSG_CONSUMED;
}
#endif

#if (BLE_APP_CLIENT)
static int gattc_cmp_evt_handler(kernel_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 kernel_task_id_t const dest_id,
                                 kernel_task_id_t const src_id)
{
	bk_printf("found %s operation = 0x%x,status = 0x%x,param->seq_num = 0x%x\r\n",__func__,param->operation,param->status,param->seq_num);
	if(param->operation == GATTC_SDP_DISC_SVC_ALL)
	{
		if(param->status == ATT_ERR_NO_ERROR)
		{
			kernel_state_set(dest_id, APPM_CONNECTED);
			sdp_need_dis_flag = 1;
		}
	}
	if((param->operation == GATTC_WRITE) && (param->seq_num == 0xa5) )
	{
		uint8_t find;
		find = sdp_enable_all_server_ntf_ind(0);
		if(sdp_need_dis_flag == 1)
		{
			kernel_state_set(dest_id, APPM_CONNECTED);
		}
		bk_printf("find = %d\r\n",find);
		if(find == 0)
		{
			//set_flash_clk(0x8);
		}
	}
	return (KERNEL_MSG_CONSUMED);
}



static int gattc_sdp_svc_ind_handler(kernel_msg_id_t const msgid,
                                     struct gattc_sdp_svc_ind const *ind,
                                     kernel_task_id_t const dest_id,
                                     kernel_task_id_t const src_id)
{
	sdp_extract_svc_info(ind);

	return (KERNEL_MSG_CONSUMED);
}
#endif

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/* Default State handlers definition. */
const struct kernel_msg_handler appm_default_state[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KERNEL_MSG_DEFAULT_HANDLER,    	(kernel_msg_func_t)appm_msg_handler},
    {APP_ADV_TIMEOUT_TIMER,     	(kernel_msg_func_t)app_adv_timeout_handler},
    {GAPM_DEVICE_READY_IND,     	(kernel_msg_func_t)gapm_device_ready_ind_handler},
    {GAPM_CMP_EVT,             		(kernel_msg_func_t)gapm_cmp_evt_handler},
    {GAPC_GET_DEV_INFO_REQ_IND, 	(kernel_msg_func_t)gapc_get_dev_info_req_ind_handler},
    {GAPC_SET_DEV_INFO_REQ_IND, 	(kernel_msg_func_t)gapc_set_dev_info_req_ind_handler},
    {GAPC_CONNECTION_REQ_IND,   	(kernel_msg_func_t)gapc_connection_req_ind_handler},
    {GAPC_CMP_EVT,             		(kernel_msg_func_t)gapc_cmp_evt_handler},
    {GAPC_DISCONNECT_IND,       	(kernel_msg_func_t)gapc_disconnect_ind_handler},
    {GAPM_PROFILE_ADDED_IND,    	(kernel_msg_func_t)gapm_profile_added_ind_handler},
    {GAPC_LE_PKT_SIZE_IND,			(kernel_msg_func_t)gapc_le_pkt_size_ind_handler},
    {GAPC_PARAM_UPDATED_IND,		(kernel_msg_func_t)gapc_param_updated_ind_handler},
    {GATTC_MTU_CHANGED_IND,			(kernel_msg_func_t)gattc_mtu_changed_ind_handler},	
    {GAPM_ADV_REPORT_IND,           (kernel_msg_func_t)gapm_adv_report_ind_handler},
#if (SECURE_CONNECTIONS)
    {GAPM_GEN_DH_KEY_IND,           (kernel_msg_func_t) gapm_gen_dh_key_ind_handler},
    {GAPM_GET_KEY_IND,              (kernel_msg_func_t) gapm_get_key_ind_handler},
#endif
    {GAPC_PARAM_UPDATE_REQ_IND, 	(kernel_msg_func_t)gapc_param_update_req_ind_handler},
#if (BLE_APP_CLIENT)
	{GATTC_CMP_EVT,             	(kernel_msg_func_t)gattc_cmp_evt_handler},
    {GATTC_SDP_SVC_IND,         	(kernel_msg_func_t)gattc_sdp_svc_ind_handler},
#endif
    {APP_PARAM_UPDATE_REQ_IND, 		(kernel_msg_func_t)gapc_update_conn_param_req_ind_handler},
    {APP_ADV_ENABLE_TIMER,			(kernel_msg_func_t)app_adv_enable_handler},
    {APP_PERIOD_TIMER,				(kernel_msg_func_t)app_period_timer_handler},
	{APP_GATTC_EXC_MTU_CMD,	        (kernel_msg_func_t)gattc_mtu_exchange_req_handler},
#if (BLE_APP_SEC)
    {APP_SEND_SECURITY_REQ,     	(kernel_msg_func_t)gapc_send_security_req_handler},
#endif
    {APP_ENCODE_IND,				(kernel_msg_func_t)app_sbc_encode_handler},
    {APP_DISCONNECT_TIMER,      	(kernel_msg_func_t)app_disconnect_timer_handler},

   	{APP_PERIOD_TEST_TIMER,         (kernel_msg_func_t)app_period_test_handler},
};

/* Specifies the message handlers that are common to all states. */
const struct kernel_state_handler appm_default_handler = KERNEL_STATE_HANDLER(appm_default_state);

/* Defines the place holder for the states of all the task instances. */
kernel_state_t appm_state[APP_IDX_MAX];

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
