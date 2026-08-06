include $(TOPDIR)/rules.mk

PKG_NAME:=esp_ubus
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/esp_ubus
	DEPENDS:=+libubus +libubox +libblobmsg-json +libserialport
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=esp_ubus
endef

define Package/esp_ubus/Description
	UBUS module for controlling an ESP micro controller
endef

define Package/esp_ubus/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/esp_ubus $(1)/usr/bin/esp_ubus
	$(INSTALL_BIN) ./files/esp_ubus.init $(1)/etc/init.d/esp_ubus
endef

$(eval $(call BuildPackage,esp_ubus))
