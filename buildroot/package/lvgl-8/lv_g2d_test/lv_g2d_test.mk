LV_G2D_TEST_SITE_METHOD = local
LV_G2D_TEST_SITE = ../../platform/thirdparty/gui/lvgl-8/lv_g2d_test
LV_G2D_TEST_LICENSE = GPLv2+, GPLv3+
LV_G2D_TEST_LICENSE_FILES = Copyright COPYING
LV_G2D_TEST_DEPENDENCIES += $(if $(LVGL8_USE_SUNXIFB_G2D),libuapi,)


ifeq ($(LVGL8_USE_SUNXIFB_G2D),y)
LV_G2D_TEST_CFLAGS+=-DLV_USE_SUNXIFB_G2D_FILL \
				-DLV_USE_SUNXIFB_G2D_BLEND \
				-DLV_USE_SUNXIFB_G2D_BLIT \
				-DLV_USE_SUNXIFB_G2D_SCALE
endif

LV_G2D_TEST_CFLAGS+=-I$(@D)/src
define LV_G2D_TEST_BUILD_CMDS
	cp -r $(LV_G2D_TEST_SITE)/../lvgl $(@D)/src/
	cp -r $(LV_G2D_TEST_SITE)/res $(@D)/
	cp -r $(LV_G2D_TEST_SITE)/../lv_drivers $(@D)/src/
	cp -r $(LV_G2D_TEST_SITE)/../lv_demos $(@D)/src
	make  -C $(@D)/src\
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(LVGL_CFLAGS) $(LV_G2D_TEST_CFLAGS)" \
		LDFLAGS="$(LVGL_LDFLAGS)" \
		all

endef

define LV_G2D_TEST_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/src/lv_g2d_test $(TARGET_DIR)/usr/bin/lv_g2d_test
	$(INSTALL) -d -m 0755 $(TARGET_DIR)/usr/share/lv_g2d_test
	$(INSTALL) -D -m 0755 $(@D)/res/* $(TARGET_DIR)/usr/share/lv_g2d_test
endef

$(eval $(generic-package))
