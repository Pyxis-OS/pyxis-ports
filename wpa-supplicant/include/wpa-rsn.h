#ifndef PYXIS_WPA_RSN_H
#define PYXIS_WPA_RSN_H

#include "utils/includes.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "rsn_supp/wpa.h"
#include "crypto/sha1.h"

/* Native consumers implement the declarations in utils/os.h and utils/eloop.h.
 * Configure only RSN, WPA_KEY_MGMT_PSK and CCMP; this library does not own
 * policy, launch authority, an input device, or any native grant.
 * wpa_sm_init takes ownership of its heap-allocated wpa_sm_ctx on success.
 * No IEEE8021X_EAPOL build: upstream disables preauthentication/PMKSA caching.
 */

#endif
