TARGET := cpu_monitor
SRCS := cpu_test.c cpu_lib.c cpu_dis.c
OBJS := $(SRCS:%.c=%.o)
DEPEND := cpu_monitor.d

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
