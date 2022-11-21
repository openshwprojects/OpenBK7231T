/*
 * hostapd / Configuration helper functions
 * Copyright (c) 2003-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "crypto/sha1.h"
#include "common/ieee802_11_defs.h"
#include "common/eapol_common.h"
#include "wpa_auth.h"
#include "ap/sta_info.h"
#include "ap_config.h"


static void hostapd_config_free_vlan(struct hostapd_bss_config *bss)
{
	struct hostapd_vlan *vlan, *prev;

	vlan = bss->vlan;
	prev = NULL;
	while (vlan) {
		prev = vlan;
		vlan = vlan->next;
		os_free(prev);
	}

	bss->vlan = NULL;
}


void hostapd_config_defaults_bss(struct hostapd_bss_config *bss)
{
	bss->logger_syslog_level = HOSTAPD_LEVEL_INFO;
	bss->logger_stdout_level = HOSTAPD_LEVEL_INFO;
	bss->logger_syslog = (unsigned int) -1;
	bss->logger_stdout = (unsigned int) -1;

	bss->auth_algs = WPA_AUTH_ALG_OPEN | WPA_AUTH_ALG_SHARED;

	bss->wep_rekeying_period = 300;
	/* use key0 in individual key and key1 in broadcast key */
	bss->broadcast_key_idx_min = 1;
	bss->broadcast_key_idx_max = 2;
	bss->eap_reauth_period = 3600;

	bss->wpa_group_rekey = 600;
	bss->wpa_gmk_rekey = 86400;
	bss->wpa_key_mgmt = WPA_KEY_MGMT_PSK;
	bss->wpa_pairwise = WPA_CIPHER_TKIP;
	bss->wpa_group = WPA_CIPHER_TKIP;
	bss->rsn_pairwise = 0;

	bss->max_num_sta = MAX_STA_COUNT;

	bss->dtim_period = 2;

	bss->radius_server_auth_port = 1812;
	bss->ap_max_inactivity = AP_MAX_INACTIVITY;
	bss->eapol_version = EAPOL_VERSION;

	bss->max_listen_interval = 65535;

	bss->pwd_group = 19; /* ECC: GF(p=256) */

#ifdef CONFIG_IEEE80211W
	bss->assoc_sa_query_max_timeout = 1000;
	bss->assoc_sa_query_retry_timeout = 201;
	bss->group_mgmt_cipher = WPA_CIPHER_AES_128_CMAC;
#endif /* CONFIG_IEEE80211W */
#ifdef EAP_SERVER_FAST
	 /* both anonymous and authenticated provisioning */
	bss->eap_fast_prov = 3;
	bss->pac_key_lifetime = 7 * 24 * 60 * 60;
	bss->pac_key_refresh_time = 1 * 24 * 60 * 60;
#endif /* EAP_SERVER_FAST */

	/* Set to -1 as defaults depends on HT in setup */
	bss->wmm_enabled = -1;

#ifdef CONFIG_IEEE80211R
	bss->ft_over_ds = 1;
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_SAE
	bss->sae_anti_clogging_threshold = 5;
#endif
}


struct hostapd_config * hostapd_config_defaults(void)
{
#define ecw2cw(ecw) ((1 << (ecw)) - 1)

	struct hostapd_config *conf;
	struct hostapd_bss_config *bss;
	const int aCWmin = 4, aCWmax = 10;
	const struct hostapd_wmm_ac_params ac_bk =
		{ aCWmin, aCWmax, 7, 0, 0 }; /* background traffic */
	const struct hostapd_wmm_ac_params ac_be =
		{ aCWmin, aCWmax, 3, 0, 0 }; /* best effort traffic */
	const struct hostapd_wmm_ac_params ac_vi = /* video traffic */
		{ aCWmin - 1, aCWmin, 2, 3008 / 32, 0 };
	const struct hostapd_wmm_ac_params ac_vo = /* voice traffic */
		{ aCWmin - 2, aCWmin - 1, 2, 1504 / 32, 0 };
	const struct hostapd_tx_queue_params txq_bk =
		{ 7, ecw2cw(aCWmin), ecw2cw(aCWmax), 0 };
	const struct hostapd_tx_queue_params txq_be =
		{ 3, ecw2cw(aCWmin), 4 * (ecw2cw(aCWmin) + 1) - 1, 0};
	const struct hostapd_tx_queue_params txq_vi =
		{ 1, (ecw2cw(aCWmin) + 1) / 2 - 1, ecw2cw(aCWmin), 30};
	const struct hostapd_tx_queue_params txq_vo =
		{ 1, (ecw2cw(aCWmin) + 1) / 4 - 1,
		  (ecw2cw(aCWmin) + 1) / 2 - 1, 15};

#undef ecw2cw

	conf = os_zalloc(sizeof(*conf));
	bss = os_zalloc(sizeof(*bss));
	if (conf == NULL || bss == NULL) {
		wpa_printf(MSG_ERROR, "Failed to allocate memory for "
			   "configuration data.");
		os_free(conf);
		os_free(bss);
		return NULL;
	}
	conf->bss = os_calloc(1, sizeof(struct hostapd_bss_config *));
	if (conf->bss == NULL) {
		os_free(conf);
		os_free(bss);
		return NULL;
	}
	conf->bss[0] = bss;

	hostapd_config_defaults_bss(bss);

	conf->num_bss = 1;

	conf->beacon_int = 100;
	conf->rts_threshold = -1; /* use driver default: 2347 */
	conf->fragm_threshold = -1; /* user driver default: 2346 */
	conf->send_probe_response = 1;
	/* Set to invalid value means do not add Power Constraint IE */
	conf->local_pwr_constraint = -1;

	conf->wmm_ac_params[0] = ac_be;
	conf->wmm_ac_params[1] = ac_bk;
	conf->wmm_ac_params[2] = ac_vi;
	conf->wmm_ac_params[3] = ac_vo;

	conf->tx_queue[0] = txq_vo;
	conf->tx_queue[1] = txq_vi;
	conf->tx_queue[2] = txq_be;
	conf->tx_queue[3] = txq_bk;

	conf->ht_capab = HT_CAP_INFO_SMPS_DISABLED;

	conf->ap_table_max_size = 255;
	conf->ap_table_expiration_time = 60;
	conf->track_sta_max_age = 180;

	conf->acs = 0;
	conf->acs_ch_list.num = 0;
#ifdef CONFIG_ACS
	conf->acs_num_scans = 5;
#endif /* CONFIG_ACS */

	return conf;
}


int hostapd_mac_comp(const void *a, const void *b)
{
	return os_memcmp(a, b, sizeof(macaddr));
}


int hostapd_mac_comp_empty(const void *a)
{
	macaddr empty = { 0 };
	return os_memcmp(a, empty, sizeof(macaddr));
}

static int hostapd_derive_psk(struct hostapd_ssid *ssid)
{
	ssid->wpa_psk = os_zalloc(sizeof(struct hostapd_wpa_psk));
	if (ssid->wpa_psk == NULL) {
		wpa_printf(MSG_ERROR, "Unable to alloc space for PSK");
		return -1;
	}
	wpa_hexdump_ascii(MSG_DEBUG, "SSID",
			  (u8 *) ssid->ssid, ssid->ssid_len);
	wpa_hexdump_ascii_key(MSG_DEBUG, "PSK (ASCII passphrase)",
			      (u8 *) ssid->wpa_passphrase,
			      os_strlen(ssid->wpa_passphrase));
	pbkdf2_sha1(ssid->wpa_passphrase,
		    ssid->ssid, ssid->ssid_len,
		    4096, ssid->wpa_psk->psk, PMK_LEN);
	wpa_hexdump_key(MSG_DEBUG, "PSK (from passphrase)",
			ssid->wpa_psk->psk, PMK_LEN);
	return 0;
}


int hostapd_setup_wpa_psk(struct hostapd_bss_config *conf)
{
	struct hostapd_ssid *ssid = &conf->ssid;

	if (ssid->wpa_passphrase != NULL) {
		if (ssid->wpa_psk != NULL) {
			wpa_printf(MSG_DEBUG, "Using pre-configured WPA PSK "
				   "instead of passphrase");
		} else {
			wpa_printf(MSG_DEBUG, "Deriving WPA PSK based on "
				   "passphrase");
			if (hostapd_derive_psk(ssid) < 0)
				return -1;
		}
		ssid->wpa_psk->group = 1;
	}

	return 0;
}

static void hostapd_config_free_wep(struct hostapd_wep_keys *keys)
{
	int i;
	for (i = 0; i < NUM_WEP_KEYS; i++) {
		bin_clear_free(keys->key[i], keys->len[i]);
		keys->key[i] = NULL;
	}
}


void hostapd_config_clear_wpa_psk(struct hostapd_wpa_psk **l)
{
	struct hostapd_wpa_psk *psk, *tmp;

	for (psk = *l; psk;) {
		tmp = psk;
		psk = psk->next;
		bin_clear_free(tmp, sizeof(*tmp));
	}
	*l = NULL;
}


void hostapd_config_free_bss(struct hostapd_bss_config *conf)
{
	//struct hostapd_eap_user *user, *prev_user;

	if (conf == NULL)
		return;

	hostapd_config_clear_wpa_psk(&conf->ssid.wpa_psk);

	str_clear_free(conf->ssid.wpa_passphrase);
 	hostapd_config_free_wep(&conf->ssid.wep);
#ifdef CONFIG_FULL_DYNAMIC_VLAN
	os_free(conf->ssid.vlan_tagged_interface);
#endif /* CONFIG_FULL_DYNAMIC_VLAN */

	os_free(conf->rsn_preauth_interfaces);
	os_free(conf->ctrl_interface);
	os_free(conf->ca_cert);
	os_free(conf->server_cert);
	os_free(conf->private_key);
	os_free(conf->private_key_passwd);
	os_free(conf->ocsp_stapling_response);
	os_free(conf->dh_file);
	os_free(conf->openssl_ciphers);
	os_free(conf->pac_opaque_encr_key);
	os_free(conf->eap_fast_a_id);
	os_free(conf->eap_fast_a_id_info);
	os_free(conf->time_zone);

#ifdef CONFIG_IEEE80211R
	{
		struct ft_remote_r0kh *r0kh, *r0kh_prev;
		struct ft_remote_r1kh *r1kh, *r1kh_prev;

		r0kh = conf->r0kh_list;
		conf->r0kh_list = NULL;
		while (r0kh) {
			r0kh_prev = r0kh;
			r0kh = r0kh->next;
			os_free(r0kh_prev);
		}

		r1kh = conf->r1kh_list;
		conf->r1kh_list = NULL;
		while (r1kh) {
			r1kh_prev = r1kh;
			r1kh = r1kh->next;
			os_free(r1kh_prev);
		}
	}
#endif /* CONFIG_IEEE80211R */

	os_free(conf->roaming_consortium);
	os_free(conf->venue_name);
	os_free(conf->nai_realm_data);
	os_free(conf->network_auth_type);
	os_free(conf->anqp_3gpp_cell_net);
	os_free(conf->domain_name);

	wpabuf_free(conf->vendor_elements);

	os_free(conf->sae_groups);

	os_free(conf->wowlan_triggers);

	os_free(conf->server_id);

	os_free(conf->no_probe_resp_if_seen_on);
	os_free(conf->no_auth_if_seen_on);

	os_free(conf);
}


/**
 * hostapd_config_free - Free hostapd configuration
 * @conf: Configuration data from hostapd_config_read().
 */
void hostapd_config_free(struct hostapd_config *conf)
{
	size_t i;

	if (conf == NULL)
		return;

	for (i = 0; i < conf->num_bss; i++)
		hostapd_config_free_bss(conf->bss[i]);
	os_free(conf->bss);
	os_free(conf->supported_rates);
	os_free(conf->basic_rates);
	os_free(conf->acs_ch_list.range);
	os_free(conf->driver_params);
#ifdef CONFIG_ACS
	os_free(conf->acs_chan_bias);
#endif /* CONFIG_ACS */

	os_free(conf);
}


/**
 * hostapd_maclist_found - Find a MAC address from a list
 * @list: MAC address list
 * @num_entries: Number of addresses in the list
 * @addr: Address to search for
 * @vlan_id: Buffer for returning VLAN ID or %NULL if not needed
 * Returns: 1 if address is in the list or 0 if not.
 *
 * Perform a binary search for given MAC address from a pre-sorted list.
 */
int hostapd_maclist_found(struct mac_acl_entry *list, int num_entries,
			  const u8 *addr, int *vlan_id)
{
	int start, end, middle, res;

	start = 0;
	end = num_entries - 1;

	while (start <= end) {
		middle = (start + end) / 2;
		res = os_memcmp(list[middle].addr, addr, ETH_ALEN);
		if (res == 0) {
			if (vlan_id)
				*vlan_id = list[middle].vlan_id;
			return 1;
		}
		if (res < 0)
			start = middle + 1;
		else
			end = middle - 1;
	}

	return 0;
}


int hostapd_rate_found(int *list, int rate)
{
	int i;

	if (list == NULL)
		return 0;

	for (i = 0; list[i] >= 0; i++)
		if (list[i] == rate)
			return 1;

	return 0;
}

const u8 * hostapd_get_psk(const struct hostapd_bss_config *conf,
			   const u8 *addr, const u8 *p2p_dev_addr,
			   const u8 *prev_psk)
{
	struct hostapd_wpa_psk *psk;
	int next_ok = prev_psk == NULL;

	if (p2p_dev_addr && !is_zero_ether_addr(p2p_dev_addr)) {
		wpa_printf(MSG_DEBUG, "Searching a PSK for " MACSTR
			   " p2p_dev_addr=" MACSTR " prev_psk=%p",
			   MAC2STR(addr), MAC2STR(p2p_dev_addr), prev_psk);
		addr = NULL; /* Use P2P Device Address for matching */
	} else {
		wpa_printf(MSG_DEBUG, "Searching a PSK for " MACSTR
			   " prev_psk=%p",
			   MAC2STR(addr), prev_psk);
	}

	for (psk = conf->ssid.wpa_psk; psk != NULL; psk = psk->next) {
		if (next_ok &&
		    (psk->group ||
		     (addr && os_memcmp(psk->addr, addr, ETH_ALEN) == 0) ||
		     (!addr && p2p_dev_addr &&
		      os_memcmp(psk->p2p_dev_addr, p2p_dev_addr, ETH_ALEN) ==
		      0)))
			return psk->psk;

		if (psk->psk == prev_psk)
			next_ok = 1;
	}

	return NULL;
}


static int hostapd_config_check_bss(struct hostapd_bss_config *bss,
				    struct hostapd_config *conf,
				    int full_config)
{
	if (bss->wpa) {
		int wep, i;

		wep = bss->default_wep_key_len > 0 ||
		       bss->individual_wep_key_len > 0;
		for (i = 0; i < NUM_WEP_KEYS; i++) {
			if (bss->ssid.wep.keys_set) {
				wep = 1;
				break;
			}
		}

		if (wep) {
			wpa_printf(MSG_ERROR, "WEP configuration in a WPA network is not supported");
			return -1;
		}
	}

	if (full_config && hostapd_mac_comp_empty(bss->bssid) != 0) {
		size_t i;

		for (i = 0; i < conf->num_bss; i++) {
			if (conf->bss[i] != bss &&
			    (hostapd_mac_comp(conf->bss[i]->bssid,
					      bss->bssid) == 0)) {
				wpa_printf(MSG_ERROR, "Duplicate BSSID " MACSTR
					   " on interface '%s' and '%s'.",
					   MAC2STR(bss->bssid),
					   conf->bss[i]->iface, bss->iface);
				return -1;
			}
		}
	}

#ifdef CONFIG_IEEE80211R
	if (full_config && wpa_key_mgmt_ft(bss->wpa_key_mgmt) &&
	    (bss->nas_identifier == NULL ||
	     os_strlen(bss->nas_identifier) < 1 ||
	     os_strlen(bss->nas_identifier) > FT_R0KH_ID_MAX_LEN)) {
		wpa_printf(MSG_ERROR, "FT (IEEE 802.11r) requires "
			   "nas_identifier to be configured as a 1..48 octet "
			   "string");
		return -1;
	}
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_IEEE80211N
	if (full_config && conf->ieee80211n &&
	    conf->hw_mode == HOSTAPD_MODE_IEEE80211B) {
		bss->disable_11n = 1;
		wpa_printf(MSG_ERROR, "HT (IEEE 802.11n) in 11b mode is not "
			   "allowed, disabling HT capabilities");
	}

	if (full_config && conf->ieee80211n &&
	    bss->ssid.security_policy == SECURITY_STATIC_WEP) {
		bss->disable_11n = 1;
		wpa_printf(MSG_ERROR, "HT (IEEE 802.11n) with WEP is not "
			   "allowed, disabling HT capabilities");
	}

	if (full_config && conf->ieee80211n && bss->wpa &&
	    !(bss->wpa_pairwise & WPA_CIPHER_CCMP) &&
	    !(bss->rsn_pairwise & (WPA_CIPHER_CCMP | WPA_CIPHER_GCMP |
				   WPA_CIPHER_CCMP_256 | WPA_CIPHER_GCMP_256)))
	{
		bss->disable_11n = 1;
		wpa_printf(MSG_ERROR, "HT (IEEE 802.11n) with WPA/WPA2 "
			   "requires CCMP/GCMP to be enabled, disabling HT "
			   "capabilities");
	}
#endif /* CONFIG_IEEE80211N */

	return 0;
}


static int hostapd_config_check_cw(struct hostapd_config *conf, int queue)
{
	int tx_cwmin = conf->tx_queue[queue].cwmin;
	int tx_cwmax = conf->tx_queue[queue].cwmax;
	int ac_cwmin = conf->wmm_ac_params[queue].cwmin;
	int ac_cwmax = conf->wmm_ac_params[queue].cwmax;

	if (tx_cwmin > tx_cwmax) {
		wpa_printf(MSG_ERROR,
			   "Invalid TX queue cwMin/cwMax values. cwMin(%d) greater than cwMax(%d)",
			   tx_cwmin, tx_cwmax);
		return -1;
	}
	if (ac_cwmin > ac_cwmax) {
		wpa_printf(MSG_ERROR,
			   "Invalid WMM AC cwMin/cwMax values. cwMin(%d) greater than cwMax(%d)",
			   ac_cwmin, ac_cwmax);
		return -1;
	}
	return 0;
}


int hostapd_config_check(struct hostapd_config *conf, int full_config)
{
	size_t i;

	if (full_config && conf->ieee80211d &&
	    (!conf->country[0] || !conf->country[1])) {
		wpa_printf(MSG_ERROR, "Cannot enable IEEE 802.11d without "
			   "setting the country_code");
		return -1;
	}

	if (full_config && conf->ieee80211h && !conf->ieee80211d) {
		wpa_printf(MSG_ERROR, "Cannot enable IEEE 802.11h without "
			   "IEEE 802.11d enabled");
		return -1;
	}

	if (full_config && conf->local_pwr_constraint != -1 &&
	    !conf->ieee80211d) {
		wpa_printf(MSG_ERROR, "Cannot add Power Constraint element without Country element");
		return -1;
	}

	if (full_config && conf->spectrum_mgmt_required &&
	    conf->local_pwr_constraint == -1) {
		wpa_printf(MSG_ERROR, "Cannot set Spectrum Management bit without Country and Power Constraint elements");
		return -1;
	}

	for (i = 0; i < NUM_TX_QUEUES; i++) {
		if (hostapd_config_check_cw(conf, i))
			return -1;
	}

	for (i = 0; i < conf->num_bss; i++) {
		if (hostapd_config_check_bss(conf->bss[i], conf, full_config))
			return -1;
	}

	return 0;
}


void hostapd_set_security_params(struct hostapd_bss_config *bss,
				 int full_config)
{
	if (bss->individual_wep_key_len == 0) {
		/* individual keys are not use; can use key idx0 for
		 * broadcast keys */
		bss->broadcast_key_idx_min = 0;
	}

	if ((bss->wpa & 2) && bss->rsn_pairwise == 0)
		bss->rsn_pairwise = bss->wpa_pairwise;
	bss->wpa_group = wpa_select_ap_group_cipher(bss->wpa, bss->wpa_pairwise,
						    bss->rsn_pairwise);

	if (bss->wpa && bss->ieee802_1x) {
		bss->ssid.security_policy = SECURITY_WPA;
	} else if (bss->wpa) {
		bss->ssid.security_policy = SECURITY_WPA_PSK;
	} else if (bss->ieee802_1x) {
		int cipher = WPA_CIPHER_NONE;
		bss->ssid.security_policy = SECURITY_IEEE_802_1X;
		bss->ssid.wep.default_len = bss->default_wep_key_len;
		if (full_config && bss->default_wep_key_len) {
			cipher = bss->default_wep_key_len >= 13 ?
				WPA_CIPHER_WEP104 : WPA_CIPHER_WEP40;
		} else if (full_config && bss->ssid.wep.keys_set) {
			if (bss->ssid.wep.len[0] >= 13)
				cipher = WPA_CIPHER_WEP104;
			else
				cipher = WPA_CIPHER_WEP40;
		}
		bss->wpa_group = cipher;
		bss->wpa_pairwise = cipher;
		bss->rsn_pairwise = cipher;
		if (full_config)
			bss->wpa_key_mgmt = WPA_KEY_MGMT_IEEE8021X_NO_WPA;
	} else if (bss->ssid.wep.keys_set) {
		int cipher = WPA_CIPHER_WEP40;
		if (bss->ssid.wep.len[0] >= 13)
			cipher = WPA_CIPHER_WEP104;
		bss->ssid.security_policy = SECURITY_STATIC_WEP;
		bss->wpa_group = cipher;
		bss->wpa_pairwise = cipher;
		bss->rsn_pairwise = cipher;
		if (full_config)
			bss->wpa_key_mgmt = WPA_KEY_MGMT_NONE;
	} else if (bss->osen) {
		bss->ssid.security_policy = SECURITY_OSEN;
		bss->wpa_group = WPA_CIPHER_CCMP;
		bss->wpa_pairwise = 0;
		bss->rsn_pairwise = WPA_CIPHER_CCMP;
	} else {
		bss->ssid.security_policy = SECURITY_PLAINTEXT;
		if (full_config) {
			bss->wpa_group = WPA_CIPHER_NONE;
			bss->wpa_pairwise = WPA_CIPHER_NONE;
			bss->rsn_pairwise = WPA_CIPHER_NONE;
			bss->wpa_key_mgmt = WPA_KEY_MGMT_NONE;
		}
	}
}
