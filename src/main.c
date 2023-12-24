/*
 *  Copyright (C) 2019 Realtek Semiconductor Corp.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <my_module.h>
#include <signal.h>
#include <rtscamkit.h>
#include <rtsavapi.h>
#include <rtsvideo.h>
#include <rtsaudio.h>
#include <rtsamixer.h>
#include <pthread.h>

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

	status = rts_av_sys_vmem_release();
	if(!status){
		printf("release vmem\n");
	}
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

static void *thread_audio(void* arg){
	int ret;
	int capture = -1;
	uint32_t second_save = 60;

	struct rts_audio_attr capt_attr;

	save_struct file_alaw;
	file_alaw.file = NULL;
	file_alaw.channel = -1;
	strcpy(file_alaw.path, "/media/audio/");
	file_alaw.fm = ALAW;
	
	signal(SIGINT, Termination);

	rts_set_log_mask(RTS_LOG_MASK_CONS);

	if (RTS_IS_ERR(ret)) {
		RTS_ERR("rts_av_init fail\n");
		goto exit;
	}

	memset(&capt_attr, 0, sizeof(capt_attr));
	snprintf(capt_attr.dev_node, sizeof(capt_attr.dev_node), "hw:0,1");
	capt_attr.rate = 8000;
	capt_attr.format = 16;
	capt_attr.channels = 1;

	capture = rts_av_create_audio_capture_chn(&capt_attr);
	if (RTS_IS_ERR(capture)) {
		ret = capture;
		goto exit;
	}
	RTS_INFO("audio capture chn : %d\n", capture);

	ret = rts_audio_set_capture_lr_volume(100, 100);
	if(ret){
		printf("fail to set capture_lr_volume\n");
		goto exit;
	}
	
	file_alaw.channel = rts_av_create_audio_encode_chn(RTS_AUDIO_TYPE_ID_ALAW, 0);
	if (RTS_IS_ERR(file_alaw.channel)) {
		ret = file_alaw.channel;
		goto exit;
	}
	RTS_INFO("encode capture chn : %d\n", file_alaw.channel);

	ret = rts_av_bind(capture, file_alaw.channel);
	if (ret) {
		RTS_ERR("fail to bind capture and encode, ret = %d\n", ret);
		goto exit;
	}

	ret = rts_av_enable_chn(capture);
	if (RTS_IS_ERR(ret)) {
		RTS_ERR("enable audio capture fail, ret = %d\n", ret);
		goto  exit;
	}

	ret = rts_av_enable_chn(file_alaw.channel);
	if (RTS_IS_ERR(ret)) {
		RTS_ERR("enable audio encode fail, ret = %d\n", ret);
		goto  exit;
	}

	time_t start = time(NULL) + 7*60*60;
	time_t now;

	func_creat_new_folder(&file_alaw, start);

	func_set_file_name(&file_alaw, start, second_save);

	rts_av_start_recv(file_alaw.channel);

	while (1) {

		now = time(NULL) + 7*60*60;

		func_save(&file_alaw, &start, now, second_save);
		printf("2\n");
		if(g_exit){
			break;
		}
		usleep(1000);
	}

	ret = rts_av_stop_recv(file_alaw.channel);
	if(!ret){
		printf("stop recv alaw\n");
	}

	ret = rts_av_disable_chn(file_alaw.channel);
	if(!ret){
		printf("disable alaw\n");
	}

	ret = rts_av_disable_chn(capture);
	if(!ret){
		printf("disable capture\n");
	}

	ret = rts_av_unbind(file_alaw.channel, capture);
	if(!ret){
		printf("unbind capture vs alaw\n");
	}
	save_struct new_name_audio;
	strcpy(new_name_audio.folder, file_alaw.folder);
	new_name_audio.fm = ALAW;
	func_set_file_name(&new_name_audio, start, difftime(now, start));
	rename(file_alaw.name, new_name_audio.name);

exit:

	if (file_alaw.channel >= 0) {
		rts_av_destroy_chn(file_alaw.channel);
		file_alaw.channel = -1;
		printf("destroy channel alaw\n");
	}
	if(capture >= 0){
		rts_av_destroy_chn(capture);
		capture = -1;
		printf("destroy channel capture\n");
	}

	return NULL;
}

int main(int argc, char *argv[])
{	
	pthread_t tid;

	struct rts_av_profile profile;
	struct rts_vin_attr vin_attr = {0};
	struct rts_h264_attr h264_attr = {0};
	struct rts_h264_ctrl *ctrl = NULL;
	// struct rts_jpgenc_attr jpg_attr = {0};

	save_struct file_h264;
	// save_struct file_jpg;

	uint32_t vin = -1;

	file_h264.file = NULL;
	file_h264.channel = -1;
	strcpy(file_h264.path, "/media/video/");
	file_h264.fm = H264;

	// file_jpg.file = NULL;
	// file_jpg.channel = -1;
	// strcpy(file_jpg.path, "/media/sdcard/thumb/");
	// file_jpg.fm = JPG;

	uint16_t ret;
	uint32_t frame = 0;
	uint32_t second_save = 60;

	rts_set_log_mask(RTS_LOG_MASK_CONS);

	signal(SIGINT, Termination);

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
	
	pthread_create(&tid, NULL, thread_audio, NULL);
	
	__init_sys_vmem();

	/* creat vin */
	vin_attr.vin_id = 0;
	vin_attr.vin_buf_num = 2;
	vin_attr.vin_mode = RTS_AV_VIN_RING_MODE;
	vin = rts_av_create_vin_chn(&vin_attr);
	if (vin < 0) {
		RTS_ERR("fail to create vin chn, ret = %d\n", vin);
		ret = RTS_RETURN(RTS_E_OPEN_FAIL);
		goto exit;
	}

	RTS_INFO("vin chnno:%d\n", vin);

	profile.fmt = RTS_V_FMT_YUV420SEMIPLANAR;
	profile.video.width = 1280;
	profile.video.height = 720;
	profile.video.numerator = 1;
	profile.video.denominator = 25;
	ret = rts_av_set_profile(vin, &profile);
	if (ret) {
		RTS_ERR("set vin profile fail, ret = %d\n", ret);
		goto exit;
	}

	/* creat h264 */
	h264_attr.level = H264_LEVEL_4;
	h264_attr.rotation = RTS_AV_ROTATION_0;
	file_h264.channel = rts_av_create_h264_chn(&h264_attr);
	if (file_h264.channel < 0) {
		RTS_ERR("fail to create h264 chn, ret = %d\n", file_h264.channel);
		ret = RTS_RETURN(RTS_E_OPEN_FAIL);
		goto exit;
	}
	RTS_INFO("h264 chnno:%d\n", file_h264.channel);

	// /* creat jpg */
	// jpg_attr.rotation = RTS_AV_ROTATION_0;
	// file_jpg.channel = rts_av_create_mjpeg_chn(&jpg_attr);
	// if (file_jpg.channel < 0) {
	// 	RTS_ERR("fail to create jpg chn, ret = %d\n", file_jpg.channel);
	// 	ret = RTS_RETURN(RTS_E_OPEN_FAIL);
	// 	goto exit;
	// }
	// RTS_INFO("jpg chn : %d\n", file_jpg.channel);

	ret = rts_av_query_h264_ctrl(file_h264.channel, &ctrl);
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

	ret = rts_av_bind(vin, file_h264.channel);
	if (ret) {
		RTS_ERR("fail to bind vin and h264, ret %d\n", ret);
		goto exit;
	}

	// ret = rts_av_bind(vin, file_jpg.channel);
	// if (ret) {
	// 	RTS_ERR("fail to bind vin and mjpeg, ret %d\n", ret);
	// 	goto exit;
	// }

	ret = rts_av_enable_chn(vin);
	if (ret) {
		RTS_ERR("enable vin fail, ret = %d\n", ret);
		goto exit;
	}

	ret = rts_av_enable_chn(file_h264.channel);
	if (ret) {
		RTS_ERR("enable h264 fail, ret = %d\n", ret);
		goto exit;
	}

	// ret = rts_av_enable_chn(file_jpg.channel);
	// if (ret) {
	// 	RTS_ERR("enable h264 fail, ret = %d\n", ret);
	// 	goto exit;
	// }

	time_t start = time(NULL) + 7*60*60;
	time_t now;

	func_creat_new_folder(&file_h264, start);

	func_set_file_name(&file_h264, start, second_save);

	// func_creat_new_folder(&file_jpg, start);

	// func_set_file_name(&file_jpg, start, second_save);

	ret = rts_av_start_recv(file_h264.channel);
	if (ret) {
		RTS_ERR("start recv h264 fail, ret = %d\n", ret);
		goto exit;
	}

	// ret = rts_av_start_recv(file_jpg.channel);
	// if (ret) {
	// 	RTS_ERR("start recv h264 fail, ret = %d\n", ret);
	// 	goto exit;
	// }

	while (!g_exit) {

		now = time(NULL) + 7*60*60;

		func_save(&file_h264, &start, now, second_save);

		if (frame == 60) {
			ret = refresh_h264_ctrl(ctrl);
			if (RTS_IS_ERR(ret))
				goto exit;
		}

		frame++;
		printf("1\n");
		if(g_exit){
			break;
		}
		usleep(1000);
	}
	pthread_join(tid, NULL);
	
	RTS_INFO("get %d frames\n", frame);

	ret = rts_av_stop_recv(file_h264.channel);
	if(!ret){
		printf("stop recv h264\n");
	}
	// rts_av_stop_recv(file_jpg.channel);

	ret = rts_av_disable_chn(file_h264.channel);
	if(!ret){
		printf("disable h264\n");
	}

	ret = rts_av_disable_chn(vin);
	if(!ret){
		printf("disable vin\n");
	}
	
	// rts_av_disable_chn(file_jpg.channel);
	ret = rts_av_unbind(file_h264.channel, vin);
	if(!ret){
		printf("ubind vin vs h264\n");
	}
	// rts_av_unbind(file_jpg.channel, vin);
	save_struct new_name_video;
	// // save_struct new_name_thumb;

	strcpy(new_name_video.folder, file_h264.folder);
	// // strcpy(new_name_thumb.folder, file_jpg.folder);

	new_name_video.fm = H264;
	// // new_name_thumb.fm = JPG;
	
	func_set_file_name(&new_name_video, start, difftime(now, start));
	// // func_set_file_name(&new_name_thumb, start, difftime(now, start));

	rename(file_h264.name, new_name_video.name);
	// // rename(file_jpg.name, new_name_thumb.name);

	
exit:
	if(ctrl){
		RTS_SAFE_RELEASE(ctrl, rts_av_release_h264_ctrl);
		printf("release ctrl\n");
	}
	
	if (vin >= 0) {
		rts_av_destroy_chn(vin);
		vin = -1;
		printf("destroy vin\n");
	}
	
	if (file_h264.channel >= 0) {
		rts_av_destroy_chn(file_h264.channel);
		file_h264.channel = -1;
		printf("destroy h264\n");
	}
	
	// if (file_jpg.channel >= 0) {
	// 	rts_av_destroy_chn(file_jpg.channel);
	// 	file_jpg.channel = -1;
	// }

	__release_sys_vmem();
	
	ret = rts_av_release();
	if(!ret){
		printf("release h264 thread\n");
	}
	return ret;
}