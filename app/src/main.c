/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);
int main(void) {
  LOG_INF("VERSION 0 - Hello World! %s", CONFIG_BOARD_TARGET);
  float val = 3;
  bool vbr = val;
  return vbr;
}
