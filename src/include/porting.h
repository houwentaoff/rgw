/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  porting.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015年12月10日 14时55分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef __PORTING_H__
#define __PORTING_H__
#include <string>
namespace ceph {
    namespace log {
    }
}
class CephContext
{
    public:
        CephContext(){};
        CephContext(uint32_t module_type_, int init_flags_ = 0);
        
        ~CephContext(){};
//        ceph::log::Log *_log;
        CephContext(const CephContext &rhs);        
        CephContext &operator=(const CephContext &rhs);        
        
};
class PerfCounters
{
    public:
        PerfCounters(){};
        ~PerfCounters(){};
};
class PerfCountersBuilder
{
    public:
        PerfCountersBuilder(CephContext *cct, const std::string &name,
                int first, int last){};
    private:
        ~PerfCountersBuilder(){};
};
class heartbeat_handle_d
{
    
};
class RGWAccessControlPolicy
{

};
struct cls_user_bucket
{
    std::string name;
    std::string data_pool;
    std::string index_pool;
    std::string marker;
    std::string bucket_id;
    std::string data_extra_pool;
};
class Throttle 
{
    public:
        Throttle(){};
        Throttle(CephContext *cct, const std::string& n, int64_t m = 0, bool _use_perf = true){};
        ~Throttle(){};
};
class OpsLogSocket
{
    public:
        OpsLogSocket(){}
        ~OpsLogSocket(){}
};
#if 0
class   RGWEnv
{

};
#endif
class RGWRados
{

};

#if 0 
/* ************************************ use  *********************************/
/* rgw_common_porting */
/* * Store error returns for output at a different point in the program */
struct rgw_err {
    rgw_err();
    rgw_err(int http, const std::string &s3);
    void clear();
    bool is_clear() const;
    bool is_err() const;
    friend std::ostream& operator<<(std::ostream& oss, const rgw_err &err);

    int http_ret;
    int ret;
    std::string s3_code;
    std::string message;
};
struct req_info {
//    RGWEnv *env;
    RGWHTTPArgs args;
    map<string, string> x_meta_map;

    string host;
    const char *method;
    string script_uri;
    string request_uri;
    string effective_uri;
    string request_params;
    string domain;

    req_info(CephContext *cct, RGWEnv *_env);
    void rebuild_from(req_info& src);
    void init_meta_info(bool *found_bad_meta);
};
}
/* * Store all the state necessary to complete and respond to an HTTP request*/
struct req_state {
    CephContext *cct;
    RGWClientIO *cio;
    http_op op;
    bool content_started;
    int format;
    ceph::Formatter *formatter;
    string decoded_uri;
    string relative_uri;
    const char *length;
    int64_t content_length;
    map<string, string> generic_attrs;
    struct rgw_err err;
    bool expect_cont;
    bool header_ended;
    uint64_t obj_size;
    bool enable_ops_log;
    bool enable_usage_log;
    uint8_t defer_to_bucket_acls;
    uint32_t perm_mask;
    utime_t header_time;

//    rgw_bucket bucket;
    string bucket_name_str;
//    rgw_obj_key object;
    string src_bucket_name;
//    rgw_obj_key src_object;
//    ACLOwner bucket_owner;
//    ACLOwner owner;

    string region_endpoint;
    string bucket_instance_id;

//    RGWBucketInfo bucket_info;
    map<string, bufferlist> bucket_attrs;
    bool bucket_exists;

    bool has_bad_meta;

//    RGWUserInfo user; 
//    RGWAccessControlPolicy *bucket_acl;
//    RGWAccessControlPolicy *object_acl;

    bool system_request;

    string canned_acl;
    bool has_acl_header;
    const char *copy_source;
    const char *http_auth;
    bool local_source; /*  source is local */
    
    int prot_flags;

    const char *os_auth_token;
    string swift_user;
    string swift_groups;

    utime_t time;

    void *obj_ctx;

    string dialect;

    string req_id;

    string trans_id;

    req_info info;

    req_state(CephContext *_cct, class RGWEnv *e);
    ~req_state();
};

#if  0 
class atomic_t
{
    atomic_t(){}:
        ~atomic_t(){};
};
#endif
#endif
#endif
