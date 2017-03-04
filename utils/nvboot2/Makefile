TARGET		= $(CURDIR)
OS		:= $(shell cd ../../uosdev; pwd)
mkfile_file = ${firstword  ${MAKEFILE_LIST}}
mkfile_path = ${dir ${realpath ${mkfile_file}} }
# ЭТОТ финт нужун чтобы обойти багу abspath, realpath, dir в makefile msys/sygwin 3.82 
#	- он путает c: с разделителем путей и генерит вывод как будто для некольких путей 
TARGET_	?= ${mkfile_path}
TARGET	?= ${shell cd ${TARGET_}; pwd}

SRC		= ${mkfile_path}

-include target.cfg

trueOS  := ${shell cd "${OS}"; pwd}
override OS := ${realpath ${trueOS}}

BOOT_ORG	?= 0xbfc00000
IMG_ORG		?= 0xbc000000


ifndef PROG

ifneq ("$(wildcard ./mcprog.exe)","")
PROG	= mcprog
else 

ifneq ("$(wildcard $(TARGET)/mcprog.exe)","")
PROG	= $(TARGET)/mcprog.exe
else
 
ifdef MCS4_HOME

ifneq ("$(wildcard $(MCS4_HOME)/../mcprog/mcprog.exe)","")
PROG	= $(MCS4_HOME)/../mcprog/mcprog.exe
endif

endif
 
endif

endif

endif

PROG_OPTS 	?= 

PLATFORM	?= nvcom02tem
PROGRAM		?= nvboot
APPLICATION	?= app

UOS_LIB 	?= libuos.a
# cant take just $(dir ) cause of buggy make 3.82 with windows drive letter
UOS_LIB_DIR_ := $(dir ${realpath ${UOS_LIB}})
UOS_LIB_DIR := $(shell cd ${UOS_LIB_DIR_}; pwd)

_APP		=  ${realname ${APPLICATION}}

SRCUNITS	= $(PROGRAM) elf relocs
UNITS		= $(SRCUNITS)
OBJS	= $(addsuffix .o, $(UNITS))

vpath	%.c	$(SRC_INCLUDES)

ifeq ("${PLATFORM}","tst4153")
CFLAGS += -DTST4153
endif

PROG_DIR = $(dir $(firstword $(PROG)))

all:		$(PROGRAM).elf $(PROGRAM).dis $(PROGRAM).bin
		$(SIZE) *.elf

$(PROGRAM).elf: $(OBJS) $(UOS_LIB)
		pwd 
		$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

clean:
		rm -rf *~ *.[oasi] *.elf *.hex *.srec *.lst *.dis .deps $(MODULES) $(PROGRAM).dis $(PROGRAM).bin img.bin

boot_only: $(PROGRAM).bin
		cp $(PROGRAM).bin boot.bin
		echo "img?" >> boot.bin

image: $(PROGRAM).bin $(APPLICATION)
		cp $(PROGRAM).bin img.bin
#		echo "IMAG" >> img.bin
		ls -l "$(APPLICATION)" | awk '{print "IMAG" $$5}' >> img.bin
		cat "$(APPLICATION)" >> img.bin

load_all: image
		cd "$(PROG_DIR)"
		$(PROG) -e2 $(TARGET)/img.bin $(BOOT_ORG)

load_boot: boot_only
		cd "$(PROG_DIR)"
		$(PROG) -e2 $(TARGET)/boot.bin $(BOOT_ORG)

load_app_cram: $(APPLICATION)
#		echo "IMGC" > app.bin
		ls -l "$(APPLICATION)" | awk '{printf "IMGC" $$5 "\n"}' > app.bin
		cat "$(APPLICATION)" >> app.bin
		cd "$(PROG_DIR)"
		$(PROG) -e2 $(TARGET)/app.bin $(IMG_ORG)

load_app_sram: $(APPLICATION)
#		echo "IMGR" > app.bin
		ls -l "$(APPLICATION)" | awk '{printf "IMGR" $$5 "\n"}' > app.bin
		cat "$(APPLICATION)" >> app.bin
		cd "$(PROG_DIR)"
		$(PROG) -e2 $(TARGET)/app.bin $(IMG_ORG)

APP_ELF = $(addsuffix .elf, $(basename $(APPLICATION)))

$(APPLICATION): $(APP_ELF)
		$(OBJCOPY) -S -R .rel.dyn -O binary $(APP_ELF) $(APPLICATION)

load_elf: $(APP_ELF)
		cp $(APP_ELF) app.elf
		$(STRIP) -s app.elf
		cd "$(PROG_DIR)"
		$(PROG) -e2 $(TARGET)/app.elf $(IMG_ORG)

app.elf: $(APP_ELF)
		cp $(APP_ELF) app.elf
		$(STRIP) -s app.elf

verify_elf: app.elf
		cd "$(PROG_DIR)"
		$(PROG) -v $(TARGET)/app.elf $(IMG_ORG)

-include $(OS)/sources/rules.mak

###
