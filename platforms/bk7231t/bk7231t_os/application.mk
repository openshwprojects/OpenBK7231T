TOP_DIR = ../../..

# Initialize tool chain  /usr/bin/gcc-arm-none-eabi-4_9-2015q1/bin
# -------------------------------------------------------------------
ifeq ($(shell uname), Linux)
  TOOLCHAIN_DIR := ../toolchain
else
  TOOLCHAIN_DIR := ../toolchain/windows
endif
ARM_GCC_TOOLCHAIN = $(TOOLCHAIN_DIR)/gcc-arm-none-eabi-4_9-2015q1/bin

CROSS_COMPILE = $(ARM_GCC_TOOLCHAIN)/arm-none-eabi-

CFG_WRAP_LIBC  ?= 1
CFG_LWIP_2_0_2 ?= 0
CFG_LWIP_2_1_3 ?= 1

ota_idx = 1

# Compilation tools
AR = $(CROSS_COMPILE)ar
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
NM = $(CROSS_COMPILE)nm
LD = $(CROSS_COMPILE)gcc
GDB = $(CROSS_COMPILE)gdb
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# -------------------------------------------------------------------
# Initialize target name and target object files
# -------------------------------------------------------------------

all: application 

TARGET=Debug

OBJ_DIR=$(TARGET)/obj
BIN_DIR=$(TARGET)/bin

# -------------------------------------------------------------------
# Include folder list
# -------------------------------------------------------------------
INCLUDES =

INCLUDES += -I./beken378/common
INCLUDES += -I./beken378/app
INCLUDES += -I./beken378/app/config
INCLUDES += -I./beken378/app/standalone-station
INCLUDES += -I./beken378/app/standalone-ap
INCLUDES += -I./beken378/ip/common
INCLUDES += -I./beken378/ip/ke/
INCLUDES += -I./beken378/ip/mac/
INCLUDES += -I./beken378/ip/lmac/src/hal
INCLUDES += -I./beken378/ip/lmac/src/mm
INCLUDES += -I./beken378/ip/lmac/src/ps
INCLUDES += -I./beken378/ip/lmac/src/rd
INCLUDES += -I./beken378/ip/lmac/src/rwnx
INCLUDES += -I./beken378/ip/lmac/src/rx
INCLUDES += -I./beken378/ip/lmac/src/scan
INCLUDES += -I./beken378/ip/lmac/src/sta
INCLUDES += -I./beken378/ip/lmac/src/tx
INCLUDES += -I./beken378/ip/lmac/src/vif
INCLUDES += -I./beken378/ip/lmac/src/rx/rxl
INCLUDES += -I./beken378/ip/lmac/src/tx/txl
INCLUDES += -I./beken378/ip/lmac/src/p2p
INCLUDES += -I./beken378/ip/lmac/src/chan
INCLUDES += -I./beken378/ip/lmac/src/td
INCLUDES += -I./beken378/ip/lmac/src/tpc
INCLUDES += -I./beken378/ip/lmac/src/tdls
INCLUDES += -I./beken378/ip/umac/src/mesh
INCLUDES += -I./beken378/ip/umac/src/rc
INCLUDES += -I./beken378/ip/umac/src/apm
INCLUDES += -I./beken378/ip/umac/src/bam
INCLUDES += -I./beken378/ip/umac/src/llc
INCLUDES += -I./beken378/ip/umac/src/me
INCLUDES += -I./beken378/ip/umac/src/rxu
INCLUDES += -I./beken378/ip/umac/src/scanu
INCLUDES += -I./beken378/ip/umac/src/sm
INCLUDES += -I./beken378/ip/umac/src/txu
INCLUDES += -I./beken378/driver/include
INCLUDES += -I./beken378/driver/common/reg
INCLUDES += -I./beken378/driver/entry
INCLUDES += -I./beken378/driver/dma
INCLUDES += -I./beken378/driver/intc
INCLUDES += -I./beken378/driver/phy
INCLUDES += -I./beken378/driver/rc_beken
INCLUDES += -I./beken378/driver/flash
INCLUDES += -I./beken378/driver/rw_pub
INCLUDES += -I./beken378/driver/common/reg
INCLUDES += -I./beken378/driver/common
INCLUDES += -I./beken378/driver/uart
INCLUDES += -I./beken378/driver/i2c
INCLUDES += -I./beken378/driver/sys_ctrl
INCLUDES += -I./beken378/driver/gpio
INCLUDES += -I./beken378/driver/general_dma
INCLUDES += -I./beken378/driver/spidma
INCLUDES += -I./beken378/driver/icu
INCLUDES += -I./beken378/driver/ble 
INCLUDES += -I./beken378/driver/ble/ble_pub/ip/ble/hl/inc 
INCLUDES += -I./beken378/driver/ble/ble_pub/ip/ble/profiles/sdp/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/ip/ble/profiles/comm/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/modules/rwip/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/modules/app/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/modules/common/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/modules/dbg/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/modules/rwip/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/modules/rf/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/modules/ecc_p256/api 
INCLUDES += -I./beken378/driver/ble/ble_pub/plf/refip/src/arch 
INCLUDES += -I./beken378/driver/ble/ble_pub/plf/refip/src/arch/boot 
INCLUDES += -I./beken378/driver/ble/ble_pub/plf/refip/src/arch/compiler 
INCLUDES += -I./beken378/driver/ble/ble_pub/plf/refip/src/arch/ll 
INCLUDES += -I./beken378/driver/ble/ble_pub/plf/refip/src/arch                                                     
INCLUDES += -I./beken378/driver/ble/ble_pub/plf/refip/src/build/ble_full/reg/fw                                    
INCLUDES += -I./beken378/driver/ble/ble_pub/plf/refip/src/driver/reg                                               
INCLUDES += -I./beken378/driver/ble/ble_pub/plf/refip/src/driver/ble_icu                                           
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/inc                                                          
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/api                                                          
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapc 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapm 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/smpc 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/smpm 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/attc 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/attm 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/atts 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/gattc 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/gattm 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/l2c/l2cc 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/hl/src/l2c/l2cm 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/ll/src/rwble                                                    
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/ll/src/lld                                                      
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/ll/src/em                                                       
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/ll/src/llm                                                      
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ble/ll/src/llc                                                      
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/em/api                                                              
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ea/api                                                              
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/hci/api 
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/hci/src                                                             
INCLUDES += -I./beken378/driver/ble/ble_lib/ip/ahi/api                                                             
INCLUDES += -I./beken378/driver/ble/ble_lib/modules/ke/api                                                         
INCLUDES += -I./beken378/driver/ble/ble_lib/modules/ke/src                                                         
INCLUDES += -I./beken378/driver/ble/ble_lib/modules/h4tl/api  
INCLUDES += -I./beken378/func/include
INCLUDES += -I./beken378/func/rf_test
INCLUDES += -I./beken378/func/user_driver
INCLUDES += -I./beken378/func/power_save
INCLUDES += -I./beken378/func/uart_debug
INCLUDES += -I./beken378/func/ethernet_intf
INCLUDES += -I./beken378/func/hostapd-2.5/hostapd
INCLUDES += -I./beken378/func/hostapd-2.5/bk_patch
INCLUDES += -I./beken378/func/hostapd-2.5/src/utils
INCLUDES += -I./beken378/func/hostapd-2.5/src/ap
INCLUDES += -I./beken378/func/hostapd-2.5/src/common
INCLUDES += -I./beken378/func/hostapd-2.5/src/drivers
INCLUDES += -I./beken378/func/hostapd-2.5/src
ifeq ($(CFG_LWIP_2_0_2),1)
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.0.2
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.0.2/src
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.0.2/port
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.0.2/src/include
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.0.2/src/include/netif
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.0.2/src/include/lwip
endif
ifeq ($(CFG_LWIP_2_1_3),1)
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.1.3
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.1.3/src
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.1.3/port
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.1.3/src/include
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.1.3/src/include/netif
INCLUDES += -I./beken378/func/lwip_intf/lwip-2.1.3/src/include/lwip
endif
INCLUDES += -I./beken378/func/temp_detect
INCLUDES += -I./beken378/func/spidma_intf
INCLUDES += -I./beken378/func/rwnx_intf
INCLUDES += -I./beken378/func/joint_up
INCLUDES += -I./beken378/func/tuya_pwm
INCLUDES += -I./beken378/os/include
INCLUDES += -I./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/portable/Keil/ARM968es
INCLUDES += -I./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/include
INCLUDES += -I./beken378/os/FreeRTOSv9.0.0

ifeq ($(CFG_WRAP_LIBC),1)
INCLUDES += -I./beken378/func/libc
endif


# -------------------------------------------------------------------
# Source file list
# -------------------------------------------------------------------
SRC_C =
DRAM_C =
SRC_OS =
SRC_CPP =

#application layer
SRC_C += ./beken378/app/app_bk.c
SRC_C += ./beken378/app/ate_app.c
SRC_C += ./beken378/app/config/param_config.c
SRC_C += ./beken378/app/standalone-ap/sa_ap.c
SRC_C += ./beken378/app/standalone-station/sa_station.c

#demo module
SRC_C += ./beken378/demo/ieee802_11_demo.c

#driver layer
SRC_C += ./beken378/driver/common/dd.c
SRC_C += ./beken378/driver/common/drv_model.c
SRC_C += ./beken378/driver/dma/dma.c
SRC_C += ./beken378/driver/driver.c
SRC_C += ./beken378/driver/entry/arch_main.c
SRC_C += ./beken378/driver/fft/fft.c
SRC_C += ./beken378/driver/flash/flash.c
SRC_C += ./beken378/driver/general_dma/general_dma.c
SRC_C += ./beken378/driver/gpio/gpio.c
SRC_C += ./beken378/driver/i2s/i2s.c
SRC_C += ./beken378/driver/i2c/i2c1.c
SRC_C += ./beken378/driver/i2c/i2c2.c
SRC_C += ./beken378/driver/icu/icu.c
SRC_C += ./beken378/driver/intc/intc.c
SRC_C += ./beken378/driver/irda/irda.c
SRC_C += ./beken378/driver/macphy_bypass/mac_phy_bypass.c
SRC_C += ./beken378/driver/phy/phy_trident.c
SRC_C += ./beken378/driver/pwm/pwm.c
SRC_C += ./beken378/driver/pwm/mcu_ps_timer.c
SRC_C += ./beken378/driver/pwm/bk_timer.c
SRC_C += ./beken378/driver/rw_pub/rw_platf_pub.c
SRC_C += ./beken378/driver/saradc/saradc.c
SRC_C += ./beken378/driver/spi/spi.c
SRC_C += ./beken378/driver/spidma/spidma.c
SRC_C += ./beken378/driver/sys_ctrl/sys_ctrl.c
SRC_C += ./beken378/driver/uart/Retarget.c
SRC_C += ./beken378/driver/uart/uart_bk.c
SRC_C += ./beken378/driver/wdt/wdt.c
# SRC_C += ./beken378/driver/ble/ble.c
# SRC_C += ./beken378/driver/ble/ble_pub/ip/ble/hl/src/prf/prf.c
# SRC_C += ./beken378/driver/ble/ble_pub/ip/ble/profiles/sdp/src/sdp_service.c
# SRC_C += ./beken378/driver/ble/ble_pub/ip/ble/profiles/sdp/src/sdp_service_task.c
# SRC_C += ./beken378/driver/ble/ble_pub/ip/ble/profiles/comm/src/comm.c
# SRC_C += ./beken378/driver/ble/ble_pub/ip/ble/profiles/comm/src/comm_task.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/app/src/app_ble.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/app/src/app_task.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/app/src/app_sdp.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/app/src/app_sec.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/app/src/app_comm.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/common/src/common_list.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/common/src/common_utils.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/common/src/RomCallFlash.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/dbg/src/dbg.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/dbg/src/dbg_mwsgen.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/dbg/src/dbg_swdiag.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/dbg/src/dbg_task.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/rwip/src/rwip.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/rf/src/ble_rf_xvr.c
# SRC_C += ./beken378/driver/ble/ble_pub/modules/ecc_p256/src/ecc_p256.c
# SRC_C += ./beken378/driver/ble/ble_pub/plf/refip/src/driver/uart/uart.c           

#function layer
SRC_C += ./beken378/func/func.c
SRC_C += ./beken378/func/bk7011_cal/bk7231U_cal.c
SRC_C += ./beken378/func/bk7011_cal/manual_cal_bk7231U.c
SRC_C += ./beken378/func/joint_up/role_launch.c
SRC_C += ./beken378/func/hostapd_intf/hostapd_intf.c
SRC_C += ./beken378/func/hostapd-2.5/bk_patch/ddrv.c
SRC_C += ./beken378/func/hostapd-2.5/bk_patch/signal.c
SRC_C += ./beken378/func/hostapd-2.5/bk_patch/sk_intf.c
SRC_C += ./beken378/func/hostapd-2.5/bk_patch/fake_socket.c
SRC_C += ./beken378/func/hostapd-2.5/hostapd/main_none.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/aes-internal.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/aes-internal-dec.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/aes-internal-enc.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/aes-unwrap.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/aes-wrap.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/bk_md5.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/md5-internal.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/rc4.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/bk_sha1.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/sha1-internal.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/sha1-pbkdf2.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/sha1-prf.c
SRC_C += ./beken378/func/hostapd-2.5/src/crypto/tls_none.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ap_config.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ap_drv_ops.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ap_list.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ap_mlme.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/beacon.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/drv_callbacks.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/hostapd.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/hw_features.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ieee802_11_auth.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ieee802_11.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ieee802_11_ht.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ieee802_11_shared.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/ieee802_1x.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/sta_info.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/tkip_countermeasures.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/utils.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/wmm.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/wpa_auth.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/wpa_auth_glue.c
SRC_C += ./beken378/func/hostapd-2.5/src/ap/wpa_auth_ie.c
SRC_C += ./beken378/func/hostapd-2.5/src/common/hw_features_common.c
SRC_C += ./beken378/func/hostapd-2.5/src/common/ieee802_11_common.c
SRC_C += ./beken378/func/hostapd-2.5/src/common/wpa_common.c
SRC_C += ./beken378/func/hostapd-2.5/src/drivers/driver_beken.c
SRC_C += ./beken378/func/hostapd-2.5/src/drivers/driver_common.c
SRC_C += ./beken378/func/hostapd-2.5/src/drivers/drivers.c
SRC_C += ./beken378/func/hostapd-2.5/src/l2_packet/l2_packet_none.c
SRC_C += ./beken378/func/hostapd-2.5/src/rsn_supp/wpa.c
SRC_C += ./beken378/func/hostapd-2.5/src/rsn_supp/wpa_ie.c
SRC_C += ./beken378/func/hostapd-2.5/src/utils/common.c
SRC_C += ./beken378/func/hostapd-2.5/src/utils/eloop.c
SRC_C += ./beken378/func/hostapd-2.5/src/utils/os_none.c
SRC_C += ./beken378/func/hostapd-2.5/src/utils/wpabuf.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/blacklist.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/bss.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/config.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/config_none.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/events.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/main_supplicant.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/notify.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/wmm_ac.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/wpa_scan.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/wpas_glue.c
SRC_C += ./beken378/func/hostapd-2.5/wpa_supplicant/wpa_supplicant.c

ifeq ($(CFG_WRAP_LIBC),1)
SRC_C += ./beken378/func/libc/stdio/lib_libvscanf.c
SRC_C += ./beken378/func/libc/stdio/lib_memsistream.c
SRC_C += ./beken378/func/libc/stdio/lib_meminstream.c
SRC_C += ./beken378/func/libc/stdio/lib_sscanf.c
SRC_C += ./beken378/func/libc/stdio/lib_vsscanf.c
SRC_C += ./beken378/func/libc/stdlib/lib_strtod.c
SRC_C += ./beken378/func/libc/stdlib/lib_qsort.c
endif

ifeq ($(CFG_LWIP_2_0_2),1)
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/port/ethernetif.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/port/net.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/port/sys_arch.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/api/api_lib.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/api/api_msg.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/api/err.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/api/netbuf.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/api/netdb.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/api/netifapi.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/api/sockets.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/api/tcpip.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/def.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/dns.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/inet_chksum.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/init.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ip.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv4/autoip.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv4/dhcp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv4/etharp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv4/icmp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv4/igmp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv4/ip4_addr.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv4/ip4.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv4/ip4_frag.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/dhcp6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/ethip6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/icmp6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/inet6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/ip6_addr.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/ip6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/ip6_frag.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/mld6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/ipv6/nd6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/mem.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/memp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/netif.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/pbuf.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/raw.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/stats.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/sys.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/tcp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/tcp_in.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/tcp_out.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/timeouts.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/core/udp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/netif/ethernet.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/apps/httpd/httpd.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/apps/httpd/fs.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.0.2/src/apps/mqtt/mqtt.c
endif

ifeq ($(CFG_LWIP_2_1_3),1)
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/port/ethernetif.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/port/net.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/port/sys_arch.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/api/api_lib.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/api/api_msg.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/api/err.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/api/netbuf.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/api/netdb.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/api/netifapi.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/api/sockets.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/api/tcpip.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/altcp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/altcp_alloc.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/altcp_tcp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/def.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/dns.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/inet_chksum.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/init.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ip.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ip_a.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv4/autoip.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv4/dhcp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv4/etharp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv4/icmp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv4/igmp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv4/ip4_addr.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv4/ip4.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv4/ip4_frag.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/dhcp6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/ethip6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/icmp6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/inet6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/ip6_addr.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/ip6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/ip6_frag.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/mld6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/ipv6/nd6.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/mem.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/memp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/netif.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/pbuf.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/raw.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/stats.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/sys.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/tcp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/tcp_in.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/tcp_out.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/timeouts.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/core/udp.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/netif/ethernet.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/apps/http/httpd.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/apps/http/fs.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/apps/mqtt/mqtt.c
endif

SRC_C += ./beken378/func/lwip_intf/dhcpd/dhcp-server.c
SRC_C += ./beken378/func/lwip_intf/dhcpd/dhcp-server-main.c
SRC_C += ./beken378/func/misc/fake_clock.c
SRC_C += ./beken378/func/misc/target_util.c
SRC_C += ./beken378/func/net_param_intf/net_param.c
SRC_C += ./beken378/func/key/multi_button.c
SRC_C += ./beken378/func/key/key_main.c
SRC_C += ./beken378/func/misc/start_type.c
SRC_C += ./beken378/func/power_save/power_save.c
SRC_C += ./beken378/func/power_save/manual_ps.c
SRC_C += ./beken378/func/power_save/mcu_ps.c
SRC_C += ./beken378/func/rf_test/rx_sensitivity.c
SRC_C += ./beken378/func/rf_test/tx_evm.c
SRC_C += ./beken378/func/rwnx_intf/rw_ieee80211.c
SRC_C += ./beken378/func/rwnx_intf/rw_msdu.c
SRC_C += ./beken378/func/rwnx_intf/rw_msg_rx.c
SRC_C += ./beken378/func/rwnx_intf/rw_msg_tx.c
SRC_C += ./beken378/func/sim_uart/gpio_uart.c
SRC_C += ./beken378/func/sim_uart/pwm_uart.c
SRC_C += ./beken378/func/spidma_intf/spidma_intf.c
SRC_C += ./beken378/func/temp_detect/temp_detect.c
SRC_C += ./beken378/func/uart_debug/cmd_evm.c
SRC_C += ./beken378/func/uart_debug/cmd_help.c
SRC_C += ./beken378/func/uart_debug/cmd_reg.c
SRC_C += ./beken378/func/uart_debug/cmd_rx_sensitivity.c
SRC_C += ./beken378/func/uart_debug/command_line.c
SRC_C += ./beken378/func/uart_debug/command_table.c
SRC_C += ./beken378/func/uart_debug/udebug.c
SRC_C += ./beken378/func/user_driver/BkDriverFlash.c
SRC_C += ./beken378/func/user_driver/BkDriverRng.c
SRC_C += ./beken378/func/user_driver/BkDriverGpio.c
SRC_C += ./beken378/func/user_driver/BkDriverPwm.c
SRC_C += ./beken378/func/user_driver/BkDriverUart.c
SRC_C += ./beken378/func/user_driver/BkDriverWdg.c
SRC_C += ./beken378/func/user_driver/BkDriverTimer.c
SRC_C += ./beken378/func/wlan_ui/wlan_cli.c
SRC_C += ./beken378/func/wlan_ui/wlan_ui.c
SRC_C += ./beken378/func/tuya_pwm/tuya_pwm.c

#rwnx ip module
#SRC_C += ./beken378/ip/common/co_dlist.c
#SRC_C += ./beken378/ip/common/co_list.c
#SRC_C += ./beken378/ip/common/co_math.c
#SRC_C += ./beken378/ip/common/co_pool.c
#SRC_C += ./beken378/ip/common/co_ring.c
#SRC_C += ./beken378/ip/ke/ke_env.c
#SRC_C += ./beken378/ip/ke/ke_event.c
#SRC_C += ./beken378/ip/ke/ke_msg.c
#SRC_C += ./beken378/ip/ke/ke_queue.c
#SRC_C += ./beken378/ip/ke/ke_task.c
#SRC_C += ./beken378/ip/ke/ke_timer.c
#SRC_C += ./beken378/ip/lmac/src/chan/chan.c
#SRC_C += ./beken378/ip/lmac/src/hal/hal_desc.c
#SRC_C += ./beken378/ip/lmac/src/hal/hal_dma.c
#SRC_C += ./beken378/ip/lmac/src/hal/hal_machw.c
#SRC_C += ./beken378/ip/lmac/src/hal/hal_mib.c
#SRC_C += ./beken378/ip/lmac/src/mm/mm.c
#SRC_C += ./beken378/ip/lmac/src/mm/mm_bcn.c
#SRC_C += ./beken378/ip/lmac/src/mm/mm_task.c
#SRC_C += ./beken378/ip/lmac/src/mm/mm_timer.c
#SRC_C += ./beken378/ip/lmac/src/p2p/p2p.c
#SRC_C += ./beken378/ip/lmac/src/ps/ps.c
#SRC_C += ./beken378/ip/lmac/src/rd/rd.c
#SRC_C += ./beken378/ip/lmac/src/rwnx/rwnx.c
#SRC_C += ./beken378/ip/lmac/src/rx/rx_swdesc.c
#SRC_C += ./beken378/ip/lmac/src/rx/rxl/rxl_cntrl.c
#SRC_C += ./beken378/ip/lmac/src/rx/rxl/rxl_hwdesc.c
#SRC_C += ./beken378/ip/lmac/src/scan/scan.c
#SRC_C += ./beken378/ip/lmac/src/scan/scan_shared.c
#SRC_C += ./beken378/ip/lmac/src/scan/scan_task.c
#SRC_C += ./beken378/ip/lmac/src/sta/sta_mgmt.c
#SRC_C += ./beken378/ip/lmac/src/td/td.c
#SRC_C += ./beken378/ip/lmac/src/tdls/tdls.c
#SRC_C += ./beken378/ip/lmac/src/tdls/tdls_task.c
#SRC_C += ./beken378/ip/lmac/src/tpc/tpc.c
#SRC_C += ./beken378/ip/lmac/src/tx/tx_swdesc.c
#SRC_C += ./beken378/ip/lmac/src/tx/txl/txl_buffer.c
#SRC_C += ./beken378/ip/lmac/src/tx/txl/txl_buffer_shared.c
#SRC_C += ./beken378/ip/lmac/src/tx/txl/txl_cfm.c
#SRC_C += ./beken378/ip/lmac/src/tx/txl/txl_cntrl.c
#SRC_C += ./beken378/ip/lmac/src/tx/txl/txl_frame.c
#SRC_C += ./beken378/ip/lmac/src/tx/txl/txl_frame_shared.c
#SRC_C += ./beken378/ip/lmac/src/tx/txl/txl_hwdesc.c
#SRC_C += ./beken378/ip/lmac/src/vif/vif_mgmt.c
#SRC_C += ./beken378/ip/mac/mac.c
#SRC_C += ./beken378/ip/mac/mac_ie.c
#SRC_C += ./beken378/ip/umac/src/apm/apm.c
#SRC_C += ./beken378/ip/umac/src/apm/apm_task.c
#SRC_C += ./beken378/ip/umac/src/bam/bam.c
#SRC_C += ./beken378/ip/umac/src/bam/bam_task.c
#SRC_C += ./beken378/ip/umac/src/me/me.c
#SRC_C += ./beken378/ip/umac/src/me/me_mgmtframe.c
#SRC_C += ./beken378/ip/umac/src/me/me_mic.c
#SRC_C += ./beken378/ip/umac/src/me/me_task.c
#SRC_C += ./beken378/ip/umac/src/me/me_utils.c
#SRC_C += ./beken378/ip/umac/src/rc/rc.c
#SRC_C += ./beken378/ip/umac/src/rc/rc_basic.c
#SRC_C += ./beken378/ip/umac/src/rxu/rxu_cntrl.c
#SRC_C += ./beken378/ip/umac/src/scanu/scanu.c
#SRC_C += ./beken378/ip/umac/src/scanu/scanu_shared.c
#SRC_C += ./beken378/ip/umac/src/scanu/scanu_task.c
#SRC_C += ./beken378/ip/umac/src/sm/sm.c
#SRC_C += ./beken378/ip/umac/src/sm/sm_task.c
#SRC_C += ./beken378/ip/umac/src/txu/txu_cntrl.c 

#ble lib
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ahi/src/ahi.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ahi/src/ahi_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapc/gapc.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapc/gapc_hci.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapc/gapc_sig.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapc/gapc_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapm/gapm.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapm/gapm_hci.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapm/gapm_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/gapm/gapm_util.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/smpc/smpc.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/smpc/smpc_api.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/smpc/smpc_crypto.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/smpc/smpc_util.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gap/smpm/smpm_api.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/attc/attc.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/attm/attm.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/attm/attm_db.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/atts/atts.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/gattc/gattc.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/gattc/gattc_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/gattm/gattm.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/gatt/gattm/gattm_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/l2c/l2cc/l2cc.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/l2c/l2cc/l2cc_lecb.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/l2c/l2cc/l2cc_pdu.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/l2c/l2cc/l2cc_sig.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/l2c/l2cc/l2cc_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/l2c/l2cm/l2cm.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/prf/prf_utils.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/hl/src/rwble_hl/rwble_hl.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/em/em_buf.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llc/llc.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llc/llc_ch_asses.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llc/llc_hci.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llc/llc_llcp.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llc/llc_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llc/llc_util.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/lld/lld.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/lld/lld_evt.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/lld/lld_pdu.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/lld/lld_sleep.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/lld/lld_util.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/lld/lld_wlcoex.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llm/llm.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llm/llm_hci.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llm/llm_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/llm/llm_util.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ble/ll/src/rwble/rwble.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/ea/src/ea.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/hci/src/hci.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/hci/src/hci_fc.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/hci/src/hci_msg.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/hci/src/hci_tl.c 
#SRC_C += ./beken378/driver/ble/ble_lib/ip/hci/src/hci_util.c 
#SRC_C += ./beken378/driver/ble/ble_lib/modules/h4tl/src/h4tl.c 
#SRC_C += ./beken378/driver/ble/ble_lib/modules/ke/src/kernel.c 
#SRC_C += ./beken378/driver/ble/ble_lib/modules/ke/src/kernel_event.c 
#SRC_C += ./beken378/driver/ble/ble_lib/modules/ke/src/kernel_mem.c 
#SRC_C += ./beken378/driver/ble/ble_lib/modules/ke/src/kernel_msg.c 
#SRC_C += ./beken378/driver/ble/ble_lib/modules/ke/src/kernel_queue.c 
#SRC_C += ./beken378/driver/ble/ble_lib/modules/ke/src/kernel_task.c 
#SRC_C += ./beken378/driver/ble/ble_lib/modules/ke/src/kernel_timer.c 
#SRC_C += ./beken378/driver/ble/ble_lib/plf/refip/src/arch/main/ble_arch_main.c

#operation system module
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/croutine.c
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/event_groups.c
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/list.c
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/portable/Keil/ARM968es/port.c
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/portable/MemMang/heap_4.c
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/queue.c
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/tasks.c
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/timers.c
SRC_OS += ./beken378/os/FreeRTOSv9.0.0/rtos_pub.c
SRC_C += ./beken378/os/mem_arch.c
SRC_C += ./beken378/os/platform_stub.c
SRC_C += ./beken378/os/str_arch.c

#examples for customer

#assembling files
SRC_S = 
SRC_S +=  ./beken378/driver/entry/boot_handlers.S
SRC_S +=  ./beken378/driver/entry/boot_vectors.S

# Generate obj list
# -------------------------------------------------------------------
SRC_O = $(patsubst %.c,%.o,$(SRC_C))
SRC_C_LIST = $(notdir $(SRC_C)) $(notdir $(DRAM_C))
OBJ_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SRC_C_LIST)))
DEPENDENCY_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.d,$(SRC_C_LIST)))

SRC_S_O = $(patsubst %.S,%.o,$(SRC_S))
SRC_S_LIST = $(notdir $(SRC_S)) 
OBJ_S_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.S,%.o,$(SRC_S_LIST)))
DEPENDENCY_S_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.S,%.d,$(SRC_S_LIST)))

SRC_OS_O = $(patsubst %.c,%.o,$(SRC_OS))
SRC_OS_LIST = $(notdir $(SRC_OS)) 
OBJ_OS_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SRC_OS_LIST)))
DEPENDENCY_OS_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.d,$(SRC_OS_LIST)))

SRC_CPP_O = $(patsubst %.cpp,%.o,$(SRC_CPP))
SRC_CPP_LIST = $(notdir $(SRC_CPP)) 
OBJ_CPP_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.cpp,%.o,$(SRC_CPP_LIST)))
DEPENDENCY_CPP_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.cpp,%.d,$(SRC_CPP_LIST)))

# Compile options
# -------------------------------------------------------------------
CPPDEFINES = 
CPPDEFINES += -DPLATFORM_BK7231T=1
CPPDEFINES += -DPLATFORM_BEKEN=1

CCFLAGS = $(CPPDEFINES)
CCFLAGS += -g0 -mthumb -mcpu=arm968e-s -march=armv5te -mthumb-interwork -mlittle-endian -Os
CCFLAGS += -ffunction-sections -fsigned-char -fdata-sections -Wno-unused-function -Wno-unused-but-set-variable

CXXFLAGS = $(CCFLAGS)
CXXFLAGS += -std=gnu++11 -MMD -fno-exceptions -fno-rtti -Wno-literal-suffix -Wno-attributes

CFLAGS = $(CCFLAGS)
CFLAGS += -std=c99 -Wunknown-pragmas -nostdlib -Wall

OSFLAGS = -flto
OSFLAGS += -g0 -marm -mcpu=arm968e-s -march=armv5te -mthumb-interwork -mlittle-endian -Os -std=c99 -ffunction-sections -Wall -fsigned-char -fdata-sections -Wunknown-pragmas

ASMFLAGS = 
ASMFLAGS += -g0 -marm -mthumb-interwork -mcpu=arm968e-s -march=armv5te -x assembler-with-cpp

LFLAGS = -flto
LFLAGS += -g0 -Wl,--gc-sections -marm -mcpu=arm968e-s -mthumb-interwork 
# LFLAGS += -nostdlib
LFLAGS += -Xlinker -Map=tuya.map  
LFLAGS += -Wl,-wrap,malloc -Wl,-wrap,_malloc_r -Wl,-wrap,free -Wl,-wrap,_free_r -Wl,-wrap,zalloc -Wl,-wrap,calloc -Wl,-wrap,realloc  -Wl,-wrap,_realloc_r

# uncomment for malloc debug and protection
# CFLAGS += -DOBK_HEAPGUARD=1
# LFLAGS += -Wl,-wrap,pvPortMalloc -Wl,-wrap,vPortFree
# LFLAGS += -Wl,-wrap,pvPortRealloc

CFLAGS += -DWRAP_PRINTF=1
LFLAGS += -Wl,-wrap,vsnprintf
LFLAGS += -Wl,-wrap,snprintf
LFLAGS += -Wl,-wrap,sprintf
LFLAGS += -Wl,-wrap,vsprintf


# ??
#LFLAGS += --specs=nano.specs

# stdlib wrapper
ifeq ($(CFG_WRAP_LIBC),1)
LFLAGS += -Wl,-wrap,strtod -Wl,-wrap,qsort
LFLAGS += -Wl,-wrap,sscanf
endif

LIBFLAGS =
LIBFLAGS += -L./beken378/lib/ -lrwnx
LIBFLAGS += -L./beken378/lib/ -lble
LIBFLAGS += -lstdc++

CFLAGS += -DWRAP_PRINTF=1
LFLAGS += -Wl,-wrap,vsnprintf
LFLAGS += -Wl,-wrap,snprintf
LFLAGS += -Wl,-wrap,sprintf
LFLAGS += -Wl,-wrap,vsprintf

# Compile
# -------------------------------------------------------------------
# add tuya iot lib compile support
# -------------------------------------------------------------------

# LIBFLAGS += -L $(TOP_DIR)/sdk/lib/ -ltuya_iot
CFLAGS += -DUSER_SW_VER=\"$(USER_SW_VER)\" -DAPP_BIN_NAME=\"$(APP_BIN_NAME)\" -DBUILD_NUMBER=$(BUILD_NUMBER)


# -------------------------------------------------------------------
# add tuya application components.mk
# -------------------------------------------------------------------

APP_MK_NAME := $(TOP_DIR)/apps/$(APP_BIN_NAME)/components.mk
ifeq ($(APP_MK_NAME), $(wildcard $(APP_MK_NAME)))
sinclude $(APP_MK_NAME)
TY_SRC_DIRS += $(foreach n, $(COMPONENTS), $(shell find $(TOP_DIR)/components/$(n)/src -type d))
TY_INC_DIRS += $(foreach n, $(COMPONENTS), $(shell find $(TOP_DIR)/components/$(n)/include -type d))
endif

# -------------------------------------------------------------------
# add tuya iot application compile support
# -------------------------------------------------------------------
TY_OUTPUT = $(TOP_DIR)/apps/$(APP_BIN_NAME)/output/$(APP_VERSION)

TY_SRC_DIRS += $(shell find ../tuya_common/src -type d)
TY_SRC_DIRS += $(shell find $(TOP_DIR)/apps/$(APP_BIN_NAME)/src -type d)
TY_SRC_DIRS += $(shell find ../tuya_os_adapter/src -type d)

SRC_C += $(foreach dir, $(TY_SRC_DIRS), $(wildcard $(dir)/*.c)) # need export
SRC_C += $(foreach dir, $(TY_SRC_DIRS), $(wildcard $(dir)/*.s)) 
SRC_C += $(foreach dir, $(TY_SRC_DIRS), $(wildcard $(dir)/*.S)) 
SRC_CPP += $(foreach dir, $(TY_SRC_DIRS), $(wildcard $(dir)/*.cpp)) 

#TY_INC_DIRS += $(shell find $(TOP_DIR)/sdk -type d)
SDK_INCLUDE_DIRS := $(shell find $(TOP_DIR)/sdk -name include -type d)
TY_INC_DIRS += $(foreach dir,$(SDK_INCLUDE_DIRS),$(shell find $(dir) -type d))

TY_INC_DIRS += $(shell find ../tuya_os_adapter/include -type d)
TY_INC_DIRS += $(shell find ../tuya_common/include -type d)
TY_INC_DIRS += $(shell find $(TOP_DIR)/apps/$(APP_BIN_NAME)/include -type d)

INCLUDES += $(foreach base_dir, $(TY_INC_DIRS), $(addprefix -I , $(base_dir))) 

$(TY_OBJS): %.o : %.c
	echo "build $@ ..."
	@$(CC) $(CFLAGS) $(TY_IOT_CFLAGS) $(INCLUDES) -c $< -o $@
	@$(CC) $(CFLAGS) $(TY_IOT_CFLAGS) $(INCLUDES) -c $< -MM -MT $@ -MF $(OBJ_DIR)/$(notdir $(patsubst %.o,%.d,$@))
	@cp $@ $(OBJ_DIR)/$(notdir $@)
	@chmod 777 $(OBJ_DIR)/$(notdir $@)

sinclude $(TY_DEPENDENCY_LIST)

CUR_PATH = $(shell pwd)	
.PHONY: application
application: prerequirement $(SRC_O) $(SRC_S_O) $(SRC_OS_O) $(SRC_CPP_O) $(TY_IOT_LIB)
ifeq ("${ota_idx}", "1")
	$(LD) $(LFLAGS) -o $(TY_OUTPUT)/$(APP_BIN_NAME)_$(APP_VERSION).axf  $(OBJ_LIST) $(OBJ_S_LIST) $(OBJ_OS_LIST) $(OBJ_CPP_LIST) $(LIBFLAGS) -T./beken378/build/bk7231_ota.ld
else ifeq ("${ota_idx}", "2")
else
	@echo ===========================================================
	@echo ota_idx must be "1" or "2"
	@echo ===========================================================
endif

	$(NM) $(TY_OUTPUT)/$(APP_BIN_NAME)_$(APP_VERSION).axf | sort > $(TY_OUTPUT)/$(APP_BIN_NAME)_$(APP_VERSION).map
	$(OBJDUMP) -d $(TY_OUTPUT)/$(APP_BIN_NAME)_$(APP_VERSION).axf > $(TY_OUTPUT)/$(APP_BIN_NAME)_$(APP_VERSION).asm
	$(OBJCOPY) -O binary $(TY_OUTPUT)/$(APP_BIN_NAME)_$(APP_VERSION).axf $(TY_OUTPUT)/$(APP_BIN_NAME)_$(APP_VERSION).bin
# Generate build info
# -------------------------------------------------------------------	

.PHONY: prerequirement
prerequirement:
	echo prerequirement0
	@echo ===========================================================
	@echo Build $(APP_BIN_NAME)
	@echo ===========================================================
	echo prerequirement1
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)
	echo prerequirement2
	
	@# add tuya bin output
	@mkdir -p $(TY_OUTPUT)

$(SRC_O): %.o : %.c
	@ echo "build $@"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -MM -MT $@ -MF $(OBJ_DIR)/$(notdir $(patsubst %.o,%.d,$@))
	@cp $@ $(OBJ_DIR)/$(notdir $@)
	@chmod 777 $(OBJ_DIR)/$(notdir $@)

$(SRC_S_O): %.o : %.S
	@ echo "build $@"
	@$(CC) $(ASMFLAGS) $(INCLUDES) -c $< -o $@
	@$(CC) $(ASMFLAGS) $(INCLUDES) -c $< -MM -MT $@ -MF $(OBJ_DIR)/$(notdir $(patsubst %.o,%.d,$@))
	@cp $@ $(OBJ_DIR)/$(notdir $@)
	@chmod 777 $(OBJ_DIR)/$(notdir $@)

$(SRC_OS_O): %.o : %.c
	@ echo "build $@"
	@$(CC) $(OSFLAGS) $(INCLUDES) -c $< -o $@
	@$(CC) $(OSFLAGS) $(INCLUDES) -c $< -MM -MT $@ -MF $(OBJ_DIR)/$(notdir $(patsubst %.o,%.d,$@))
	@cp $@ $(OBJ_DIR)/$(notdir $@)
	@chmod 777 $(OBJ_DIR)/$(notdir $@)

$(SRC_CPP_O): %.o : %.cpp
	@ echo "build $@"
	@$(CC) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@$(CC) $(CXXFLAGS) $(INCLUDES) -c $< -MM -MT $@ -MF $(OBJ_DIR)/$(notdir $(patsubst %.o,%.d,$@))
	@cp $@ $(OBJ_DIR)/$(notdir $@)
	@chmod 777 $(OBJ_DIR)/$(notdir $@)

-include $(DEPENDENCY_LIST)
-include $(DEPENDENCY_S_LIST)
-include $(DEPENDENCY_OS_LIST)
-include $(DEPENDENCY_CPP_LIST)

# -------------------------------------------------------------------	
# Generate build info
# -------------------------------------------------------------------	
.PHONY: setup
setup:
	@echo "----------------"
	@echo Setup $(GDB_SERVER)
	@echo "----------------"
	
.PHONY: debug
debug:
	@if [ ! -f $(FLASH_TOOLDIR)/rtl_gdb_debug.txt ] ; then echo Please do \"make setup GDB_SERVER=[jlink or openocd]\" first; echo && false ; fi
ifeq ($(findstring CYGWIN, $(OS)), CYGWIN) 
	$(FLASH_TOOLDIR)/Check_Jtag.sh
	cmd /c start $(GDB) -x $(FLASH_TOOLDIR)/rtl_gdb_debug.txt 
else
	$(GDB) -x $(FLASH_TOOLDIR)/rtl_gdb_debug.txt
endif

.PHONY: clean
clean:
	rm -rf $(TARGET)
	rm -f $(SRC_O)
	rm -f $(SRC_S_O)
	rm -f $(SRC_OS_O)
	rm -f $(SRC_CPP_O)
	rm -rf $(TY_OBJS)
	rm -f $(TY_IOT_LIB)
	rm -rf $(TY_OUTPUT)
	
