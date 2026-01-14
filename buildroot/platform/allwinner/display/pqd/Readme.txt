
                              ____________

                               PQD README
                              ____________


Table of Contents
_________________

1. How to build PQD
.. 1. build with cmake
.. 2. build with gnu make
2. pq config file


1 How to build PQD
==================

  pqd supports two compilation methods, you can choose cmake or gnu make !


1.1 build with cmake
~~~~~~~~~~~~~~~~~~~

  ,----
  |
  | export CROSS_COMPILE="/path/to/arm-linux-gnueabi-"
  | make -p build && cd build
  | cmake ../
  | make
  |
  `----


1.2 build with gnu make
~~~~~~~~~~~~~~~~~~~~~~

  ,----
  |
  | export CROSS_COMPILE="/path/to/arm-linux-gnueabi-"
  | make
  |
  `----


2 pq config file
================

  the pqdata will store in '/etc/.sunxi_pqdata.bin' default,
  but you can change this default path by modify PQDATA_FILE_NAME in pqdata/config.h



3 use pqd on Android

3.1 please use Android.bp for building:
    cflag must define ANDROID_PLT;
    pq config file must rename , such as sunxi_pqdata, because name string format
    which is .sunxi_pqdata.bin is not allowed by android build system.
    change PQDATA_FILE_NAME in pqdata/config.h;

3.2 add pqd in product package like
    android/device/softwinner/xxx_example_product.mk
    + PRODUCT_PACKAGES += \
    +    pqd




3.3 add seclinux rule for start pqd in init.rc, example:
    android/device/softwinner/common$ git diff --cached
    diff --git a/sepolicy/vendor/awpqd.te b/sepolicy/vendor/awpqd.te
    new file mode 100644
    index 0000000..c7bfaa5
    --- /dev/null
    +++ b/sepolicy/vendor/awpqd.te
    @@ -0,0 +1,4 @@
    +# awpqd service
    +type awpqd, domain;
    +type awpqd_exec, exec_type, vendor_file_type, file_type;
    +init_daemon_domain(awpqd)
    diff --git a/sepolicy/vendor/file_contexts b/sepolicy/vendor/file_contexts
    index f823c0b..a0e363d 100644
    --- a/sepolicy/vendor/file_contexts
    +++ b/sepolicy/vendor/file_contexts
    +
    +#pqd
    +/vendor/bin/pqd  u:object_r:awpqd_exec:s0


