/*
 * SegCache: encapsulate non-const global variables (including both static and non-static) required by segcache.
 * 
 * By Siyuan Sheng (2023.08.10).
 */

#pragma once

#include <time/time.h>

struct SegCache
{
    // From time/time.h
    time_t time_start;
    proc_time_i proc_sec;
    proc_time_fine_i proc_ms;
    proc_time_fine_i proc_us;
    proc_time_fine_i proc_ns;
};