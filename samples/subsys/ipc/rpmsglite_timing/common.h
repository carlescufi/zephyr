/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_H__
#define COMMON_H__

#define SHM_START_ADDR		0x04000400
#define SHM_SIZE		0x7c00

#define TIMING_PIN	25

#define MSG_MAX_SIZE	256
#define MSG_SIZES	{1, 4, 8, 16, 32, 64, 128, 256}

#define M4_CORE_ADDR	0x01
#define M0_CORE_ADDR	0x02

#endif
