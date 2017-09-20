include $(TOPDIR)/rules.mk

PKG_NAME:=bear
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/bear
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libpthread +alsa-lib +zlib + libid3tag +libmad
  TITLE:=Bear -------- ken used as a platform
endef

define Package/bear/description
    It linux play sound platform .....................
endef

define Build/Prepare
	echo "Here is Package/Prepare"
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += $(FPIC)
TARGET_CFLAGS += -I$(PKG_BUILD_DIR)/include
TARGET_LDFLAGS += -L$(PKG_BUILD_DIR)/lib


LIBS:= -lpthread -lasound -lmad -lid3tag -lz -lmp3lame

MAKE_FLAGS += \
        CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)" \
        LDFLAGS="$(TARGET_LDFLAGS) $(LIBS)"


define Package/bear/install
	echo "Here is Package/install"
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bear $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/app_user $(1)/bin/
	$(INSTALL_DIR) $(1)/usr
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP)  $(PKG_BUILD_DIR)/lib/libmp3lame.so               $(1)/usr/lib/
	$(CP)  $(PKG_BUILD_DIR)/lib/libmp3lame.so.*             $(1)/usr/lib/
endef
    
$(eval $(call BuildPackage,bear))
