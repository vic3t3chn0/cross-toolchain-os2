/*
 * \brief  Interfaces to library-global objects
 * \author Norman Feske
 * \date   2016-04-29
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BASE__INTERNAL__GLOBALS_H_
#define _INCLUDE__BASE__INTERNAL__GLOBALS_H_

#include <parent/parent.h>

namespace Genode {

	class Region_map;
	class Ram_allocator;
	class Env;
	class Local_session_id_space;

	extern Region_map    *env_stack_area_region_map;
	extern Ram_allocator *env_stack_area_ram_allocator;

	Thread_capability main_thread_cap();

	void init_stack_area();
	void init_exception_handling(Env &);
	void init_signal_transmitter(Env &);
	void init_cxx_heap(Env &);
	void init_cxx_guard();
	void init_ldso_phdr(Env &);
	void init_signal_thread(Env &);
	void init_root_proxy(Env &);
	void init_tracing(Env &);
	void init_log(Parent &);
	void init_exit(Parent &);
	void init_parent_resource_requests(Env &);
	void init_heartbeat_monitoring(Env &);
	void deinit_heartbeat_monitoring();
	void exec_static_constructors();

	void destroy_signal_thread();

	void cxx_demangle(char const*, char*, size_t);
	void cxx_current_exception(char *out, size_t size);
	void cxx_free_tls(void *thread);

	Id_space<Parent::Client> &env_session_id_space();
	Env &internal_env();

	void binary_ready_hook_for_platform();
}

void genode_exit(int);

#endif /* _INCLUDE__BASE__INTERNAL__GLOBALS_H_ */
