/*
 * \brief  Utility for synchronizing the access of interface methods
 * \author Norman Feske
 * \date   2013-05-16
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BASE__SYNCED_INTERFACE_H_
#define _INCLUDE__BASE__SYNCED_INTERFACE_H_

/* Genode includes */
#include <base/mutex.h>

namespace Genode {

	template <typename, typename LOCK = Genode::Mutex> class Synced_interface;
}


/*
 * Utility for synchronizing the access of interface methods
 *
 * The 'Synced_interface' utility makes the serialization of interface
 * method calls easy. The 'Synced_interface' is a functor that takes a lock
 * and a pointer to an interface as arguments. When called, the functor
 * returns a smart pointer to the interface. When this smart pointer gets
 * dereferenced, the smart pointer takes care of acquiring and releasing
 * the lock while the interface method is executed.
 */
template <typename IF, typename LOCK>
class Genode::Synced_interface
{
	public:

		class Guard
		{
			private:

				LOCK &_lock;
				IF   *_interface;

				Guard(LOCK &lock, IF *interface)
				: _lock(lock), _interface(interface)
				{
					_lock.acquire();
				}

				friend class Synced_interface;

				Guard &operator = (Guard const &);

			public:

				~Guard() { _lock.release(); }

				Guard(Guard const &other)
				: _lock(other._lock), _interface(other._interface) { }

				IF *operator -> () { return _interface; }
		};

	private:

		LOCK &_lock;
		IF   *_interface;

	public:

		Synced_interface(LOCK &lock, IF *interface)
		: _lock(lock), _interface(interface) { }

		Guard operator () ()
		{
			return Guard(_lock, _interface);
		}

		Guard operator () () const
		{
			return Guard(_lock, _interface);
		}
};

#endif /* _INCLUDE__BASE__SYNCED_INTERFACE_H_ */
