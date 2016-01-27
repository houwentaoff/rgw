/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting_rest.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015年12月11日 13时51分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef __PORTING_REST_H__
#define __PORTING_REST_H__

#include "porting_common.h"
#include "porting_op.h"

extern std::map<std::string, std::string> rgw_to_http_attrs;

class RGWRESTMgr {
    bool should_log;
    protected:
    map<string, RGWRESTMgr *> resource_mgrs;
    multimap<size_t, string> resources_by_size;
    RGWRESTMgr *default_mgr;

    public:
    RGWRESTMgr() : should_log(false), default_mgr(NULL) {}
    virtual ~RGWRESTMgr();

    void register_resource(string resource, RGWRESTMgr *mgr);
    void register_default_mgr(RGWRESTMgr *mgr);

    virtual RGWRESTMgr *get_resource_mgr(struct req_state *s, const string& uri, string *out_uri);
    virtual RGWHandler *get_handler(struct req_state *s) { return NULL; }
    virtual void put_handler(RGWHandler *handler) { delete handler; }

    void set_logging(bool _should_log) { should_log = _should_log; }
    bool get_logging() { return should_log; }
};

class RGWREST {
    RGWRESTMgr mgr;

    static int preprocess(struct req_state *s, RGWClientIO *cio);
    public:
    RGWREST() {}
    RGWHandler *get_handler(RGWRados *store, struct req_state *s, RGWClientIO *cio,
            RGWRESTMgr **pmgr, int *init_error);
    void put_handler(RGWHandler *handler) {
        mgr.put_handler(handler);
    }

    void register_resource(string resource, RGWRESTMgr *m, bool register_empty = false) {
        if (!register_empty && resource.empty())
            return;

        mgr.register_resource(resource, m);
    }
    void register_default_mgr(RGWRESTMgr *m) {
        mgr.register_default_mgr(m);
    }
};
class RGWHandler_ObjStore : public RGWHandler {
    protected:
        virtual bool is_obj_update_op() { return false; }
        virtual RGWOp *op_get() { return NULL; }
        virtual RGWOp *op_put() { return NULL; }
        virtual RGWOp *op_delete() { return NULL; }
        virtual RGWOp *op_head() { return NULL; }
        virtual RGWOp *op_post() { return NULL; }
        virtual RGWOp *op_copy() { return NULL; }
        virtual RGWOp *op_options() { return NULL; }

        virtual int validate_bucket_name(const string& bucket);
        virtual int validate_object_name(const string& object);

        static int allocate_formatter(struct req_state *s, int default_formatter, bool configurable);
    public:
        RGWHandler_ObjStore() {}
        virtual ~RGWHandler_ObjStore() {}
        int read_permissions(RGWOp *op);

        virtual int authorize() = 0;
};
class RGWListBuckets_ObjStore : public RGWListBuckets {
public:
  RGWListBuckets_ObjStore() {}
  ~RGWListBuckets_ObjStore() {}
};

class RGWListBucket_ObjStore : public RGWListBucket {
public:
  RGWListBucket_ObjStore() {}
  ~RGWListBucket_ObjStore() {}
};

class RGWGetObj_ObjStore : public RGWGetObj
{
protected:
  bool sent_header;
public:
  RGWGetObj_ObjStore() : sent_header(false) {}

  virtual void init(RGWRados *store, struct req_state *s, RGWHandler *h) {
    RGWGetObj::init(store, s, h);
    sent_header = false;
  }

  int get_params();
};

class RGWCreateBucket_ObjStore : public RGWCreateBucket {
public:
  RGWCreateBucket_ObjStore() {}
  ~RGWCreateBucket_ObjStore() {}
};

class RGWDeleteBucket_ObjStore : public RGWDeleteBucket {
public:
  RGWDeleteBucket_ObjStore() {}
  ~RGWDeleteBucket_ObjStore() {}
};

class RGWPutObj_ObjStore : public RGWPutObj
{
public:
  RGWPutObj_ObjStore() {}
  ~RGWPutObj_ObjStore() {}

  virtual int verify_params();
  virtual int get_params();
  int get_data(bufferlist& bl);
};

class RGWInitMultipart_ObjStore : public RGWInitMultipart {
public:
  RGWInitMultipart_ObjStore() {}
  ~RGWInitMultipart_ObjStore() {}
};
class RGWDeleteObj_ObjStore : public RGWDeleteObj {
public:
  RGWDeleteObj_ObjStore() {}
  ~RGWDeleteObj_ObjStore() {}
};
class RGWStatBucket_ObjStore : public RGWStatBucket {
public:
  RGWStatBucket_ObjStore() {}
  ~RGWStatBucket_ObjStore() {}
};
class RGWGetACLs_ObjStore : public RGWGetACLs {
public:
  RGWGetACLs_ObjStore() {}
  ~RGWGetACLs_ObjStore() {}
};
static const int64_t NO_CONTENT_LENGTH = -1;

extern void set_req_state_err(struct req_state *s, int err_no);
extern void dump_errno(struct req_state *s);
extern void dump_errno(struct req_state *s, int ret);
extern void dump_start(struct req_state *s);
extern void end_header(struct req_state *s,
                       RGWOp *op = NULL,
                       const char *content_type = NULL,
                       const int64_t proposed_content_length = NO_CONTENT_LENGTH,
		       bool force_content_type = false);
extern void list_all_buckets_start(struct req_state *s);
extern void dump_owner(struct req_state *s, string& id, string& name, const char *section = NULL);
extern void list_all_buckets_end(struct req_state *s);
extern void dump_time(struct req_state *s, const char *name, time_t *t);
extern void rgw_flush_formatter(struct req_state *s,
                                         ceph::Formatter *formatter);
extern void rgw_flush_formatter_and_reset(struct req_state *s,
					 ceph::Formatter *formatter);
extern void dump_continue(struct req_state *s);
extern void dump_bucket_from_state(struct req_state *s);


extern void abort_early(struct req_state *s, RGWOp *op, int err);
extern void dump_redirect(struct req_state *s, const string& redirect);

extern void dump_range(struct req_state *s, uint64_t ofs, uint64_t end, uint64_t total_size);

extern void dump_epoch_header(struct req_state *s, const char *name, time_t t);
extern void dump_content_length(struct req_state *s, uint64_t len);
extern void dump_last_modified(struct req_state *s, time_t t);
extern void dump_etag(struct req_state *s, const char *etag);
extern void dump_time_header(struct req_state *s, const char *name, time_t t);
extern void rgw_rest_init(CephContext *cct, void * /*RGWRegion& */region);
extern void dump_string_header(struct req_state *s, const char *name, const char *val);

#endif
