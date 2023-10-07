/*
 * Help to include the header files of all control messages (e.g., between edge nodes and beacon nodes).
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef CONTROL_MESSAGE_H
#define CONTROL_MESSAGE_H

#include "message/control/benchmark/initialization_request.h"
#include "message/control/benchmark/initialization_response.h"
#include "message/control/benchmark/startrun_request.h"
#include "message/control/benchmark/startrun_response.h"
#include "message/control/benchmark/switch_slot_request.h"
#include "message/control/benchmark/switch_slot_response.h"
#include "message/control/benchmark/finish_warmup_request.h"
#include "message/control/benchmark/finish_warmup_response.h"
#include "message/control/benchmark/finishrun_request.h"
#include "message/control/benchmark/finishrun_response.h"
#include "message/control/benchmark/simple_finishrun_response.h"

#include "message/control/cooperation/acquire_writelock_request.h"
#include "message/control/cooperation/acquire_writelock_response.h"
#include "message/control/cooperation/directory_lookup_request.h"
#include "message/control/cooperation/directory_update_request.h"
#include "message/control/cooperation/directory_lookup_response.h"
#include "message/control/cooperation/directory_update_response.h"
#include "message/control/cooperation/finish_block_request.h"
#include "message/control/cooperation/finish_block_response.h"
#include "message/control/cooperation/release_writelock_request.h"
#include "message/control/cooperation/release_writelock_response.h"
#include "message/control/cooperation/invalidation_request.h"
#include "message/control/cooperation/invalidation_response.h"

// ONLY for COVERED
#include "message/control/cooperation/covered_directory_lookup_request.h"
#include "message/control/cooperation/covered_directory_lookup_response.h"
#include "message/control/cooperation/covered_directory_update_request.h"
#include "message/control/cooperation/covered_directory_update_response.h"
#include "message/control/cooperation/covered_acquire_writelock_request.h"
#include "message/control/cooperation/covered_acquire_writelock_response.h"
#include "message/control/cooperation/covered_release_writelock_request.h"
#include "message/control/cooperation/covered_release_writelock_response.h"
// For cooperative cache placement management (hybrid data fetching)
#include "message/control/cooperation/covered_placement_directory_lookup_response.h"
// For cooperative cache placement management (during non-blocking placement deployment)
#include "message/control/cooperation/covered_placement_directory_update_request.h"
#include "message/control/cooperation/covered_placement_directory_update_response.h"
// For cooperative cache placement management (non-blocking placement notification)
#include "message/control/cooperation/covered_placement_notify_request.h"
// For cooperative cache placement management (lazy victim fetching for placement calculation)
#include "message/control/cooperation/covered_victim_fetch_request.h"
#include "message/control/cooperation/covered_victim_fetch_response.h"

#endif