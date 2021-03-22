#include <cga/lib/utility.hpp>
#include <pthread.h>

void cga::thread_role::set_os_name (std::string thread_name)
{
	pthread_setname_np (thread_name.c_str ());
}
