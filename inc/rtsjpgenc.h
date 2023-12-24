/*
 * Realtek Semiconductor Corp.
 *
 * libs/include/rtsjpeg.h
 *
 * Copyright (C) 2014      Ming Qian<ming_qian@realsil.com.cn>
 */
#ifndef _RTSJPGENC_H
#define _RTSJPGENC_H
#include <stdint.h>
#include "rts_camera_jpgenc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RTSJPGENC_MAX_OUTNUM	4

typedef const void *RtsJpgEncInst;

enum rtsjpgenc_picture_type {
	RTSJPGENC_YUV420_SEMIPLANAR = 0,
	RTSJPGENC_YUV422_SEMIPLANAR
};

enum rtsjpgenc_rotation {
	RTSJPGENC_ROTATE_0 = 0,
	RTSJPGENC_ROTATE_90R = 2, /*rotate 90 degrees clockwise*/
	RTSJPGENC_ROTATE_90L = 3, /*rotate 90 degrees counter-clockwise*/
};

enum rtsjpgenc_enc_mode {
	RTSJPGENC_NORMAL = 0,
	RTSJPGENC_STREAM_NORMAL,
	RTSJPGENC_STREAM_CONTINUOUS
};

enum rtsjpgenc_rotation_mirror {
	RTSJPGENC_RM_NO = 0,
	RTSJPGENC_RM_VER = 1,
	RTSJPGENC_RM_HOR = 2,
	RTSJPGENC_RM_180 = 3,
	RTSJPGENC_RM_90L_HOR = 4,
	RTSJPGENC_RM_90R = 5,
	RTSJPGENC_RM_90L = 6,
	RTSJPGENC_RM_90L_VER = 7,
};


struct rtsjpgenc_config {
	enum rtsjpgenc_picture_type input_type;
	uint8_t rotation;
	uint8_t mode;
	uint8_t outnum;
	uint8_t l2f;
	uint8_t isp_chn;
	uint16_t width;
	uint16_t height;
};

struct rtsjpgenc_coding_ctrl {
	uint16_t normal_compress_rate:4; /*0.5 as the unit*/
};

struct rtsjpgenc_encin {
	uint32_t src_busLuma;
	uint32_t src_busChroma;
	uint32_t out_bus_buf[RTSJPGENC_MAX_OUTNUM];
	uint32_t out_buf_size;
	void *p_outbuf[RTSJPGENC_MAX_OUTNUM];
	uint32_t out_bytesused;
	uint8_t out_idx;
	uint64_t out_time;
};

int rtsjpgenc_init(RtsJpgEncInst *pinst);
int rtsjpgenc_release(RtsJpgEncInst inst);
int rtsjpgenc_set_config(RtsJpgEncInst inst, struct rtsjpgenc_config *pcfg);
int rtsjpgenc_encode(RtsJpgEncInst inst, struct rtsjpgenc_encin *pencin);
int rtsjpgenc_get_coding_ctrl(RtsJpgEncInst inst,
			      struct rtsjpgenc_coding_ctrl *ctrl);
int rtsjpgenc_set_coding_ctrl(RtsJpgEncInst inst,
			      struct rtsjpgenc_coding_ctrl *ctrl);
int rtsjpgenc_set_quality(RtsJpgEncInst inst, int quality);
int rtsjpgenc_set_epoll_hook(RtsJpgEncInst inst, void *master,
			     int (*hook_func)(void *master, int fd));
int rtsjpgenc_check_done(RtsJpgEncInst inst);
int rtsjpgenc_make_rdy(RtsJpgEncInst inst);
int rtsjpgenc_use_sigio(RtsJpgEncInst inst, int sigio_need);

int rtsjpgenc_streamon(RtsJpgEncInst inst, struct rtsjpgenc_encin *pencin);
int rtsjpgenc_push_back_outbuf(RtsJpgEncInst inst, uint8_t idx);
int rtsjpgenc_pop_outbuf(RtsJpgEncInst inst, struct rtsjpgenc_encin *pencin);
int rtsjpgenc_streamoff(RtsJpgEncInst inst);
int rtsjpgenc_get_fd(RtsJpgEncInst inst);
int rtsjpgenc_clr_info(RtsJpgEncInst inst, int isp_chn);

#ifdef __cplusplus
}
#endif
#endif
