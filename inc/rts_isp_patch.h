/*
 * Copyright (c) 2022 Realtek Semiconductor Corp. All rights reserved.
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-Proprietary
 *
 * This software component is confidential and proprietary to Realtek
 * Semiconductor Corp. Disclosure, reproduction, redistribution, in whole
 * or in part, of this work and its derivatives without express permission
 * is prohibited.
 */

#ifndef _RTS_ISP_PATCH_H_INC_
#define _RTS_ISP_PATCH_H_INC_

#include <stdint.h>
#include <rts_isp_define.h>
#include <rts_isp_errno.h>

struct isp_notify_dynamic_ae {
	enum rts_isp_hdr_mode mode;
	int num; /* exposure gain channel num */

	float exposure[RTS_ISP_HDR_CHAN_MAX]; /* us */
	float analog_gain[RTS_ISP_HDR_CHAN_MAX];
	float digital_gain[RTS_ISP_HDR_CHAN_MAX];
	float isp_hdr_gain[RTS_ISP_HDR_CHAN_MAX];
	float isp_gain;

	float sensor_gain[RTS_ISP_HDR_CHAN_MAX];
	float total_gain[RTS_ISP_HDR_CHAN_MAX];

	uint32_t iq_total_gain[RTS_ISP_HDR_CHAN_MAX]; /* total_gain * 16 */
	uint32_t iq_exp_gain[RTS_ISP_HDR_CHAN_MAX]; /* exposure * total_gain / 100 */
	uint32_t iq_ratio[RTS_ISP_HDR_CHAN_MAX]; /* exposure * total gain ratio */
	uint32_t iq_sensor_gain[RTS_ISP_HDR_CHAN_MAX]; /* sensor_gain * 16 */
};

struct isp_notify_dynamic_awb {
	uint32_t color_temp;
	float r_gain;
	float g_gain;
	float b_gain;
};

struct isp_notify_dynamic_sensor {
	int high_temp_en;
	uint32_t temperature;
};

struct isp_notify_dynamic {
	struct isp_notify_dynamic *pre;

	struct isp_notify_dynamic_ae ae;
	struct isp_notify_dynamic_awb awb;
	struct isp_notify_dynamic_sensor sensor;
};

struct rts_isp_sensor_patch_ops {
	int (*iq_change)(uint32_t isp_id, int iq_sel, int night);
	int (*dynamic)(uint32_t isp_id, const struct isp_notify_dynamic *dyn);
	/* optional */
	int (*init)(uint32_t isp_id);
	int (*cleanup)(uint32_t isp_id);
	int (*preview_start)(uint32_t isp_id);
	int (*preview_stop)(uint32_t isp_id);
};

/* these APIs can only be used in sensor patch */
void rts_isp_patch_set_sharpness(uint32_t isp_id, int sharpness);
void rts_isp_patch_set_saturation(uint32_t isp_id, int saturation);
void rts_isp_patch_set_contrast(uint32_t isp_id, int contrast);
void rts_isp_patch_set_nr_level(uint32_t isp_id, int nr_level);
void rts_isp_patch_set_de_level(uint32_t isp_id, int de_level);

#endif /* _RTS_ISP_PATCH_H_INC_ */

