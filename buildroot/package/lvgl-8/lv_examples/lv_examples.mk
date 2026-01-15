LV_EXAMPLES_SITE_METHOD = local
LV_EXAMPLES_SITE = platform/thirdparty/gui/lvgl-8/lv_examples
LV_EXAMPLES_LICENSE = GPLv2+, GPLv3+
LV_EXAMPLES_LICENSE_FILES = Copyright COPYING
LV_EXAMPLES_DEPENDENCIES += $(if $(LVGL8_USE_SUNXIFB_G2D),libuapi,)



LV_EXAMPLES_CFLAGS+=-I$(@D)/src
define LV_EXAMPLES_BUILD_CMDS
	cp -r $(LV_EXAMPLES_SITE)/../lvgl $(@D)/src/
	cp -r $(LV_EXAMPLES_SITE)/../lv_drivers $(@D)/src/
	cp -r $(LV_EXAMPLES_SITE)/../lvgl/demos $(@D)/src
	make  -C $(@D)/src\
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(LVGL_CFLAGS) $(LV_EXAMPLES_CFLAGS)" \
		LDFLAGS="$(LVGL_LDFLAGS)" \
		all

endef

define LV_EXAMPLES_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/src/lv_examples $(TARGET_DIR)/usr/bin/lv_examples
endef

$(eval $(generic-package))
