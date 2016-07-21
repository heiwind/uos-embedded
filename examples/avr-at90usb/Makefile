TARGET		= $(CURDIR)
OS		= $(shell cd ../..; pwd)
include target.cfg

TEST		= test_debug
TESTS		= test_debug.bin
OUTS		= $(TESTS:%.bin=%.elf)

# Using avrdude programmer with STK200 LPT adapter.
PROG           = avrdude -cstk200 -Enoreset -pusb647

# Using avrdude programmer with JTAGICE USB adapter.
# PROG            = avrdude -cjtag1 -P /dev/ttyUSB? -pusb647

# Using avrdude programmer with AVRISP USB adapter.
# PROG           = avrdude -cstk500v2 -P /dev/ttyUSB? -pusb647

all:		$(OUTS) $(TESTS)
		$(SIZE) $(OUTS)

$(OUTS) $(TESTS): startup.o libuos.a

clean:
		rm -rf assembler.txt map.txt *.elf *.hex *.bin *.srec *.[oasi] *.lst *.dis *~ .deps $(MODULES)

probe:
		$(PROG) -v

load:		$(TEST).bin
		$(OBJDUMP) -S $(TEST).elf > assembler.txt
		$(OBJDUMP) -t $(TEST).elf > map.txt
		even-fw $(TEST).bin
		$(PROG) -e -U flash:w:$(TEST).bin

include $(OS)/sources/rules.mak

###
