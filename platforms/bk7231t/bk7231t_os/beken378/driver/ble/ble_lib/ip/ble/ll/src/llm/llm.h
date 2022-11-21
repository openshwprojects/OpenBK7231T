/**
 ****************************************************************************************
 *
 * @file llm.h
 *
 * @brief Main API file for the Link Layer manager
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 ****************************************************************************************
 */

#ifndef LLM_H_
#define LLM_H_

/**
 ****************************************************************************************
 * @addtogroup CONTROLLER
 * @ingroup ROOT
 * @brief BLE Lower Layers
 *
 * The CONTROLLER contains the modules allowing the physical link establishment,
 * maintenance and management.
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup LLM LLM
 * @ingroup CONTROLLER
 * @brief Link Layer Manager
 *
 * The link layer manager contains the modules allowing the physical link establishment,
 * and all the non-connected states.
 * @{
 ****************************************************************************************
 */




/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "common_bt.h"
#include "common_math.h"
#include "common_utils.h"
#include "em_buf.h"
#include "llm_task.h"
#include "rwip.h"
#include "ea.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/// Maximum authorized RPA Timeout
#define LLM_RPA_TIMEOUT_MAX                 (0xA1B8)
/// Default RPA Timeout
#define LLM_RPA_TIMEOUT_DEFAULT             (0x0384)

/// LLM_P256_BYTE_TIMOUT
#define LLM_P256_BYTE_TIMOUT                (0x0010)


/// Enhanced Privacy 1.2 info
///   7  6  5  4  3  2  1  0
/// +--+--+--+--+--+--+--+--+
/// |      RFU        |TO|EN|
/// +--+--+--+--+--+--+--+--+
enum llm_enh_priv
{
    /// - [7:1]: RFU
    LLM_PRIV_RFU_MASK     = 0x8C,
    LLM_PRIV_RFU_LSB      = 2,

    /// - [1]:    Resolving Private Address Renewal timer started
    LLM_RPA_RENEW_TIMER_EN_MASK  = 0x02,
    LLM_RPA_RENEW_TIMER_EN_LSB   = 1,

    /// - [0]:    Enable/Disable Privacy Flag
    LLM_PRIV_ENABLE_MASK  = 0x01,
    LLM_PRIV_ENABLE_LSB   = 0,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
/// Advertising parameters
struct advertising_pdu_params
{
    /// Pointer on the data adv request
    struct kernel_msg *adv_data_req;
    /// Pointer on the scan response request
    struct kernel_msg *scan_rsp_req;
    /// Pointer of the ADV_X_IND descriptor
    struct em_buf_node *adv_desc_node;
    /// Pointer of the SCAN_RSP descriptor
    struct em_buf_node *scan_rsp_desc_node;
    /// Peer address
    struct bd_addr peer_addr;
    /// Connection interval min
    uint16_t intervalmin;
    /// Connection interval max
    uint16_t intervalmax;
    /// Channel mapping
    uint8_t channelmap;
    /// Filtering policy
    uint8_t filterpolicy;
    /// Advertising type
    uint8_t type;
    /// Data length
    uint8_t datalen;
    /// Scan RSP length
    uint8_t scanrsplen;
    /// Local address type
    uint8_t own_addr_type;
    /// Peer address type
    uint8_t peer_addr_type;
    /// Advertising frequency: true for low duty cycle, false for high duty cycle
    bool adv_ldc_flag;
};

///Scanning parameters
struct scanning_pdu_params
{
    /// Pointer of the CONNECT_REQ and SCAN_REQ descriptor
    struct em_buf_node *conn_req_desc_node;
    /// Scan interval
    uint16_t interval;
    /// Scan window
    uint16_t window;
    /// Filtering policy
    uint8_t filterpolicy;
    /// Scanning type
    uint8_t type;
    /// Duplicate the advertising report
    uint8_t filter_duplicate;
    /// Local address type
    uint8_t own_addr_type;

    ///Scanning channel map, add by hengzhang for fixed channel scan
    uint8_t              channel_map;
};

///Access address generation structure
struct access_addr_gen
{
    /// random
    uint8_t intrand;
    /// index 1
    uint8_t ct1_idx;
    /// index 2
    uint8_t ct2_idx;
};

/// Advertising report list
struct adv_device_list
{
    /// Header
    struct common_list_hdr hdr;
    /// Advertising type
    uint8_t adv_type;
    /// Advertising device address
    struct bd_addr adv_addr;
};

//advertising pdu
///structure adv undirected
struct llm_pdu_adv
{
    /// advertising address
    struct bd_addr  adva;
    /// advertising data
    uint8_t         *adva_data;
};
///structure adv directed
struct llm_pdu_adv_directed
{
    /// advertising address
    struct bd_addr  adva;
    /// initiator address
    struct bd_addr  inita;
};

//scanning pdu
///structure scan request
struct llm_pdu_scan_req
{
    /// scanning address
    struct bd_addr  scana;
    /// advertising address
    struct bd_addr  adva;
};
///structure scan response
struct llm_pdu_scan_rsp
{
    /// advertising address
    struct bd_addr  adva;
    /// scan response data
    uint8_t         *scan_data;

};
///initiating pdu
///structure connection request reception
struct llm_pdu_con_req_rx
{
    /// initiator address
    struct bd_addr      inita;
    /// advertiser address
    struct bd_addr      adva;
    /// access address
    struct access_addr  aa;
    /// CRC init
    struct crc_init     crcinit;
    /// window size
    uint8_t             winsize;
    /// window offset
    uint16_t            winoffset;
    /// interval
    uint16_t            interval;
    /// latency
    uint16_t            latency;
    /// timeout
    uint16_t            timeout;
    /// channel mapping
    struct le_chnl_map  chm;
    /// hopping
    uint8_t             hop_sca;
};
///structure connection request transmission
struct llm_pdu_con_req_tx
{
    /// access address
    struct access_addr  aa;
    /// CRC init
    struct crc_init     crcinit;
    /// window size
    uint8_t             winsize;
    /// window offset
    uint16_t            winoffset;
    /// interval
    uint16_t            interval;
    /// latency
    uint16_t            latency;
    /// timeout
    uint16_t            timeout;
    /// channel mapping
    struct le_chnl_map  chm;
    /// hopping
    uint8_t             hop_sca;
};

///structure for the test mode
struct llm_test_mode
{
    /// flag indicating the end of test
    bool end_of_tst;
    /// Direct test type
    uint8_t  directtesttype;
};

#if (BLE_CENTRAL || BLE_PERIPHERAL || BLE_MESH)
#if (SECURE_CONNECTIONS)
typedef enum t_key_multiplication_type
{
    LLM_ECC_IDLE                  = 0,
    LLM_PUBLIC_KEY_GENERATION     = 1,
    LLM_DHKEY_GENERATION          = 2
} t_key_multi_type;

typedef struct t_public_key256
{
    uint8_t x[PUBLIC_KEY_P256_LEN];
    uint8_t y[PUBLIC_KEY_P256_LEN];
} t_public_key256;
#endif // (SECURE_CONNECTIONS)
#endif //(BLE_CENTRAL || BLE_PERIPHERAL || BLE_MESH)

#if (BLE_CENTRAL || BLE_PERIPHERAL)
///structure for the data length extension
struct data_len_ext
{
    ///the value of connMaxTxOctets that the Controller will use for a new connection.
    uint16_t conn_initial_max_tx_octets;
    ///the value of connMaxTxTime that the Controller will use for a new connection.
    uint16_t conn_initial_max_tx_time;
    ///the maximum value of connMaxTxOctets that the Controller supports.
    uint16_t suppted_max_tx_octets;
    ///the maximum value of connMaxTxTime that the Controller supports.
    uint16_t suppted_max_tx_time;
    ///the maximum value of connMaxRxOctets that the Controller supports.
    uint16_t suppted_max_rx_octets;
    ///the maximum value of connMaxRxTime that the Controller supports.
    uint16_t suppted_max_rx_time;
};
#if (BLE_2MBPS)
struct phys_tag
{
    ///the preference among the PHYs that the Controller supports in a given direction.
    uint8_t rate_preference;
    ///the value of the transmitter PHYs that the Host prefers the Controller to use.
    uint8_t conn_initial_rate_tx;
    ///the value of the receiver PHYs that the Host prefers the Controller to use.
    uint8_t conn_initial_rate_rx;
};
#endif // (BLE_2MPBS)

///Structure for the channel map assessment
struct channel_map_assess
{
    /// Assessment timer (second)
    uint16_t assess_timer;
    /// Lower limit of the receiver quality casted
    int8_t  lower_limit;
    /// Upper limit of the receiver quality casted
    int8_t  upper_limit;
    /// Noise RSSI
    int8_t  rssi_noise_limit;
    /// Re-assessment counter (nb of assessment timer expiration)
    uint8_t reassess_count;
    ///current channel map
    struct le_chnl_map ch_map;
    ///protection for the command
    bool llm_le_set_host_ch_class_cmd_sto;
};
#endif

/// LLM environment structure to be saved
struct llm_le_env_tag
{
    /// List of encryption requests
    struct common_list enc_req;

#if (BLE_CENTRAL || BLE_OBSERVER)
    /// Advertising reports filter policy
    struct common_list adv_list;

    /// Scanning parameters
    struct scanning_pdu_params *scanning_params;
#endif //(BLE_CENTRAL || BLE_OBSERVER)

#if (BLE_BROADCASTER || BLE_PERIPHERAL)
    /// Advertising parameters
    struct advertising_pdu_params *advertising_params;
#endif //(BLE_BROADCASTER || BLE_PERIPHERAL)

#if (BLE_CENTRAL || BLE_PERIPHERAL)
    /// Connected bd address list
    struct common_list cnx_list;

    /// Data length extension values
    struct data_len_ext  data_len_val;

    /// Channel Assessment structure environment
    struct channel_map_assess ch_map_assess;

#if (BLE_2MBPS)
    /// 2MPBS PHY  values
    struct phys_tag  phys_val;
#endif // (BLE_2MBPS)
#endif //(BLE_CENTRAL || BLE_PERIPHERAL)

    /// Event mask
    struct evt_mask eventmask;

    /// Access address
    struct access_addr_gen aa;

    /// conhdl_allocated
    uint16_t conhdl_alloc;

    /// Element
    struct ea_elt_tag *elt;

    ///encryption pending
    bool enc_pend;

    ///test mode
    struct llm_test_mode test_mode;

    /// random bd_address
    struct bd_addr rand_add;

    /// public bd_address
    struct bd_addr public_add;

    /// Resolvable private address renew timeout
    uint16_t  enh_priv_rpa_timeout;

#if (BLE_CENTRAL || BLE_PERIPHERAL || BLE_MESH)
#if (SECURE_CONNECTIONS)
    /// The time between the processing of each byte in a P-256 calculation
    uint16_t  p256_byte_process_timeout;
#endif // (SECURE_CONNECTIONS)
#endif // (BLE_CENTRAL || BLE_PERIPHERAL || BLE_MESH)

    /// Stop command deferred opcode and state
    uint16_t opcode;
    uint8_t  state;

    /// Enhanced Privacy 1.2 info (@see enum llm_enh_priv)
    /// - [7:1]: RFU
    /// - [0]:    Enable/Disable Privacy Flag
    uint8_t  enh_priv_info;

    /// current @type in the register
    uint8_t curr_addr_type;

    /// number of devices in WL
    uint8_t nb_dev_in_wl;
    /// number of devices in WL written in HW (not in Black List)
    uint8_t nb_dev_in_hw_wl;

#if (BLE_CENTRAL || BLE_PERIPHERAL || BLE_MESH)
#if (SECURE_CONNECTIONS)
    t_public_key256  public_key256;
    uint8_t          secret_key256[32];
    t_key_multi_type cur_ecc_multiplication;
#endif // (SECURE_CONNECTIONS)
#endif // (BLE_CENTRAL || BLE_PERIPHERAL || BLE_MESH)

    ///Boolean to avoid having buffer full (non encrypted link) and SW encrypt at the same time which lead an error
    bool enc_postponed;

};

extern const struct supp_cmds llm_local_cmds;
extern const struct le_features llm_local_le_feats;
extern const struct le_states llm_local_le_states;
#if (BLE_CENTRAL || BLE_PERIPHERAL)
extern const struct data_len_ext llm_local_data_len_values;
#endif

/*
 * DEFINES
 ****************************************************************************************
 */
/// Advertising channel TX power
#define LLM_ADV_CHANNEL_TXPWR                   rwip_rf.txpwr_max

/// Advertising set parameters range min
#define LLM_ADV_INTERVAL_MIN                    32//(0x20)
/// Advertising set parameters range max
#define LLM_ADV_INTERVAL_MAX                    16384//(0x4000)

/// Scanning set parameters range min
#define LLM_SCAN_INTERVAL_MIN                    4//(0x4)
/// Scanning set parameters range max
#define LLM_SCAN_INTERVAL_MAX                    16384//(0x4000)

/// Scanning set parameters range min
#define LLM_SCAN_WINDOW_MIN                    4//(0x4)
/// Scanning set parameters range max
#define LLM_SCAN_WINDOW_MAX                    16384//(0x4000)


/// In case where the adv_type is set to non connectable or discoverable
#define LLM_ADV_INTERVAL_MIN_NONCON_DISC        160 //(0xA0)

/// Time out value for the advertising direct event
#define LLM_LE_ADV_TO_DIRECTED                  1024  //10,24 s -> 1024 ticks(10ms)

/// Default value for the number of advertising report
#define LLM_LE_ADV_REPORT_DFT                   0x1

/// Frequency max for the receiver test mode
#define RX_TEST_FREQ_MAX                        39

/// Size maximum for the receiver test mode
#define RX_TEST_SIZE_MAX                        255

/// Number max of good channel
#define LE_NB_CH_MAP_MAX                        37

/// default irq interrupt threshold
#define RX_THR_DFT                              1

/// Index dedicated for the advertising pdu
enum
{
    LLM_LE_ADV_DUMMY_IDX = (BLE_TX_DESC_CNTL - 1),
#if (BLE_OBSERVER || BLE_PERIPHERAL || BLE_CENTRAL)
    LLM_LE_SCAN_CON_REQ_ADV_DIR_IDX,
#endif // BLE_OBSERVER || BLE_PERIPHERAL || BLE_CENTRAL
#if (BLE_BROADCASTER || BLE_PERIPHERAL)
    LLM_LE_SCAN_RSP_IDX,
#endif // BLE_BROADCASTER || BLE_PERIPHERAL
    LLM_LE_ADV_IDX
};


/// Advertising Access Address
#define LLM_LE_ADV_AA    0x8E89BED6

/// Scanning default interval (10ms)
#define LLM_LE_SCAN_INTERV_DFLT       16 //(0X10)

/// Advertising default interval (1,28s)
#define LLM_LE_ADV_INTERV_DFLT        2048 //(0X800)

/// Advertising default channel map (ch37, ch38, ch39)
#define LLM_LE_ADV_CH_MAP_DFLT        0X7

#if !(BLE_QUALIF)
/// Features byte 0
/*
#define BLE_FEATURES_BYTE0  ( BLE_ENC_FEATURE | BLE_CON_PARAM_REQ_PROC_FEATURE | BLE_REJ_IND_EXT_FEATURE | BLE_SLAVE_INIT_EXCHG_FEATURE | BLE_PING_FEATURE\
                            | BLE_LENGTH_EXT_FEATURE | BLE_LL_PRIVACY_FEATURE | BLE_EXT_SCAN_POLICY_FEATURE)
*/                            
#define BLE_FEATURES_BYTE0  ( BLE_ENC_FEATURE | BLE_CON_PARAM_REQ_PROC_FEATURE | BLE_REJ_IND_EXT_FEATURE | BLE_SLAVE_INIT_EXCHG_FEATURE | BLE_PING_FEATURE\
                            | BLE_LENGTH_EXT_FEATURE | BLE_LL_PRIVACY_FEATURE | BLE_EXT_SCAN_POLICY_FEATURE)
/// Features byte 1
#if (BLE_2MBPS)
#define BLE_FEATURES_BYTE1  ( BLE_2MBPS_FEATURE | BLE_STABLE_MOD_FEATURE)
#else // !(BLE_2MBPS)
/// Features byte 1
#define BLE_FEATURES_BYTE1  0x00
#endif //(BLE_2MBPS)
#else
/// Features byte 0
#define BLE_FEATURES_BYTE0  ( BLE_ENC_FEATURE)
/// Features byte 1
#define BLE_FEATURES_BYTE1  0x00
#endif

/// Features byte 2
#define BLE_FEATURES_BYTE2  0x00
/// Features byte 3
#define BLE_FEATURES_BYTE3  0x00
/// Features byte 4
#define BLE_FEATURES_BYTE4  0x00
/// Features byte 5
#define BLE_FEATURES_BYTE5  0x00
/// Features byte 6
#define BLE_FEATURES_BYTE6  0x00
/// Features byte 7
#define BLE_FEATURES_BYTE7  0x00

/// States byte 0
#define BLE_STATES_BYTE0    ( BLE_NON_CON_ADV_STATE | BLE_DISC_ADV_STATE\
                            | BLE_CON_ADV_STATE | BLE_HDC_DIRECT_ADV_STATE\
                            | BLE_PASS_SCAN_STATE | BLE_ACTIV_SCAN_STATE\
                            | BLE_INIT_MASTER_STATE | BLE_CON_SLAVE_STATE)
#if !(BLE_QUALIF)
/// States byte 1
#define BLE_STATES_BYTE1    ( BLE_NON_CON_ADV_PASS_SCAN_STATE | BLE_DISC_ADV_PASS_SCAN_STATE\
                            | BLE_CON_ADV_PASS_SCAN_STATE | BLE_HDC_DIRECT_ADV_PASS_SCAN_STATE\
                            | BLE_NON_CON_ADV_ACTIV_SCAN_STATE | BLE_DISC_ADV_ACTIV_SCAN_STATE\
                            | BLE_CON_ADV_ACTIV_SCAN_STATE | BLE_HDC_DIRECT_ADV_ACTIV_SCAN_STATE)

/// States byte 2
#define BLE_STATES_BYTE2    ( BLE_NON_CON_ADV_INIT_STATE | BLE_DISC_ADV_INIT_STATE\
                            | BLE_NON_CON_ADV_MASTER_STATE | BLE_DISC_ADV_MASTER_STATE\
                            | BLE_NON_CON_ADV_SLAVE_STATE | BLE_DISC_ADV_SLAVE_STATE\
                            | BLE_PASS_SCAN_INIT_STATE | BLE_ACTIV_SCAN_INIT_STATE)

/// States byte 3
#define BLE_STATES_BYTE3    ( BLE_PASS_SCAN_MASTER_STATE | BLE_ACTIV_SCAN_MASTER_STATE\
                            | BLE_PASS_SCAN_SLAVE_STATE | BLE_ACTIV_SCAN_SLAVE_STATE\
                            | BLE_INIT_MASTER_MASTER_STATE | BLE_LDC_DIRECT_ADV_STATE\
                            | BLE_LDC_DIRECT_ADV_PASS_SCAN_STATE | BLE_LDC_DIRECT_ADV_ACTIV_SCAN_STATE)

/// States byte 4
#define BLE_STATES_BYTE4    ( BLE_CON_ADV_INIT_MASTER_SLAVE_STATE | BLE_HDC_DIRECT_ADV_INIT_MASTER_SLAVE_STATE\
                            | BLE_LDC_DIRECT_ADV_INIT_MASTER_SLAVE_STATE | BLE_CON_ADV_MASTER_SLAVE_STATE\
                            | BLE_HDC_DIRECT_ADV_MASTER_SLAVE_STATE | BLE_LDC_DIRECT_ADV_MASTER_SLAVE_STATE\
                            | BLE_CON_ADV_SLAVE_SLAVE_STATE | BLE_HDC_DIRECT_ADV_SLAVE_SLAVE_STATE)

/// States byte 5
#define BLE_STATES_BYTE5    ( BLE_LDC_DIRECT_ADV_SLAVE_SLAVE_STATE |  BLE_INIT_MASTER_SLAVE_STATE)
#else
/// States byte 1
#define BLE_STATES_BYTE1    0x0

/// States byte 2
#define BLE_STATES_BYTE2    ( BLE_NON_CON_ADV_MASTER_STATE | BLE_DISC_ADV_MASTER_STATE\
                            | BLE_NON_CON_ADV_SLAVE_STATE | BLE_DISC_ADV_SLAVE_STATE)

/// States byte 3
#define BLE_STATES_BYTE3    ( BLE_PASS_SCAN_MASTER_STATE | BLE_ACTIV_SCAN_MASTER_STATE\
                            | BLE_PASS_SCAN_SLAVE_STATE | BLE_ACTIV_SCAN_SLAVE_STATE\
                            | BLE_INIT_MASTER_MASTER_STATE)

/// States byte 4
#define BLE_STATES_BYTE4    0x0

/// States byte 5
#define BLE_STATES_BYTE5    0x0
#endif
/// States byte 6
#define BLE_STATES_BYTE6    0x0
/// States byte 7
#define BLE_STATES_BYTE7    0x0

///Le Initiating enables
enum
{
    INIT_DIS                  = 0x00,
    INIT_EN,
    INIT_EN_END
};
///Le Direct test types
enum
{
    TEST_TX                  = 0x00,
    TEST_RX,
    TEST_RXTX,
    TEST_END
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct llm_le_env_tag llm_le_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the BLE LLM task
 *
 * This function initializes the the LLC task, as well as the environment of the LLM
 *
 ****************************************************************************************
 */
void llm_init(bool reset);

/**
 ****************************************************************************************
 * @brief Sends the NOP event.
 *
 * This function sends the No Operation command completed event to the host when all the
 * Initializations are done.
 *
 ****************************************************************************************
 */
void llm_ble_ready(void);

/**
 ****************************************************************************************
 * @brief Handle the command clear the white list.
 *
 * This function clear the public and private white lists.
 *
 ****************************************************************************************
 */
void llm_wl_clr(void);

/**
 ****************************************************************************************
 * @brief Handle the command set advertising parameters.
 *
 * This function checks the parameters , fulfill the advertising packet with the useful
 * parameters
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the advertising mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_adv_param(struct hci_le_set_adv_param_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set advertising data.
 *
 * This function checks the parameters , fulfill the advertising packet with the data
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the advertising data.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
#if (BLE_BROADCASTER || BLE_PERIPHERAL)
uint8_t llm_set_adv_data(struct hci_le_set_adv_data_cmd const *param);
#endif

/**
 ****************************************************************************************
 * @brief Handle the command set advertising enable.
 *
 * This function checks the parameters , starts or stops the requested advertising mode.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to start or stop the advertising mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_adv_en(struct hci_le_set_adv_en_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set scanning parameters.
 *
 * This function checks the parameters , fulfill the scanning packet with the useful
 * parameters
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the scanning mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_scan_param(struct hci_le_set_scan_param_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set scanning enable.
 *
 * This function checks the parameters , starts or stops the requested scanning mode.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to start or stop the scanning mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_scan_en(struct hci_le_set_scan_en_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set scanning response data.
 *
 * This function checks the parameters , fulfill the scanning packet with the data
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the scanning response data.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_scan_rsp_data(struct hci_le_set_scan_rsp_data_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command start transmit test mode.
 *
 * This function checks the parameters , set the transmit mode parameters, turn on the
 * mode and set the LLM state.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the transmit test mode.
 * @param[in] msgid        Type of the command Legacy or Enhanced test mode
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_test_mode_start_tx(void const *param, kernel_msg_id_t const msgid);

/**
 ****************************************************************************************
 * @brief Handle the command start receive test mode.
 *
 * This function checks the parameters , set the receive mode parameters, turn on the
 * mode and set the LLM state.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the receive test mode.
 * @param[in] msgid        Type of the command Legacy or Enhanced test mode
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_test_mode_start_rx(void const *param, kernel_msg_id_t const msgid);

/**
 ****************************************************************************************
 * @brief Handle the command add device in the white list.
 *
 * This function adds the device in the white list, according to the type (public or
 * private)
 *
 * @param[in] bd_addr      BLE Address value
 * @param[in] bd_addr_type BLE Address type
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_wl_dev_add_hdl(struct bd_addr *bd_addr, uint8_t bd_addr_type);

/**
 ****************************************************************************************
 * @brief Handle the command remove device in the white list.
 *
 * This function removes the device in the white list, according to the type (public or
 * private)
 *
 * @param[in] bd_addr      BLE Address value
 * @param[in] bd_addr_type BLE Address type
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_wl_dev_rem_hdl(struct bd_addr *bd_addr, uint8_t bd_addr_type);

/**
 ****************************************************************************************
 * @brief Add a device in HW white list
 *
 * @param[in] bd_addr      BLE Address value
 * @param[in] bd_addr_type BLE Address type
 *
 ****************************************************************************************
 */
void llm_wl_dev_add(struct bd_addr *bd_addr, uint8_t bd_addr_type);

/**
 ****************************************************************************************
 * @brief Remove  a device from HW white list
 *
 * @param[in] bd_addr      BLE Address value
 * @param[in] bd_addr_type BLE Address type
 *
 ****************************************************************************************
 */
void llm_wl_dev_rem(struct bd_addr *bd_addr, uint8_t bd_addr_type);

/**
 ****************************************************************************************
 * @brief Handle the command create connection.
 *
 * This function checks the parameters, fulfill the connect_req pdu with the useful
 * parameters, changes the state of the LLM task and requests the LLD to schedule the
 * connection initiation.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to create the link.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_create_con(struct hci_le_create_con_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command start encryption.
 *
 * This function sets the key and the data to be encrypted in the encryption engine and
 * start it.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to star tthe encryption engine.
 *
 ****************************************************************************************
 */
void llm_encryption_start(struct llm_enc_req const *param);

/**
 ****************************************************************************************
 * @brief Handle the end of the encryption process.
 *
 * This function provide to the host or the LLC the data encrypted by the engine.
 *
 ****************************************************************************************
 */
void llm_encryption_done(void);

/**
 ****************************************************************************************
 * @brief Sends the command complete event.
 *
 * This function notify the host that the command is completed.
 *
 * @param[in] opcode        Command opcode
 * @param[in] status        Status on the completion of the command.
 ****************************************************************************************
 */
void llm_common_cmd_complete_send(uint16_t opcode, uint8_t status);

/**
 ****************************************************************************************
 * @brief Sends the command status event.
 *
 * This function notify the host that the command is understood.
 *
 * @param[in] opcode        Command opcode
 * @param[in] status        Status on the completion of the command.
 ****************************************************************************************
 */
void llm_common_cmd_status_send(uint16_t opcode, uint8_t status);

/**
 ****************************************************************************************
 * @brief Handles the connection request pdu.
 *
 * This function extracts the parameters from the packet received and takes the
 * associated actions (new pdu generation, new state , parameters update, etc...).
 *
 * @param[in] rxdesc         Pointer on the received pdu packet.
 * @param[in] status         Packet status bit field
 *
 ****************************************************************************************
 */
void llm_con_req_ind(uint8_t rx_hdl, uint16_t status);

#if (BLE_CENTRAL || BLE_OBSERVER)
/**
 ****************************************************************************************
 * @brief Handles the advertising packet.
 *
 * This function extracts the parameters from the advertising packet received and
 * generates the event if needed
 *
 * @param[in] rxdesc         Pointer on the received advertising packet.
 *
 ****************************************************************************************
 */
void llm_le_adv_report_ind(uint8_t rx_hdl);
#endif // BLE_CENTRAL || BLE_OBSERVER

/**
 ****************************************************************************************
 * @brief Handles the transmission confirmation of the packet.
 *
 * This function sends the connection complete event to the host and start the LLC task
 * for the dedicated conhdl.
 *
 * @param[in] rx_hdl         Handle on the received advertising packet who has generated
 *                           the sending of the connect_req pdu.
 *
 ****************************************************************************************
 */
void llm_con_req_tx_cfm(uint8_t  rx_hdl);

/**
 ****************************************************************************************
 * @brief Clear the resolution address list
 ****************************************************************************************
 */
void llm_ral_clear(void);


/**
 ****************************************************************************************
 * @brief Add A device into resolving address list
 *
 * @param[in] param command parameters
 *
 * @return status of the request
 ****************************************************************************************
 */
uint8_t llm_ral_dev_add(struct hci_le_add_dev_to_rslv_list_cmd *param);

/**
 ****************************************************************************************
 * @brief Remove a device from resolving address list
 *
 * @param[in] param command parameters
 *
 * @return status of the request
 ****************************************************************************************
 */
uint8_t llm_ral_dev_rm(struct hci_le_rmv_dev_from_rslv_list_cmd *param);


/**
 ****************************************************************************************
 * @brief Retrieve value of peer or local Resolvable Private Address (RPA)
 *
 * @param[in]  param command parameters
 * @param[out] addr  RPA found
 * @param[in]  local True to retrieve local RPA, else peer RPA
 *
 * @return status of the request
 ****************************************************************************************
 */
uint8_t llm_ral_get_rpa(struct hci_le_rd_loc_rslv_addr_cmd *param, struct bd_addr *addr, bool local);

/**
 ****************************************************************************************
 * @brief Modify Resolvable Private Address Renew Timeout duration
 *
 * @param[in]  param command parameters
 *
 * @return status of the request
 ****************************************************************************************
 */
uint8_t llm_ral_set_timeout(struct hci_le_set_rslv_priv_addr_to_cmd *param);


/**
 ****************************************************************************************
 * @brief Update state of the enhanced privacy
 ****************************************************************************************
 */
void llm_ral_update(void);

/// @} LLM

#endif // LLM_H_
