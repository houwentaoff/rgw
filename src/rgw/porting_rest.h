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


#endif
