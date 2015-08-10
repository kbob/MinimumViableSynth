# pass LIBOPENCM3 := the root of your libopencm3 tree.

    OFILES := main.o button.o console.o gpio.o spi.o usb-midi.o

# OFILES := usbmidi.o console.o

 L3_INCDIR := $(LIBOPENCM3)/include
 L3_LIBDIR := $(LIBOPENCM3)/lib

        CC := arm-none-eabi-gcc

      DEFS := -DSTM32F4
  FP_FLAGS := -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS := -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

  CPPFLAGS := -MD -Wall -Wundef -I$(L3_INCDIR) $(DEFS)
    CFLAGS := -std=gnu99 -Os -g -Werror -Wundef \
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

# arm-none-eabi-gcc --static -nostartfiles -L../../../../../libopencm3//lib -T../stm32f429i-discovery.ld -Wl,-Map=lcd-dma.map -Wl,--gc-sections -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 sdram.o clock.o console.o lcd-spi.o lcd-dma.o -lm -lopencm3_stm32f4 -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group -o lcd-dma.elf
