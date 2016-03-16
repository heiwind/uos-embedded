TARGET		= $(CURDIR)
OS		= $(shell cd ../..; pwd)
include target.cfg

TEST		= test-button-intr
TESTS		+= test-led.bin
TESTS		+= test-debug.bin
TESTS		+= test-task.bin
TESTS		+= test-timer.bin
TESTS		+= test-tswitch.bin
TESTS		+= test-latency.bin
TESTS		+= philosophers.bin
TESTS		+= test-usbdev.bin
TESTS		+= test-hid.bin
TESTS		+= test-prog-flash.bin
TESTS		+= test-button.bin
TESTS		+= test-button-intr.bin
TESTS		+= test-timer-timeout.bin
OUTS		= $(TESTS:%.bin=%.elf)

all:		$(OUTS) $(TESTS) $(TEST).dis
		$(SIZE) $(OUTS)

$(OUTS) $(TESTS): startup.o libuos.a

clean:
		rm -rf *~ *.[oasi] *.elf *.lst *.dis *.map .deps $(TESTS) $(MODULES)

load:		$(TEST).bin
		st-flash --reset write $< 0x08000000

include $(OS)/sources/rules.mak
