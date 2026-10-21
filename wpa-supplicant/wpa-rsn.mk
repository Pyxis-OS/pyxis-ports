WPA_RSN_PREFIX := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
WPA_RSN_CPPFLAGS := -I$(WPA_RSN_PREFIX)/include/wpa-rsn \
                    -I$(WPA_RSN_PREFIX)/include/wpa-rsn/utils
WPA_RSN_LIBRARIES := $(WPA_RSN_PREFIX)/lib/libwpa-rsn.a
