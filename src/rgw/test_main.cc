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
#include "common/Throttle.h"
#include "common/QueueRing.h"
#include "common/safe_io.h"
#include "include/str_list.h"
#include "porting_op.h"
#include "porting_rest.h"
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

class RGWProcess;

struct RGWProcessEnv {
  RGWRados *store;
  RGWREST *rest;
//  OpsLogSocket *olog;
  int *olog;
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

class RGWProcess {
  deque<RGWRequest *> m_req_queue;
protected:
  RGWRados *store;
//  OpsLogSocket *olog;
  int *olog;
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
      process->req_throttle.put(1);
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
	     /*g_conf->rgw_op_thread_suicide_timeout*/0, &m_tp) {}
  virtual ~RGWProcess() {}
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

class RGWFCGXFrontend : public RGWProcessFrontend {
public:
  RGWFCGXFrontend(RGWProcessEnv& pe, RGWFrontendConfig *_conf) : RGWProcessFrontend(pe, _conf) {}

  int init() {
//    pprocess = new RGWFCGXProcess(g_ceph_context, &env, g_conf->rgw_thread_pool_size, conf);
    return 0;
  }
};


class RGWFCGXProcess : public RGWProcess {
  int max_connections;
public:
  RGWFCGXProcess(CephContext *cct, RGWProcessEnv *pe, int num_threads, RGWFrontendConfig *_conf) :
    RGWProcess(cct, pe, num_threads, _conf),
    max_connections(num_threads + (num_threads >> 3)) /* have a bit more connections than threads so that requests
                                                       are still accepted even if we're still processing older requests */
    {}
  void run();
  void handle_request(RGWRequest *req);
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
//    rest.register_default_mgr(set_logging(new RGWRESTMgr_S3));
//    register_async_signal_handler(SIGHUP, sighup_handler);
//    register_async_signal_handler(SIGTERM, handle_sigterm);
//    register_async_signal_handler(SIGINT, handle_sigterm);
//    register_async_signal_handler(SIGUSR1, handle_sigterm);

    RGWFrontendConfig *config;
    RGWFrontend *fe;    
    RGWProcessEnv fcgi_pe = { store, &rest, 0/*olog*/, 0 };

    fe = new RGWFCGXFrontend(fcgi_pe, config);
    r = fe->init();
    if (r < 0) {
      derr << "ERROR: failed initializing frontend" << dendl;
      return -r;
    }
    fe->run();

    return EXIT_SUCCESS;
}
