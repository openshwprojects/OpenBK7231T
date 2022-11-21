/*
 * hostapd / IEEE 802.1X-2004 Authenticator
 * Copyright (c) 2002-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"
#include "crypto/md5.h"
#include "crypto/crypto.h"
#include "crypto/random.h"
#include "common/ieee802_11_defs.h"
#include "ap/hostapd.h"
#include "accounting.h"
#include "ap/sta_info.h"
#include "wpa_auth.h"
#include "preauth_auth.h"
#include "ap_config.h"
#include "ap_drv_ops.h"
#include "wps_hostapd.h"
#include "ieee802_1x.h"


static void ieee802_1x_finished(struct hostapd_data *hapd,
				struct sta_info *sta, int success,
				int remediation);


void ieee802_1x_set_sta_authorized(struct hostapd_data *hapd,
				   struct sta_info *sta, int authorized)
{
	int res;

	if (sta->flags & WLAN_STA_PREAUTH)
		return;

	if (authorized) {
		ap_sta_set_authorized(hapd, sta, 1);
		res = hostapd_set_authorized(hapd, sta, 1);
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE8021X,
			       HOSTAPD_LEVEL_DEBUG, "authorizing port");
	} else {
		ap_sta_set_authorized(hapd, sta, 0);
		res = hostapd_set_authorized(hapd, sta, 0);
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE8021X,
			       HOSTAPD_LEVEL_DEBUG, "unauthorizing port");
	}
#ifdef CONFIG_NO_ACCOUNTING
    (void)res;
	if (authorized) {
		os_get_reltime(&sta->connected_time);
		accounting_sta_start(hapd, sta);
	}
#endif
}

/**
 * ieee802_1x_receive - Process the EAPOL frames from the Supplicant
 * @hapd: hostapd BSS data
 * @sa: Source address (sender of the EAPOL frame)
 * @buf: EAPOL frame
 * @len: Length of buf in octets
 *
 * This function is called for each incoming EAPOL frame from the interface
 */
void ieee802_1x_receive(struct hostapd_data *hapd, const u8 *sa, const u8 *buf,
			size_t len)
{
	struct sta_info *sta;
	struct ieee802_1x_hdr *hdr;
	struct ieee802_1x_eapol_key *key;
	u16 datalen;
	//struct rsn_pmksa_cache_entry *pmksa;
	//int key_mgmt;

	if (!hapd->conf->ieee802_1x && !hapd->conf->wpa && !hapd->conf->osen &&
	    !hapd->conf->wps_state)
		return;

	wpa_printf(MSG_DEBUG, "IEEE 802.1X: %lu bytes from " MACSTR,
		   (unsigned long) len, MAC2STR(sa));
	sta = ap_get_sta(hapd, sa);
	if (!sta || (!(sta->flags & (WLAN_STA_ASSOC | WLAN_STA_PREAUTH)) &&
		     !(hapd->iface->drv_flags & WPA_DRIVER_FLAGS_WIRED))) {
		wpa_printf(MSG_DEBUG, "IEEE 802.1X data frame from not "
			   "associated/Pre-authenticating STA");
		return;
	}

	if (len < sizeof(*hdr)) {
		wpa_printf(MSG_INFO, "   too short IEEE 802.1X packet");
		return;
	}

	hdr = (struct ieee802_1x_hdr *) buf;
	datalen = be_to_host16(hdr->length);
	wpa_printf(MSG_DEBUG, "   IEEE 802.1X: version=%d type=%d length=%d",
		   hdr->version, hdr->type, datalen);

	if (len - sizeof(*hdr) < datalen) {
		wpa_printf(MSG_INFO, "   frame too short for this IEEE 802.1X packet");
		return;
	}
	if (len - sizeof(*hdr) > datalen) {
		wpa_printf(MSG_DEBUG, "   ignoring %lu extra octets after "
			   "IEEE 802.1X packet",
			   (unsigned long) len - sizeof(*hdr) - datalen);
	}


	key = (struct ieee802_1x_eapol_key *) (hdr + 1);
	if (datalen >= sizeof(struct ieee802_1x_eapol_key) &&
	    hdr->type == IEEE802_1X_TYPE_EAPOL_KEY &&
	    (key->type == EAPOL_KEY_TYPE_WPA ||
	     key->type == EAPOL_KEY_TYPE_RSN)) {
		wpa_receive(hapd->wpa_auth, sta->wpa_sm, (u8 *) hdr,
			    sizeof(*hdr) + datalen);
		return;
	}
}

static void ieee802_1x_set_port_authorized(void *ctx, void *sta_ctx,
					   int authorized)
{
	struct hostapd_data *hapd = ctx;
	struct sta_info *sta = sta_ctx;
	ieee802_1x_set_sta_authorized(hapd, sta, authorized);
}

int ieee802_1x_tx_status(struct hostapd_data *hapd, struct sta_info *sta,
			 const u8 *buf, size_t len, int ack)
{
	struct ieee80211_hdr *hdr;
	u8 *pos;
	const unsigned char rfc1042_hdr[ETH_ALEN] =
		{ 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

	if (sta == NULL)
		return -1;
	if (len < sizeof(*hdr) + sizeof(rfc1042_hdr) + 2)
		return 0;

	hdr = (struct ieee80211_hdr *) buf;
	pos = (u8 *) (hdr + 1);
	if (os_memcmp(pos, rfc1042_hdr, sizeof(rfc1042_hdr)) != 0)
		return 0;
	pos += sizeof(rfc1042_hdr);
	if (WPA_GET_BE16(pos) != ETH_P_PAE)
		return 0;
	pos += 2;

	return ieee802_1x_eapol_tx_status(hapd, sta, pos, buf + len - pos,
					  ack);
}


int ieee802_1x_eapol_tx_status(struct hostapd_data *hapd, struct sta_info *sta,
			       const u8 *buf, int len, int ack)
{
	const struct ieee802_1x_hdr *xhdr =
		(const struct ieee802_1x_hdr *) buf;
	const u8 *pos = buf + sizeof(*xhdr);
	struct ieee802_1x_eapol_key *key;

	if (len < (int) sizeof(*xhdr))
		return 0;
	wpa_printf(MSG_DEBUG, "IEEE 802.1X: " MACSTR " TX status - version=%d "
		   "type=%d length=%d - ack=%d",
		   MAC2STR(sta->addr), xhdr->version, xhdr->type,
		   be_to_host16(xhdr->length), ack);

	if (xhdr->type != IEEE802_1X_TYPE_EAPOL_KEY)
		return 0;

	if (pos + sizeof(struct wpa_eapol_key) <= buf + len) {
		const struct wpa_eapol_key *wpa;
		wpa = (const struct wpa_eapol_key *) pos;
		if (wpa->type == EAPOL_KEY_TYPE_RSN ||
		    wpa->type == EAPOL_KEY_TYPE_WPA)
			wpa_auth_eapol_key_tx_status(hapd->wpa_auth,
						     sta->wpa_sm, ack);
	}

	/* EAPOL EAP-Packet packets are eventually re-sent by either Supplicant
	 * or Authenticator state machines, but EAPOL-Key packets are not
	 * retransmitted in case of failure. Try to re-send failed EAPOL-Key
	 * packets couple of times because otherwise STA keys become
	 * unsynchronized with AP. */
	if (!ack && pos + sizeof(*key) <= buf + len) {
		key = (struct ieee802_1x_eapol_key *) pos;
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE8021X,
			       HOSTAPD_LEVEL_DEBUG, "did not Ack EAPOL-Key "
			       "frame (%scast index=%d)",
			       key->key_index & BIT(7) ? "uni" : "broad",
			       key->key_index & ~BIT(7));
		/* TODO: re-send EAPOL-Key couple of times (with short delay
		 * between them?). If all attempt fail, report error and
		 * deauthenticate STA so that it will get new keys when
		 * authenticating again (e.g., after returning in range).
		 * Separate limit/transmit state needed both for unicast and
		 * broadcast keys(?) */
	}
	/* TODO: could move unicast key configuration from ieee802_1x_tx_key()
	 * to here and change the key only if the EAPOL-Key packet was Acked.
	 */
    (void)key;

	return 1;
}

void ieee802_1x_notify_port_enabled(struct eapol_state_machine *sm,
				    int enabled)
{
	if (sm == NULL)
		return;
}


void ieee802_1x_notify_port_valid(struct eapol_state_machine *sm,
				  int valid)
{
	if (sm == NULL)
		return;
}


void ieee802_1x_notify_pre_auth(struct eapol_state_machine *sm, int pre_auth)
{
	if (sm == NULL)
		return;
}

