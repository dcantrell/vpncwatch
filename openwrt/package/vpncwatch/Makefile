include $(TOPDIR)/rules.mk

PKG_NAME:=vpncwatch
PKG_VERSION:=1.2
PKG_RELEASE:=1
PKG_MD5SUM:=34b63b8b0f0a31177af7cdc98a700a0c

PKG_SOURCE_URL:=http://dcantrel.fedorapeople.org/vpncwatch
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(TOPDIR)/package/rules.mk

$(eval $(call PKG_template,VPNCWATCH,vpncwatch,$(PKG_VERSION)-$(PKG_RELEASE),$(ARCH)))

$(PKG_BUILD_DIR)/.configured:
	touch $@

$(PKG_BUILD_DIR)/.built:
	$(MAKE) -C $(PKG_BUILD_DIR) CC=$(TARGET_CC)
	touch $@

$(IPKG_VPNCWATCH):
	mkdir -p $(IDIR_VPNCWATCH)/usr/sbin
	cp $(PKG_BUILD_DIR)/$(PKG_NAME) $(IDIR_VPNCWATCH)/usr/sbin/
	$(STRIP) $(IDIR_VPNCWATCH)/usr/sbin/*
	$(IPKG_BUILD) $(IDIR_VPNCWATCH) $(PACKAGE_DIR)

mostlyclean:
	$(MAKE) -C $(PKG_BUILD_DIR) clean
	rm -f $(PKG_BUILD_DIR)/.built
