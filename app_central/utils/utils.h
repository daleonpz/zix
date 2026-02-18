// Copyright (c) 2026 Daniel Paredes (daleonpz)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <zephyr/logging/log.h>

#define UTILS_PRINT_HEX(p_label, p_text, len)                                                      \
    ({                                                                                             \
        LOG_DBG("---- %s (len: %u): ----", p_label, len);                                          \
        LOG_HEXDUMP_DBG(p_text, len, "Content:");                                                  \
        LOG_DBG("---- %s end  ----", p_label);                                                     \
    })
