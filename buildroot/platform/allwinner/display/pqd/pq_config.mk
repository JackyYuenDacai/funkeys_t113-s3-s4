PQ_ROOT := $(shell dirname $(lastword $(MAKEFILE_LIST)))
PRODUCT_COPY_FILES += $(PQ_ROOT)/sunxi_pqdata:$(TARGET_COPY_OUT_VENDOR)/etc/sunxi_pqdata

