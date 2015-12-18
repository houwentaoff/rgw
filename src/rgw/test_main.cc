/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  test_main.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015年12月09日 10时00分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <curl/curl.h>

#include "acconfig.h"
#ifdef FASTCGI_INCLUDE_DIR
# include "fastcgi/fcgiapp.h"
#else
# include "fcgiapp.h"
#endif

#include "rgw_fcgi.h"
//#include "common/errno.h"
#include "common/WorkQueue.h"
#include "common/Timer.h"
//#include "common/Throttle.h"
#include "common/QueueRing.h"
#include "common/safe_io.h"
#include "include/str_list.h"
#include "porting_op.h"
#include "porting_rest.h"
#include "porting_rest_s3.h"
#include "porting_rest_admin.h"
#include "porting_common.h"

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <deque>
#include "include/types.h"
#include "common/BackTrace.h"
#include <errno.h>
#include "porting_rest_bucket.h"

#define SOCKET_BACKLOG 1024

CephContext *g_ceph_context = NULL;

class RGWProcess;

struct RGWProcessEnv {
  RGWRados *store;
  RGWREST *rest;
  OpsLogSocket *olog;
//  int *olog;
  int port;
};

struct RGWRequest
{
  uint64_t id;
  struct req_state *s;
  string req_str;
  RGWOp *op;
  utime_t ts;

  RGWRequest(uint64_t id) : id(id), s(NULL), op(NULL) {
  }

  virtual ~RGWRequest() {}

  void init_state(req_state *_s) {
    s = _s;
  }

  void log_format(struct req_state *s, const char *fmt, ...)
  {
#define LARGE_SIZE 1024
    char buf[LARGE_SIZE];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    log(s, buf);
  }

  void log_init() {
//    ts = ceph_clock_now(g_ceph_context);
  }

  void log(struct req_state *s, const char *msg) {
    if (s->info.method && req_str.size() == 0) {
      req_str = s->info.method;
      req_str.append(" ");
      req_str.append(s->info.request_uri);
    }
//    utime_t t = ceph_clock_now(g_ceph_context) - ts;
//    dout(2) << "req " << id << ":" << t << ":" << s->dialect << ":" << req_str << ":" << (op ? op->name() : "") << ":" << msg << dendl;
  }
};
class RGWFrontendConfig {
  string config;
  map<string, string> config_map;
  int parse_config(const string& config, map<string, string>& config_map);
  string framework;
public:
  RGWFrontendConfig(const string& _conf) : config(_conf) {}
  int init() {
    int ret = parse_config(config, config_map);
    if (ret < 0)
      return ret;
    return 0;
  }
  bool get_val(const string& key, const string& def_val, string *out);
  bool get_val(const string& key, int def_val, int *out);

  map<string, string>& get_config_map() { return config_map; }

  string get_framework() { return framework; }
};
int RGWFrontendConfig::parse_config(const string& config, map<string, string>& config_map)
{
  list<string> config_list;
  get_str_list(config, " ", config_list);

  list<string>::iterator iter;
  for (iter = config_list.begin(); iter != config_list.end(); ++iter) {
    string& entry = *iter;
    string key;
    string val;

    if (framework.empty()) {
      framework = entry;
      dout(0) << "framework: " << framework << dendl;
      continue;
    }

    ssize_t pos = entry.find('=');
    if (pos < 0) {
      dout(0) << "framework conf key: " << entry << dendl;
      config_map[entry] = "";
      continue;
    }

    int ret = parse_key_value(entry, key, val);
    if (ret < 0) {
      cerr << "ERROR: can't parse " << entry << std::endl;
      return ret;
    }

    dout(0) << "framework conf key: " << key << ", val: " << val << dendl;
    config_map[key] = val;
  }

  return 0;
}

bool RGWFrontendConfig::get_val(const string& key, const string& def_val, string *out)
{
 map<string, string>::iterator iter = config_map.find(key);
 if (iter == config_map.end()) {
   *out = def_val;
   return false;
 }

 *out = iter->second;
 return true;
}

bool RGWFrontendConfig::get_val(const string& key, int def_val, int *out)
{
  string str;
  bool found = get_val(key, "", &str);
  if (!found) {
    *out = def_val;
    return false;
  }
  string err;
  *out = strict_strtol(str.c_str(), 10, &err);
  if (!err.empty()) {
    cerr << "error parsing int: " << str << ": " << err << std::endl;
    return -EINVAL;
  }
  return 0;
}

class RGWProcess {
  deque<RGWRequest *> m_req_queue;
protected:
  RGWRados *store;
  OpsLogSocket *olog;
  ThreadPool m_tp;
  Throttle req_throttle;
  RGWREST *rest;
  
  RGWFrontendConfig *conf;
  int sock_fd;

  struct RGWWQ : public ThreadPool::WorkQueue<RGWRequest> {
    RGWProcess *process;
    RGWWQ(RGWProcess *p, time_t timeout, time_t suicide_timeout, ThreadPool *tp)
      : ThreadPool::WorkQueue<RGWRequest>("RGWWQ", timeout, suicide_timeout, tp), process(p) {}

    bool _enqueue(RGWRequest *req) {
      process->m_req_queue.push_back(req);
//      perfcounter->inc(l_rgw_qlen);
      dout(20) << "enqueued request req=" << hex << req << dec << dendl;
      _dump_queue();
      return true;
    }
    void _dequeue(RGWRequest *req) {
      assert(0);
    }
    bool _empty() {
      return process->m_req_queue.empty();
    }
    RGWRequest *_dequeue() {
      if (process->m_req_queue.empty())
	return NULL;
      RGWRequest *req = process->m_req_queue.front();
      process->m_req_queue.pop_front();
      dout(20) << "dequeued request req=" << hex << req << dec << dendl;
      _dump_queue();
//      perfcounter->inc(l_rgw_qlen, -1);
      return req;
    }
    using ThreadPool::WorkQueue<RGWRequest>::_process;
    void _process(RGWRequest *req) {
//      perfcounter->inc(l_rgw_qactive);
      process->handle_request(req);
//      process->req_throttle.put(1);
//      perfcounter->inc(l_rgw_qactive, -1);
    }
    void _dump_queue() {
//      if (!g_conf->subsys.should_gather(ceph_subsys_rgw, 20)) {
//        return;
//      }
      deque<RGWRequest *>::iterator iter;
      if (process->m_req_queue.empty()) {
        dout(20) << "RGWWQ: empty" << dendl;
        return;
      }
      dout(20) << "RGWWQ:" << dendl;
      for (iter = process->m_req_queue.begin(); iter != process->m_req_queue.end(); ++iter) {
        dout(20) << "req: " << hex << *iter << dec << dendl;
      }
    }
    void _clear() {
      assert(process->m_req_queue.empty());
    }
  } req_wq;

public:
  RGWProcess(CephContext *cct, RGWProcessEnv *pe, int num_threads, RGWFrontendConfig *_conf)
    : store(pe->store), olog(pe->olog), m_tp(cct, "RGWProcess::m_tp", num_threads),
      req_throttle(cct, "rgw_ops", num_threads * 2),
      rest(pe->rest),
      conf(_conf),
      sock_fd(-1),
      req_wq(this, 0/*  g_conf->rgw_op_thread_timeout*/,
	     /*g_conf->rgw_op_thread_suicide_timeout*/0, &m_tp) {};
  virtual ~RGWProcess() {};
  virtual void run() = 0;
  virtual void handle_request(RGWRequest *req) = 0;

  void close_fd() {
    if (sock_fd >= 0) {
      ::close(sock_fd);
      sock_fd = -1;
    }
  }
};



class RGWProcessControlThread : public Thread {
  RGWProcess *pprocess;
public:
  RGWProcessControlThread(RGWProcess *_pprocess) : pprocess(_pprocess) {}

  void *entry() {
    pprocess->run();
    return NULL;
  }
};


class RGWFrontend {
public:
  virtual ~RGWFrontend() {}

  virtual int init() = 0;

  virtual int run() = 0;
  virtual void stop() = 0;
  virtual void join() = 0;

};

class RGWProcessFrontend : public RGWFrontend {
protected:
  RGWFrontendConfig *conf;
  RGWProcess *pprocess;
  RGWProcessEnv env;
  RGWProcessControlThread *thread;

public:
  RGWProcessFrontend(RGWProcessEnv& pe, RGWFrontendConfig *_conf) : conf(_conf), pprocess(NULL), env(pe), thread(NULL) {
  }

  ~RGWProcessFrontend() {
    delete thread;
    delete pprocess;
  }

  int run() {
//    assert(pprocess); /* should have initialized by init() */
    thread = new RGWProcessControlThread(pprocess);
    thread->create();
    return 0;
  }

  void stop() {
    pprocess->close_fd();
    thread->kill(SIGUSR1);
  }

  void join() {
    thread->join();
  }
};

struct RGWFCGXRequest : public RGWRequest {
  FCGX_Request *fcgx;
  QueueRing<FCGX_Request *> *qr;

  RGWFCGXRequest(uint64_t req_id, QueueRing<FCGX_Request *> *_qr) : RGWRequest(req_id), qr(_qr) {
    qr->dequeue(&fcgx);
  }

  ~RGWFCGXRequest() {
    FCGX_Finish_r(fcgx);
    qr->enqueue(fcgx);
  }
};

class RGWFCGXProcess : public RGWProcess {
  int max_connections;
public:

  RGWFCGXProcess(CephContext *cct, RGWProcessEnv *pe, int num_threads, RGWFrontendConfig *_conf) :
    RGWProcess(cct, pe, num_threads, _conf),
    max_connections(num_threads + (num_threads >> 3)){} /* have a bit more connections than threads so that requests
                                                     are still accepted even if we're still processing older requests */   
  void run();
  void handle_request(RGWRequest *req);
};
void RGWFCGXProcess::run()
{
  string socket_path;
  string socket_port;
  string socket_host;

  conf->get_val("socket_path", "/rgw.sock", &socket_path);
  conf->get_val("socket_port", "9000"/*g_conf->rgw_port*/, &socket_port);
  conf->get_val("socket_host", "127.0.0.1"/*g_conf->rgw_host*/, &socket_host);

  if (socket_path.empty() && socket_port.empty() && socket_host.empty()) {
    socket_path = "/rgw_socket_path";//g_conf->rgw_socket_path;
    if (socket_path.empty()) {
      dout(0) << "ERROR: no socket server point defined, cannot start fcgi frontend" << dendl;
      return;
    }
  }

  if (!socket_path.empty()) {
    string path_str = socket_path;

    /* this is necessary, as FCGX_OpenSocket might not return an error, but rather ungracefully exit */
    int fd = open(path_str.c_str(), O_CREAT, 0644);
    if (fd < 0) {
      int err = errno;
      /* ENXIO is actually expected, we'll get that if we try to open a unix domain socket */
      if (err != ENXIO) {
        dout(0) << "ERROR: cannot create socket: path=" << path_str << " error=" << "cpp_strerror"/*cpp_strerror(err)*/ << dendl;
        return;
      }
    } else {
      close(fd);
    }

    const char *path = path_str.c_str();
    sock_fd = FCGX_OpenSocket(path, SOCKET_BACKLOG);
    if (sock_fd < 0) {
      dout(0) << "ERROR: FCGX_OpenSocket (" << path << ") returned " << sock_fd << dendl;
      return;
    }
    if (chmod(path, 0777) < 0) {
      dout(0) << "WARNING: couldn't set permissions on unix domain socket" << dendl;
    }
  } else if (!socket_port.empty()) {
    string bind = socket_host + ":" + socket_port;
    sock_fd = FCGX_OpenSocket(bind.c_str(), SOCKET_BACKLOG);
    if (sock_fd < 0) {
      dout(0) << "ERROR: FCGX_OpenSocket (" << bind.c_str() << ") returned " << sock_fd << dendl;
      return;
    }
  }

  m_tp.start();

  FCGX_Request fcgx_reqs[max_connections];

  QueueRing<FCGX_Request *> qr(max_connections);
  for (int i = 0; i < max_connections; i++) {
    FCGX_Request *fcgx = &fcgx_reqs[i];
    FCGX_InitRequest(fcgx, sock_fd, 0);
    qr.enqueue(fcgx);
  }

  for (;;) {
    RGWFCGXRequest *req = new RGWFCGXRequest(100/*store->get_new_req_id()*/, &qr);
    dout(10) << "allocated request req=" << hex << req << dec << dendl;
    //req_throttle.get(1);
    int ret = FCGX_Accept_r(req->fcgx);
    if (ret < 0) {
      delete req;
      dout(0) << "ERROR: FCGX_Accept_r returned " << ret << dendl;
      //req_throttle.put(1);
      break;
    }

    req_wq.queue(req);
  }

  m_tp.drain(&req_wq);
  m_tp.stop();

  dout(20) << "cleaning up fcgx connections" << dendl;

  for (int i = 0; i < max_connections; i++) {
    FCGX_Finish_r(&fcgx_reqs[i]);
  }
}
static int process_request(RGWRados *store, RGWREST *rest, RGWRequest *req, RGWClientIO *client_io, OpsLogSocket *olog);

void RGWFCGXProcess::handle_request(RGWRequest *r)
{
  RGWFCGXRequest *req = static_cast<RGWFCGXRequest *>(r);
  FCGX_Request *fcgx = req->fcgx;
  RGWFCGX client_io(fcgx);

 
  int ret = process_request(store, rest, req, &client_io, olog);
  if (ret < 0) {
    /* we don't really care about return code */
    dout(20) << "process_request() returned " << ret << dendl;
  }

  FCGX_Finish_r(fcgx);

  delete req;
}

class RGWFCGXFrontend : public RGWProcessFrontend {
public:
  RGWFCGXFrontend(RGWProcessEnv& pe, RGWFrontendConfig *_conf) : RGWProcessFrontend(pe, _conf) {}

  int init() {
    pprocess = new RGWFCGXProcess(g_ceph_context, &env, 256/*g_conf->rgw_thread_pool_size*/, conf);
    return 0;
  }
};

void sighup_handler(int signum)
{
//  g_ceph_context->reopen_logs();
}
static void handle_sigterm(int signum)
{
  dout(1) << __func__ << dendl;
  FCGX_ShutdownPending();

  // send a signal to make fcgi's accept(2) wake up.  unfortunately the
  // initial signal often isn't sufficient because we race with accept's
  // check of the flag wet by ShutdownPending() above.
  if (signum != SIGUSR1) {
//    signal_shutdown();

    // safety net in case we get stuck doing an orderly shutdown.
    uint64_t secs = 0;//g_ceph_context->_conf->rgw_exit_timeout_secs;
    if (secs)
      alarm(secs);
    dout(1) << __func__ << " set alarm for " << secs << dendl;
  }

}

static RGWRESTMgr *set_logging(RGWRESTMgr *mgr)
{
      mgr->set_logging(true);
        return mgr;
}

static int process_request(RGWRados *store, RGWREST *rest, RGWRequest *req, RGWClientIO *client_io, OpsLogSocket *olog)
{
  int ret = 0;

  client_io->init(g_ceph_context);

  req->log_init();

  dout(1) << "====== starting new request req=" << hex << req << dec << " =====" << dendl;
//  perfcounter->inc(l_rgw_req);

  RGWEnv& rgw_env = client_io->get_env();

  struct req_state rstate(g_ceph_context, &rgw_env);

  struct req_state *s = &rstate;

  //RGWObjectCtx rados_ctx(store, s);
  //s->obj_ctx = &rados_ctx;

  //s->req_id = store->unique_id(req->id);
  //s->trans_id = store->unique_trans_id(req->id);

  req->log_format(s, "initializing for trans_id = %s", s->trans_id.c_str());

  RGWOp *op = NULL;
  int init_error = 0;
  bool should_log = false;
  RGWRESTMgr *mgr;
  RGWHandler *handler = rest->get_handler(store, s, client_io, &mgr, &init_error);
  if (init_error != 0) {
//    abort_early(s, NULL, init_error);
    goto done;
  }

  should_log = mgr->get_logging();

  req->log(s, "getting op");
  op = handler->get_op(store);
  if (!op) {
//    abort_early(s, NULL, -ERR_METHOD_NOT_ALLOWED);
    goto done;
  }
  req->op = op;

  req->log(s, "authorizing");
  ret = handler->authorize();
  if (ret < 0) {
    dout(10) << "failed to authorize request" << dendl;
  //  abort_early(s, op, ret);
    goto done;
  }
#if 0
  if (s->user.suspended) {
    dout(10) << "user is suspended, uid=" << s->user.user_id << dendl;
    //abort_early(s, op, -ERR_USER_SUSPENDED);
    goto done;
  }
#endif
  req->log(s, "reading permissions");
  ret = handler->read_permissions(op);
  if (ret < 0) {
    //abort_early(s, op, ret);
    goto done;
  }

  req->log(s, "init op");
  ret = op->init_processing();
  if (ret < 0) {
    //abort_early(s, op, ret);
    goto done;
  }

  req->log(s, "verifying op mask");
  ret = op->verify_op_mask();
  if (ret < 0) {
    //abort_early(s, op, ret);
    goto done;
  }

  req->log(s, "verifying op permissions");
  ret = op->verify_permission();
  if (ret < 0) {
    if (s->system_request) {
      dout(2) << "overriding permissions due to system operation" << dendl;
    } else {
      //abort_early(s, op, ret);
      goto done;
    }
  }

  req->log(s, "verifying op params");
  ret = op->verify_params();
  if (ret < 0) {
    //abort_early(s, op, ret);
    goto done;
  }

  req->log(s, "executing");
  op->pre_exec();
  op->execute();
  op->complete();
done:
  int r = client_io->complete_request();
  if (r < 0) {
    dout(0) << "ERROR: client_io->complete_request() returned " << r << dendl;
  }
  if (should_log) {
    ;//rgw_log_op(store, s, (op ? op->name() : "unknown"), olog);
  }

  int http_ret = s->err.http_ret;

  req->log_format(s, "http status=%d", http_ret);

  if (handler)
    handler->put_op(op);
  rest->put_handler(handler);

  dout(1) << "====== req done req=" << hex << req << dec << " http_status=" << http_ret << " ======" << dendl;

  return (ret < 0 ? ret : s->err.ret);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
    ldout(0, 0)<<"hello world\n"<<dendl;
//    check_curl();    
    curl_global_init(CURL_GLOBAL_ALL);
  
    FCGX_Init();    
    int r = 0;
    RGWRados *store ;    
    RGWREST rest;
    rest.register_default_mgr(set_logging(new RGWRESTMgr_S3));
    //admin
    RGWRESTMgr_Admin *admin_resource = new RGWRESTMgr_Admin;
//    admin_resource->register_resource("usage", new RGWRESTMgr_Usage);
//    admin_resource->register_resource("user", new RGWRESTMgr_User);
    admin_resource->register_resource("bucket", new RGWRESTMgr_Bucket);    
    /*Registering resource for /admin/metadata*/
#if 0
    admin_resource->register_resource("metadata", new RGWRESTMgr_Metadata);
    admin_resource->register_resource("log", new RGWRESTMgr_Log);
    admin_resource->register_resource("opstate", new RGWRESTMgr_Opstate);
    admin_resource->register_resource("replica_log", new RGWRESTMgr_ReplicaLog);
    admin_resource->register_resource("config", new RGWRESTMgr_Config);
#endif
    rest.register_resource("admin"/*g_conf->rgw_admin_entry*/, admin_resource);
    //    rest.register_default_mgr(set_logging(new RGWRESTMgr_S3));
    //    register_async_signal_handler(SIGHUP, sighup_handler);
    //    register_async_signal_handler(SIGTERM, handle_sigterm);
    //    register_async_signal_handler(SIGINT, handle_sigterm);
//    register_async_signal_handler(SIGUSR1, handle_sigterm);

    RGWFrontendConfig *config = new RGWFrontendConfig("test null");
    RGWFrontend *fe;    
    RGWProcessEnv fcgi_pe = { store, &rest, 0/*olog*/, 0 };

    r = config->init();
    if (r < 0) {
      cerr << "ERROR: failed to init config: " << "f" << std::endl;
      return EINVAL;
    }
    fe = new RGWFCGXFrontend(fcgi_pe, config);
    r = fe->init();
    if (r < 0) {
      derr << "ERROR: failed initializing frontend" << dendl;
      return -r;
    }
    fe->run();
    do{}
    while (true);

    return EXIT_SUCCESS;
}
