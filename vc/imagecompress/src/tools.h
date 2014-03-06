#pragma once

#include "stdio.h"
#include "stdint.h"

#define LOGI(format, ...) ( printf( format "\n", __VA_ARGS__ ) )
#define LOGW(format, ...) ( printf( "[Warning] " format "\n", __VA_ARGS__ ) )
#define LOGE(format, ...) ( printf( "[Error] " format "\n", __VA_ARGS__ ) )

template<class T> inline T  Max2( T x, T y ) { return ( x > y ) ? x : y; }
template<class T> inline T  Min2( T x, T y ) { return ( x < y ) ? x : y; }
template<class T> inline T  Max3( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? x : z ) : ( ( y > z ) ? y : z ); }
template<class T> inline T  Min3( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? x : z ) : ( ( y < z ) ? y : z ); }

#define M_IN
#define M_OUT
