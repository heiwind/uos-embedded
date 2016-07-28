TARGET		= $(CURDIR)
OS		= $(shell cd ../..; pwd)
include target.cfg

TEST  = test_task.bin

TESTS = test_debug.bin test_task.bin test_timer.bin

OUTS  = $(TESTS:%.bin=%.elf)
LSS   = $(TESTS:%.bin=%.lss)
MAPS  = $(TESTS:%.bin=%.map)

# Using avrdude programmer with STK200 LPT adapter.
PROG           = avrdude -cstk200 -Enoreset -pusb647

# Using avrdude programmer with JTAGICE USB adapter.
# PROG            = avrdude -cjtag1 -P /dev/ttyUSB? -pusb647

# Using avrdude programmer with AVRISP USB adapter.
# PROG           = avrdude -cstk500v2 -P /dev/ttyUSB? -pusb647

all:	$(OUTS) $(TESTS) $(LSS) $(MAPS)
		$(SIZE) $(OUTS)


$(OUTS) $(TESTS): startup.o libuos.a

$(LSS): %.lss: %.elf
		$(OBJDUMP) -h -S $< > $@

$(MAPS): %.map: %.elf
		$(NM) $< > $@
		
clean:
		rm -rf *.lss *.map *.elf *.hex *.bin *.srec *.[oasi] *.lst *.dis *~ .deps $(MODULES)

probe:
		$(PROG) -v

load:	$(TEST)
		even-fw $(TEST)
		$(PROG) -e -U flash:w:$(TEST)

include $(OS)/sources/rules.mak




###
