#pragma once

#include "stdio.h"

#define LOGI(format, ...) ( printf( format "\n", __VA_ARGS__ ) )
#define LOGW(format, ...) ( printf( "[Warning] " format "\n", __VA_ARGS__ ) )
#define LOGE(format, ...) ( printf( "[Error] " format "\n", __VA_ARGS__ ) )
