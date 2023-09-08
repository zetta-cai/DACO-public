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

#endif