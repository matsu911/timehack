#include <sys/time.h>
#include <iostream>
#include <dlfcn.h>

int main()
{
  struct timeval tv;

  typedef void (*use_custom_gettimeofday_t)(bool);
  typedef void (*set_custom_gettimeofday_t)(const __time_t &, const __suseconds_t &);

  use_custom_gettimeofday_t use_custom_gettimeofday = 0;
  set_custom_gettimeofday_t set_custom_gettimeofday = 0;

  void * handle = dlopen(NULL, RTLD_LAZY);

  if(!handle) 
  {
    std::cerr << "Cannot open library: " << dlerror() << std::endl;
    // return 1;
  }

  use_custom_gettimeofday = (use_custom_gettimeofday_t) dlsym(handle, "use_custom_gettimeofday");
  set_custom_gettimeofday = (set_custom_gettimeofday_t) dlsym(handle, "set_custom_gettimeofday");

  if(use_custom_gettimeofday)
    use_custom_gettimeofday(true);

  gettimeofday(&tv, 0);

  std::cout << tv.tv_sec << " " << tv.tv_usec << std::endl;

  if(set_custom_gettimeofday)
    set_custom_gettimeofday(1287113815, 752507);

  gettimeofday(&tv, 0);

  std::cout << tv.tv_sec << " " << tv.tv_usec << std::endl;

  if(use_custom_gettimeofday)
    use_custom_gettimeofday(false);

  gettimeofday(&tv, 0);

  std::cout << tv.tv_sec << " " << tv.tv_usec << std::endl;

  if(handle)
    dlclose(handle);

  return 0;
}
