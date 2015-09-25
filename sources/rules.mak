ARFLAGS		?= r
RANLIB		?= @:
UOS_LIB		?= 
ifeq ( "$(UOS_LIB)" , "")
UOS_LIB		= ${TARGET}/libuos.a
OLD_MODE	= TRUE
else
OLD_MODE	= FALSE
endif
#UOS_LIB_DIR	?= $(strip ${shell dirname ${UOS_LIB}} )

#!!! relative path should be resolved to build target lib
TRUE_UOS_LIB := ${realpath ${UOS_LIB}}
override UOS_LIB := $(TRUE_UOS_LIB)

startup.o:	$(OS)/sources/runtime/$(ARCH)/$(STARTUP)
		$(CC) $(ASFLAGS) -c $< -o $@


define do_build_uos
		@for m in $(MODULES); do\
			[ -d $$m ] || mkdir $$m;\
			$(MAKE) -f$(OS)/sources/module.mak -C$$m\
				OS=$(OS) TARGET=$(TARGET) MODULE=$$m UOS_LIB=${UOS_LIB};\
		done
		$(RANLIB) ${UOS_LIB}
endef

#in old mode UOS_LIB always builds, in new - only when it absent, 
#		to force rebuild use 'build_uos' target
ifeq ( "${OLD_MODE}", "TRUE" )
${UOS_LIB}:build_uos
else
${UOS_LIB}:
		@echo build libuos on absent ${UOS_LIB} 
		$(call do_build_uos)

endif


build_uos:	always
		@echo rebuild ${UOS_LIB} 
		$(call do_build_uos)

always:

.SUFFIXES:	.i .srec .hex .dis .cpp .cxx .fl .bin .elf

.PHONY:		$(MODULES) depend

%.o:	%.c
		[ -d .deps ] || mkdir .deps
		$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ -c $<

.cpp.o:
		@[ -d .deps ] || mkdir .deps
		$(CXX) $(CFLAGS) $(DEPFLAGS) -c $<

.cxx.o:
		@[ -d .deps ] || mkdir .deps
		$(CXX) $(CFLAGS) $(DEPFLAGS) -c $<

.S.o:
		@[ -d .deps ] || mkdir .deps
		$(CC) $(ASFLAGS) $(DEPFLAGS) -c $<

.c.s:
		$(CC) $(CFLAGS) -S $<

.c.i:
		$(CC) $(CFLAGS) -E $< > $@

%.elf:	%.c
		[ -d .deps ] || mkdir .deps
		$(CC) $(LDFLAGS) $(CFLAGS) $(DEPFLAGS) $< $(LIBS) -o $@

%.elf:	%.o
		$(CC) $(LDFLAGS) $< $(LIBS) -o $@

.fl.cxx .fl.h:
		fluid -c $<

%.srec:	%.elf
		$(OBJCOPY) -O srec $< $@
		@chmod -x $@

.elf.hex:
		$(OBJCOPY) -O ihex $< $@
		@chmod -x $@

%.bin:	%.elf
		$(OBJCOPY) -O binary $< $@
		@chmod -x $@

.elf.dis:
		$(OBJDUMP) -d -z -S $< > $@

ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif
