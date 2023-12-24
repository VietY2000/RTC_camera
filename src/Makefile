
CC = /home/viety/UBOOT_CAM/Realtek/rts39xx_sdk_v5.0.1/toolchain/tool/bin/asdk-linux-gcc

CFLAGS 	+= -I/home/viety/UBOOT_CAM/Realtek/rts39xx_sdk_v5.0.1/out/demo/my_workspaces/inc

LDFLAGS += -L/home/viety/UBOOT_CAM/Realtek/rts39xx_sdk_v5.0.1/out/demo/my_workspaces/lib

PHONY: audio
audio:
	$(CC) -o audio audio.c $(CFLAGS) -lrtsaudio -lrtstream -lasound -lsoxr $(LDFLAGS) 
	@echo "---------build OK---------"

PHONY: video
video:
	$(CC) -o video video.c $(CFLAGS) -lrtsvideo -lrtstream $(LDFLAGS)
	@echo "---------build OK---------"

PHONY: main
main:
	$(CC) -o main main.c my_module.c $(CFLAGS) -lpthread -lrtsvideo -lrtstream -lasound -lrtsaudio -lsoxr $(LDFLAGS)
	@echo "---------build OK---------"

PHONY: cp
cp:
	cp audio main video /home/viety/UBOOT_CAM/Realtek/rts39xx_sdk_v5.0.1/out/demo/NFS_SHARE