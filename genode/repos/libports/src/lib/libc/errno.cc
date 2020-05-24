/*
 * \brief  C-library back end
 * \author Norman Feske
 * \date   2008-11-11
 *
 * Note: this version not thread safe
 */

/*
 * Copyright (C) 2008-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <internal/pthread.h>

extern "C" int *__error(void)
{
	return &pthread_self()->thread_local_errno;
}
