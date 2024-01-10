//
// Created by Juncheng Yang on 11/17/19.
//

#ifndef libCacheSim_REQUEST_H
#define libCacheSim_REQUEST_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <string> // Siyuan: for key-value caching

#include "enum.h"
#include "logging.h"
#include "mem.h"

// (OBSOLETE) Siyuan: as g++ will not mangle struct non-static member variables, we use extern "C" for C language linkage after declaration of struct request such that we can declare instances of std::string (NOT use covered::Key and covered::Value now)
// Siyuan: as g++ will not mangle struct non-static member variables, compiler will ignore extern "C" even if std::string is NOT supported by gcc
#ifdef __cplusplus
extern "C" {
#endif

/* need to optimize this for CPU cacheline */
typedef struct request {
  int64_t real_time; /* use uint64_t because vscsi uses microsec timestamp */
  uint64_t hv;        /* hash value, used when offloading hash to reader */
  obj_id_t obj_id;
  uint32_t obj_size;
  int32_t ttl;
  req_op_e op;

  uint64_t n_req;
  int64_t next_access_vtime;
  struct {
    uint64_t key_size : 16;
    uint64_t val_size : 48;
  };

  int32_t ns;  // namespace
  int32_t content_type;
  int32_t tenant_id;

  int32_t bucket_id;
  int32_t age;
  int32_t hostname;
  int16_t extension;
  int16_t colo;
  int16_t n_level;
  int16_t n_param;
  int8_t method;

  bool valid; /* indicate whether request is valid request
               * it is invlalid if the trace reaches the end */
  
  // Siyuan: for key-value caching
  // NOTE: sizeof(std::string) can work correctly for sizeof(cache_obj_t) and sizeof(request_t) <- std::string is equivalent to char*, which occupies 8B when using sizeof(cache_obj_t) and sizeof(request_t) to create new cache objects and requests; std::string is responsible for memory allocation/free referred by char* when get/update Key
  bool is_keybased_req; // Use obj_id as key if false, or use key otherwise
  std::string key;
  std::string value;
} request_t;

/**
 * allocate a new request_t struct and fill in necessary field
 * @return
 */
static inline request_t *new_request() {
  request_t *req = my_malloc(request_t);
  memset(req, 0, sizeof(request_t));
  req->obj_size = 1;
  req->op = OP_INVALID;
  req->valid = true;
  req->obj_id = 0;
  req->real_time = 0;
  req->hv = 0;
  req->next_access_vtime = -2;
  return req;
}

/**
 * copy the req_src to req_dest
 * @param req_dest
 * @param req_src
 */
static inline void copy_request(request_t *req_dest, request_t *req_src) {
  memcpy(req_dest, req_src, sizeof(request_t));
}

/**
 * clone the given request
 * @param req
 * @return
 */
static inline request_t *clone_request(request_t *req) {
  request_t *req_new = my_malloc(request_t);
  copy_request(req_new, req);
  return req_new;
}

/**
 * free the memory used by req
 * @param req
 */
static inline void free_request(request_t *req) { my_free(request_t, req); }

static inline void print_request(request_t *req) {
#if defined(SUPPORT_TTL) && SUPPORT_TTL == 1
  INFO("req real_time %lu, id %llu, size %ld, ttl %ld, op %s, valid %d\n",
       (unsigned long)req->real_time, (unsigned long long)req->obj_id,
       (long)req->obj_size, (long)req->ttl, req_op_str[req->op], req->valid);
#else
  printf("req real_time %lu, id %llu, size %ld, op %s, valid %d\n",
         (unsigned long)req->real_time, (unsigned long long)req->obj_id,
         (long)req->obj_size, req_op_str[req->op], req->valid);
#endif
}

#ifdef __cplusplus
}
#endif

#endif  // libCacheSim_REQUEST_H
