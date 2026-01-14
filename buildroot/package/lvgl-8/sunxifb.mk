#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ifeq ($(LVGL8_USE_SUNXIFB_DOUBLE_BUFFER),y)
LVGL_CFLAGS+=-DUSE_SUNXIFB_DOUBLE_BUFFER
endif

ifeq ($(LVGL8_USE_SUNXIFB_CACHE),y)
LVGL_CFLAGS+=-DUSE_SUNXIFB_CACHE
endif

ifeq ($(LVGL8_USE_SUNXIFB_G2D),y)
LVGL_CFLAGS+=-DUSE_SUNXIFB_G2D
LVGL_LDFLAGS+=-luapi
endif

ifeq ($(LVGL8_USE_SUNXIFB_G2D_ROTATE),y)
LVGL_CFLAGS+=-DUSE_SUNXIFB_G2D_ROTATE
endif

ifeq ($(LICHEE_KERN_VER),linux-5.4)
LVGL_CFLAGS+=-DCONF_G2D_VERSION_NEW
endif

ifeq ($(LVGL8_USE_FREETYPE),y)
LVGL_CFLAGS+=-I$(STAGING_DIR)/usr/include/freetype2
LVGL_LDFLAGS+=-lfreetype
endif
