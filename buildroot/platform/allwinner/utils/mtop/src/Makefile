TARGET := mtop
SRCS := mtop.c check_hardware.c config.c
OBJS := $(SRCS:%.c=%.o)
DEPEND := mtop.d
#CC := ../../../../out/riscv64-linux-x86_64-20200528/bin/riscv64-unknown-linux-gnu-gcc
#CC := ../../../../out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc
#CC := ../../../../out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc

.PHONY: all clean

all:$(TARGET)

-include $(DEPEND)

$(DEPEND):$(SRCS)
	$(CC) -MM $(SRCS) > $@.$$$$ ;\
	sed 's/$(OBJS)/$(OBJS) $@/g' < $@.$$$$ >$@ ;\
	$(RM) $@.$$$$

%.o:%.c
	$(CC) -c $(CFLAGS) $<

$(TARGET):$(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	$(RM) $(TARGET) $(OBJS) $(DEPEND)
