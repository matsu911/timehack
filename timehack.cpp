/* -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <sys/time.h>
#include <iostream>
#include <dlfcn.h>
#include <boost/thread.hpp>
// #include <boost/foreach.hpp>

#include <execinfo.h>
#include <vector>
#include <string>

std::vector<std::string> get_call_stack()
{
  try
  {
    void *trace[128];
    size_t size = backtrace(trace, sizeof(trace) / sizeof(void*));

    std::vector<std::string> ret;
    Dl_info info;
    for(size_t i = 1; i < size; ++i) // skip myself
    {
      if(dladdr(trace[i], &info) && info.dli_sname)
        ret.push_back(info.dli_sname);
    }

    return ret;
  }
  // catch(std::logic_error & e)
  // {
  //   std::cerr << e.what() << " in " << __PRETTY_FUNCTION__ << std::endl;
  // }
  catch(...)
  {
    std::cerr << "unknown error in " << __PRETTY_FUNCTION__ << std::endl;
  }
}

static class custom_gettimeofday
{
  mutable boost::mutex _mutex;

  typedef int (*gettimeofday_t)(struct timeval *__restrict, __timezone_ptr_t);

  void *         _handle;
  gettimeofday_t _gettimeofday;

  __time_t      _sec;
  __suseconds_t _usec;
  bool          _use_custom_time;

public:
  custom_gettimeofday() :
    _handle(NULL),
    _sec(0UL),
    _usec(0UL),
    _use_custom_time(false)
  {
    _handle = dlopen("/lib64/libc.so.6", RTLD_LAZY);

    if(!_handle) 
    {
      std::cerr << "Cannot open library: " << dlerror() << std::endl;
      return;
    }

    typedef int (*gettimeofday_t)(struct timeval *__restrict, __timezone_ptr_t);

    _gettimeofday = (gettimeofday_t) dlsym(_handle, "gettimeofday");

    if(!_gettimeofday)
    {
      std::cerr << "Cannot load symbol 'gettimeofday': " << dlerror() << std::endl;
      dlclose(_handle);
      return;
    }
  }

  ~custom_gettimeofday()
  {
    if(!_handle)
      dlclose(_handle);
  }

  inline void get(__time_t * sec, __suseconds_t * usec) const
  {
    boost::mutex::scoped_lock lock(_mutex);
    *sec  = _sec;
    *usec = _usec;
  }

  inline void set(const __time_t & sec, const __suseconds_t & usec)
  {
    boost::mutex::scoped_lock lock(_mutex);
    _sec  = sec;
    _usec = usec;
  }

  inline void use_custom_gettimeofday(bool use)
  {
    boost::mutex::scoped_lock lock(_mutex);
    _use_custom_time = use;
  }

  inline int gettimeofday(struct timeval *__restrict __tv,
                          __timezone_ptr_t __tz) const
  {
    bool use_custom;

    {
      boost::mutex::scoped_lock lock(_mutex);
      use_custom = _use_custom_time;
    }

    if(use_custom)
    {
      std::vector<std::string> call_stack = get_call_stack();

      // called by tibrv
      if(std::find(call_stack.begin(), call_stack.end(), "__tibrvCondition_TimedWait") != call_stack.end() ||
         std::find(call_stack.begin(), call_stack.end(), "_tibrv_GetCurrentTime") != call_stack.end() ||
         std::find(call_stack.begin(), call_stack.end(), "_tibrv_GetHostByName") != call_stack.end())
      {
        return _gettimeofday(__tv, __tz);
      }

      // BOOST_FOREACH(const std::string &s, get_call_stack())
      // {
      //   std::cout << s << std::endl;
      // }

      // __tibrvCondition_TimedWait
      //   _tibrv_GetCurrentTime
      //   _tibrv_GetHostByName

      get(&__tv->tv_sec, &__tv->tv_usec);
      return 0;
    }
    else
    {
      return _gettimeofday(__tv, __tz);
    }
  }

} custom_gettimeofday_obj;

extern "C"
{
  void use_custom_gettimeofday(bool use)
  {
    custom_gettimeofday_obj.use_custom_gettimeofday(use);
  }

  void set_custom_gettimeofday(const __time_t & sec, const __suseconds_t & usec)
  {
    custom_gettimeofday_obj.set(sec, usec);
  }

  int gettimeofday (struct timeval *__restrict __tv,
                    __timezone_ptr_t __tz)
  {
    return custom_gettimeofday_obj.gettimeofday(__tv, __tz);
  }
}
