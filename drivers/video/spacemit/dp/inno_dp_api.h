/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 Spacemit Co., Ltd.
 *
 */

#ifndef __INNO_DP_API_H__
#define __INNO_DP_API_H__

#include "inno_conn.h"

struct inno_conn_t *inno_get_conn_module(enum modules module_id);

int inno_conn_init(struct inno_conn_t *conn);
int inno_conn_prepare(struct inno_conn_t *conn);
int inno_conn_unprepare(struct inno_conn_t *conn);
int inno_conn_enable(struct inno_conn_t *conn);
int inno_conn_disable(struct inno_conn_t *conn);
bool inno_conn_detect(struct inno_conn_t *conn);
void inno_conn_exit(struct inno_conn_t *conn);

#endif /* __INNO_DP_API_H__ */
