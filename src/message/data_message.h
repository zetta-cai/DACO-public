/*
 * Help to include the header files of all data messages (e.g., local messages between clients and edge nodes, and global messages between edge nodes and cloud).
 * 
 * By Siyuan Sheng (2023.05.18).
 */

#ifndef DATA_MESSAGE_H
#define DATA_MESSAGE_H

#include "message/data/local/local_get_request.h"
#include "message/data/local/local_put_request.h"
#include "message/data/local/local_del_request.h"
#include "message/data/local/local_get_response.h"
#include "message/data/local/local_put_response.h"
#include "message/data/local/local_del_response.h"

#include "message/data/global/global_get_request.h"
#include "message/data/global/global_put_request.h"
#include "message/data/global/global_del_request.h"
#include "message/data/global/global_get_response.h"
#include "message/data/global/global_put_response.h"
#include "message/data/global/global_del_response.h"

#include "message/data/redirected/redirected_get_request.h"
#include "message/data/redirected/redirected_get_response.h"

// ONLY used by COVERED
#include "message/data/redirected/covered_redirected_get_request.h"
#include "message/data/redirected/covered_redirected_get_response.h"
// For cooperative cache placement management (non-blocking data fetching)
#include "message/data/redirected/covered/covered_bgfetch_redirected_get_request.h"
#include "message/data/redirected/covered/covered_bgfetch_redirected_get_response.h"
#include "message/data/global/covered/covered_bgfetch_global_get_request.h"
#include "message/data/global/covered/covered_bgfetch_global_get_response.h"

#endif