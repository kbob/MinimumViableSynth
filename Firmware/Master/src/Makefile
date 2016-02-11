# pass LIBOPENCM3 := the root of your libopencm3 tree.
ifeq '$(LIBOPENCM3)' ''
   $(error Please pass LIBOPENCM3 = the root of your libopencm3 tree)
endif

########################################################################
   SOURCES := main.c anim.c button.c config.c console.c gpio.c          \
              lcd-dma.c midi.c modes.c sdram.c spi.c spi-proto.c        \
              spi-responder.c state.c systick.c usb-midi.c
    CFILES := $(filter %.c, $(SOURCES))
    OFILES := $(CFILES:%.c=%.o)

 L3_INCDIR := $(LIBOPENCM3)/include
 L3_LIBDIR := $(LIBOPENCM3)/lib

        CC := arm-none-eabi-gcc

      DEFS := -DSTM32F4
  FP_FLAGS := -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS := -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

  CPPFLAGS := -MD -I$(L3_INCDIR) $(DEFS)
    CFLAGS := -std=gnu99 -O0 -g -Wall -Wundef -Werror \
              -fno-common -ffunction-sections -fdata-sections \
              $(CPPFLAGS) $(ARCH_FLAGS)
   LDFLAGS := -g --static -nostartfiles \
              -L$(L3_LIBDIR)  \
              -T stm32f429i-discovery.ld \
              -Wl,-Map=master.map -Wl,--gc-sections \
              $(ARCH_FLAGS)
   LDLIBES := -lopencm3_stm32f4
    LDLIBS := -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group


default: master.elf

master.elf: $(OFILES)
	$(LINK.o) $^ $(LDLIBES) $(LDLIBS) -o $@

clean:
	rm -f *.d *.o *.elf *.map

ifeq '$(filter clean$ help,$(or $(MAKECMDGOALS),help))' ''
  -include $(CFILES:%.c=%.d)
endif
