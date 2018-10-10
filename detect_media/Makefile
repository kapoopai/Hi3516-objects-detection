# Hisilicon Hi3516 sample Makefile

PWD = $(shell pwd)

ARCH=arm
#ARCH=x86

ifeq ($(ARCH), arm)	
include Makefile.param

MPI_LIBS := $(REL_LIB)/libmpi.a
MPI_LIBS += $(REL_LIB)/libisp.a
MPI_LIBS += $(REL_LIB)/libive.a
else
CC=gcc
endif

# target source
#获取到本目录下所有c
SRC  := $(wildcard ./*.c)
SRC  += $(wildcard ./alg/*.c)

NFS_PATH := /home/davanqi/Hi3516A/ty_proj/filesys/rootfs/home/dvs
NFS_USRLIB_PATH := /home/davanqi/Hi3516A/ty_proj/filesys/rootfs/usr/lib
BIN_PATH := app
BIN := $(PWD)/$(BIN_PATH)/detect_media
OBJ_PATH := $(PWD)/obj/
#OBJ_SUB_PATH := $(OBJ_PATH)common $(OBJ_PATH)bin $(OBJ_PATH)libcodec $(OBJ_PATH)src
OBJ_SUB_PATH += $(OBJ_PATH)main
OBJ_SUB_PATH += $(OBJ_PATH)alg

#构造预期的目标 .o 文件
ALL_OBJ = $(patsubst %.c,%.o,$(SRC))

#指定 .o 文件生成的目录
ALL_OBJ_POS = $(addprefix $(OBJ_PATH),$(ALL_OBJ))

.PHONY : clean all

#$(warning TARGET: $(target))
#$(warning PWD: $(PWD))
#$(warning ALL_DIR: $(ALL_DIR))
#$(warning ALL_OBJ: $(ALL_OBJ))
#$(warning SRC: $(SRC))
#$(warning OBJ: $(OBJ))
#$(warning ALL_OBJ_POS: $(ALL_OBJ_POS))

ifeq ($(ARCH), arm)
LIBS := $(PWD)/libs/arm
else
LIBS := $(PWD)/libs/x86
endif

DYN_PATH := -L$(LIBS)
ALG_LIBS := -lusb -lmvnc

# add include
INC_FLAGS += -Iinc -Iinc/hi_intf -Iinc/vpu_intf
DFLAGS = -DARCH=$(ARCH)

CFLAGS += -Wall -g
CFLAGS += $(INC_FLAGS)
#CFLAGS += $(INC_FLAGS)
#CFLAGS += -DH265_TEST

# add end

%.o:%.c
	@mkdir -p $(OBJ_PATH)
	@mkdir -p $(OBJ_SUB_PATH)
	@$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $(OBJ_PATH)$@
	@echo ">>> make object file success!! file: $< "

all: $(ALL_OBJ)
#.PHONY : all 
	@mkdir -p $(BIN_PATH)
	$(CC) $(CFLAGS) -o $(BIN) $(ALL_OBJ_POS) $(MPI_LIBS) $(JPEGD_LIBA) $(AUDIO_LIBA) $(ALG_LIBS) $(DYN_PATH)

clean:
	@rm -f $(ALL_OBJ)
	@rm -f $(BIN)
	@rm -f $(ALL_OBJ_POS)
	
cleanstream:
	@rm -f *.h264
	@rm -f *.jpg
	@rm -f *.mjp
	@rm -f *.mp4
