################################################################################
# steward-nds (FunKey S version) package makefile
################################################################################

STEWARD_NDS_VERSION = v1-funkey
STEWARD_NDS_SITE_METHOD = local
STEWARD_NDS_SITE = /home/jackyyuen/Codes/steward-nds
STEWARD_NDS_LICENSE = LGPL-2.1
STEWARD_NDS_LICENSE_FILES = LICENSE

STEWARD_NDS_DEPENDENCIES = sdl sdl_image sdl_ttf host-squashfs libpng zlib alsa-lib

# Define package directory for OPK creation
STEWARD_NDS_PKGDIR = $(dir $(lastword $(MAKEFILE_LIST)))

# Use host SDL config (available during build)
STEWARD_NDS_SDL_CFLAGS += $(shell $(HOST_DIR)/usr/bin/sdl-config --cflags)
STEWARD_NDS_SDL_LIBS   += $(shell $(HOST_DIR)/usr/bin/sdl-config --libs)

STEWARD_NDS_CFLAGS += $(STEWARD_NDS_SDL_CFLAGS)
STEWARD_NDS_CFLAGS += -DFUNKEY_S -Ofast -DNDEBUG
STEWARD_NDS_CFLAGS += -Wall -fdata-sections -ffunction-sections -flto
STEWARD_NDS_CFLAGS += -I./ -I./libretro-common/include/

STEWARD_NDS_LIBS += $(STEWARD_NDS_SDL_LIBS)
STEWARD_NDS_LIBS += -lSDL -lSDL_image -lSDL_ttf -lasound -lpng -lz -lm -ldl -Wl,--gc-sections -flto

define STEWARD_NDS_BUILD_CMDS
    $(MAKE) 
endef

 

define STEWARD_NDS_INSTALL_TARGET_CMDS
    $(INSTALL) -d -m 0755 $(TARGET_DIR)/usr/games
    $(INSTALL) -m 0755 $(@D)/picoarch $(TARGET_DIR)/usr/games/
endef

define STEWARD_NDS_CREATE_OPK
    $(INSTALL) -d -m 0755 $(TARGET_DIR)/usr/local/share/OPKs/Libretro
    $(HOST_DIR)/usr/bin/mksquashfs $(STEWARD_NDS_PKGDIR)/opk \
        $(TARGET_DIR)/usr/local/share/OPKs/Libretro/steward-nds.opk \
        -all-root -noappend -no-exports -no-xattrs
endef
STEWARD_NDS_POST_INSTALL_TARGET_HOOKS += STEWARD_NDS_CREATE_OPK

$(eval $(generic-package))