/*
 *  Copyright (C) 2019 Realtek Semiconductor Corp.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <rtscamkit.h>
#include <rtsavapi.h>
#include <rtsvideo.h>

/* code flow

+---------+      +---------+
|         |      |         |
|  vin_ch | ===> | h264_ch | ===> rts_av_set_h264_ctrl ===> h264ctrl_out.h264
|         |      |         |
+---------+      +---------+

*/

static int g_exit;

static void Termination(int sign)
{
	g_exit = 1;
}

static void print_help_info(void)
{
	fprintf(stdout, "DESCRIPTION:\n");
	fprintf(stdout, "\tan example to change h264 ctrl\n");
	fprintf(stdout, "USAGE:\n");
	fprintf(stdout, "\texample_h264_ctrl\n");
}

static int __init_sys_vmem(void)
{
	int ret = 0;
	struct rts_sys_vmem_cfg cfg = {0};
	int share = 0;
	int status = 0;

	status = rts_av_sys_vmem_status();

	if (status == RTS_SYS_VMEM_STATUS_ON)
		goto out;

	/* 1-channel */
	cfg.stream[0].enable = 1;
	cfg.stream[0].fmt = RTS_V_FMT_YUV420SEMIPLANAR;
	cfg.stream[0].width = 0;
	cfg.stream[0].height = 0;

	/* vin */
	cfg.stream[0].module[0].type = RTS_AV_ID_VIN;
	cfg.stream[0].module[0].cnt = 1;
	cfg.stream[0].module[0].mode = 1;

	/* h26x */
	cfg.stream[0].module[1].type = RTS_AV_ID_H264;
	cfg.stream[0].module[1].cnt = 1;
	cfg.stream[0].module[1].outbuf.setted = 1;
	cfg.stream[0].module[1].outbuf.shared = share;
	cfg.stream[0].module[1].outbuf.num = 1;
	cfg.stream[0].module[1].outbuf.size = 0; // default size

	ret = rts_av_sys_vmem_set_conf(&cfg);
	if (ret) {
		RTS_ERR("failed to set sysmem cfg, ret:%d\n", ret);
		return ret;
	}

	ret = rts_av_sys_vmem_init();
	if (ret) {
		RTS_ERR("failed to init sysmem cfg, ret:%d\n", ret);
		return ret;
	}

out:
	return ret;
}

static void __release_sys_vmem(void)
{
	int status = 0;

	status = rts_av_sys_vmem_status();

	if (status == RTS_SYS_VMEM_STATUS_OFF)
		return;

	rts_av_sys_vmem_release();
}

static int refresh_h264_ctrl(struct rts_h264_ctrl *ctrl)
{
	int ret;

	ret = rts_av_get_h264_ctrl(ctrl);
	if (ret) {
		RTS_ERR("set h264 ctrl fail, ret = %d\n", ret);
		return RTS_RETURN(RTS_E_GET_FAIL);
	}

	RTS_INFO("intra_qp_delta = %d, bitrate_mode = %d\n",
		ctrl->intra_qp_delta, ctrl->bitrate_mode);

	ctrl->bitrate_mode = RTS_BITRATE_MODE_CBR;
	ctrl->intra_qp_delta = -3;

	ret = rts_av_set_h264_ctrl(ctrl);
	if (ret) {
		RTS_ERR("set h264 ctrl fail, ret = %d\n", ret);
		return RTS_RETURN(RTS_E_SET_FAIL);
	}

	/**
	 * get check whether the new value is set or not,
	 * no need in actual use
	 */
	ret = rts_av_get_h264_ctrl(ctrl);
	if (ret) {
		RTS_ERR("set h264 ctrl fail, ret = %d\n", ret);
		return RTS_RETURN(RTS_E_GET_FAIL);
	}

	RTS_INFO("after refresh intra_qp_delta = %d, bitrate_mode = %d\n",
		ctrl->intra_qp_delta, ctrl->bitrate_mode);

	return ret;
}

int main(int argc, char *argv[])
{
	struct rts_av_profile profile;
	struct rts_vin_attr vin_attr = {0};
	struct rts_h264_attr h264_attr = {0};
	struct rts_h264_ctrl *ctrl = NULL;

	FILE *pfile = NULL;
	int vin = -1;
	int h264 = -1;
	int ret;
	int number = 0;

	rts_set_log_mask(RTS_LOG_MASK_CONS);

	signal(SIGINT, Termination);
	signal(SIGTERM, Termination);

	if (argc >= 2) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			print_help_info();
			return 0;
		}
	}

	ret = rts_av_init();
	if (ret) {
		RTS_ERR("rts_av_init fail\n");
		return ret;
	}

	__init_sys_vmem();

	vin_attr.vin_id = 0;
	vin_attr.vin_buf_num = 2;
	vin_attr.vin_mode = RTS_AV_VIN_RING_MODE;
	if (vin_attr.vin_id != 0 && vin_attr.vin_mode == 1) {
		RTS_ERR("vin ring mode only work in vin_id 0\n");
		return RTS_RETURN(RTS_E_INVALID_ARG);
	}
	if (vin_attr.vin_id > 1 && vin_attr.vin_mode == 2) {
		RTS_ERR("vin direct mode only work in vin_id 0/1\n");
		return RTS_RETURN(RTS_E_INVALID_ARG);
	}
	if (vin_attr.vin_mode < 0 || vin_attr.vin_mode > 2) {
		RTS_ERR("vin mode range [0~2]\n");
		return RTS_RETURN(RTS_E_INVALID_ARG);
	}
	vin = rts_av_create_vin_chn(&vin_attr);
	if (vin < 0) {
		RTS_ERR("fail to create vin chn, ret = %d\n", vin);
		ret = RTS_RETURN(RTS_E_OPEN_FAIL);
		goto exit;
	}

	RTS_INFO("vin chnno:%d\n", vin);

	profile.fmt = RTS_V_FMT_YUV420SEMIPLANAR;
	profile.video.width = 640;
	profile.video.height = 480;
	profile.video.numerator = 1;
	profile.video.denominator = 15;
	ret = rts_av_set_profile(vin, &profile);
	if (ret) {
		RTS_ERR("set vin profile fail, ret = %d\n", ret);
		goto exit;
	}

	h264_attr.level = H264_LEVEL_4;
	h264_attr.rotation = RTS_AV_ROTATION_0;
	h264 = rts_av_create_h264_chn(&h264_attr);
	if (h264 < 0) {
		RTS_ERR("fail to create h264 chn, ret = %d\n", h264);
		ret = RTS_RETURN(RTS_E_OPEN_FAIL);
		goto exit;
	}
	RTS_INFO("h264 chnno:%d\n", h264);

	ret = rts_av_query_h264_ctrl(h264, &ctrl);
	if (ret) {
		RTS_ERR("query h264 ctrl fail, ret = %d\n", ret);
		return ret;
	}

	ctrl->bitrate_mode = RTS_BITRATE_MODE_C_VBR;
	ctrl->intra_qp_delta = 0;

	ret = rts_av_set_h264_ctrl(ctrl);
	if (ret) {
		RTS_ERR("set h264 ctrl fail, ret = %d\n", ret);
		goto exit;
	}

	ret = rts_av_bind(vin, h264);
	if (ret) {
		RTS_ERR("fail to bind vin and h264, ret %d\n", ret);
		goto exit;
	}

	ret = rts_av_enable_chn(vin);
	if (ret) {
		RTS_ERR("enable vin fail, ret = %d\n", ret);
		goto exit;
	}

	ret = rts_av_enable_chn(h264);
	if (ret) {
		RTS_ERR("enable h264 fail, ret = %d\n", ret);
		goto exit;
	}

	ret = rts_av_start_recv(h264);
	if (ret) {
		RTS_ERR("start recv h264 fail, ret = %d\n", ret);
		goto exit;
	}

	RTS_INFO("save to h264ctrl_out.h264\n");
	pfile = fopen("h264ctrl_out.h264", "wb");
	if (!pfile)
		RTS_ERR("open h264 file h264ctrl_out.h264 fail\n");

	while (!g_exit) {
		struct rts_av_buffer *buffer = NULL;

		if (rts_av_recv_block(h264, &buffer, 100))
			continue;

		if (buffer) {
			fwrite(buffer->vm_addr, 1,
				buffer->bytesused, pfile);
			number++;
			rts_av_put_buffer(buffer);
		}

		if (number == 60) {
			ret = refresh_h264_ctrl(ctrl);
			if (RTS_IS_ERR(ret))
				goto exit;
		}
	}

	RTS_INFO("get %d frames\n", number);

	rts_av_disable_chn(vin);
	rts_av_disable_chn(h264);

exit:
	RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
	RTS_SAFE_RELEASE(pfile, fclose);

	if (vin >= 0) {
		rts_av_destroy_chn(vin);
		vin = -1;
	}
	if (h264 >= 0) {
		rts_av_destroy_chn(h264);
		h264 = -1;
	}

	__release_sys_vmem();

	rts_av_release();

	return ret;
}