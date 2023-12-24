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
#include <string.h>
#include <signal.h>
#include <rtscamkit.h>
#include <rtsavapi.h>
#include <rtsaudio.h>
#include <getopt.h>

/* code flow

        +------------+
        |            |
mic ===>| capture_ch |===>stream buffer
        |            |
        +------------+

*/

struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{0, 0, 0, 0}
};


static int g_exit;
static void Termination(int sign)
{
	g_exit = 1;
}

void print_help_info(void)
{
	fprintf(stdout, "DESCRIPTION:\n");
	fprintf(stdout, "\tprovide a example for capture audio\n");
	fprintf(stdout, "USAGE:\n");
	fprintf(stdout, "\texample_audio_capture\n");
	fprintf(stdout, "\tuse ctrl + c to quit\n");
}

int main(int argc, char *argv[])
{
	int ret;
	int frame_num = 0;
	int capture = -1;
	int c;
	struct rts_audio_attr attr;

	signal(SIGINT, Termination);
	signal(SIGTERM, Termination);

	rts_set_log_mask(RTS_LOG_MASK_CONS);

	while ((c = getopt_long(argc, argv,
				":h", longopts, NULL)) != -1) {
		switch (c) {
		case 'h':
			print_help_info();
			return RTS_OK;
		}
	}
	ret = rts_av_init();
	if (RTS_IS_ERR(ret)) {
		RTS_ERR("rts_av_init fail\n");
		return ret;
	}

	memset(&attr, 0, sizeof(attr));
	snprintf(attr.dev_node, sizeof(attr.dev_node), "hw:0,1");
	attr.rate = 8000;
	attr.format = 16;
	attr.channels = 1;

	capture = rts_av_create_audio_capture_chn(&attr);
	if (RTS_IS_ERR(capture)) {
		ret = capture;
		goto exit;
	}
	RTS_INFO("audio capture chn : %d\n", capture);

	ret = rts_av_enable_chn(capture);
	if (RTS_IS_ERR(ret)) {
		RTS_ERR("enable audio capture fail, ret = %d\n", ret);
		goto  exit;
	}

	rts_av_start_recv(capture);

	FILE *pfile = NULL;
	pfile = fopen("test.pcm", "wb");
	if(!pfile){
		printf("error opne file\n");
		goto exit;
	}

	while (!g_exit) {
		struct rts_av_buffer *buffer = NULL;

		if (RTS_IS_ERR(rts_av_recv_block(capture, &buffer, 100)))
			continue;

		if (!buffer){
			
			continue;
		}	
		frame_num++;
		fwrite(buffer->vm_addr, 1, buffer->bytesused, pfile);
		RTS_INFO("frames bytes %d, tv = %llu\n",
			 buffer->bytesused, buffer->timestamp);
		RTS_SAFE_RELEASE(buffer, rts_av_put_buffer);
	}

	rts_av_disable_chn(capture);
	rts_av_stop_recv(capture);

	RTS_INFO("get %d frames\n", frame_num);
exit:
	RTS_SAFE_CLOSE(capture, rts_av_destroy_chn);
	rts_av_release();
	RTS_SAFE_RELEASE(pfile, fclose);
	return ret;
}
