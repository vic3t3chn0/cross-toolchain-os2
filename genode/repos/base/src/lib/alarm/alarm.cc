/*
 * \brief   Timed event scheduler
 * \date    2005-10-24
 * \author  Norman Feske
 */

/*
 * Copyright (C) 2005-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/log.h>
#include <base/alarm.h>

using namespace Genode;


void Alarm_scheduler::_unsynchronized_enqueue(Alarm *alarm)
{
	if (alarm->_active) {
		error("trying to insert the same alarm twice!");
		return;
	}

	alarm->_active++;

	/* if alarmlist is empty add first element */
	if (!_head) {
		alarm->_next = 0;
		_head = alarm;
		return;
	}

	/* if deadline is smaller than any other deadline, put it on the head */
	if (alarm->_raw.is_pending_at(_head->_raw.deadline, _head->_raw.deadline_period)) {
		alarm->_next = _head;
		_head = alarm;
		return;
	}

	/* find list element with a higher deadline */
	Alarm *curr = _head;
	while (curr->_next &&
	       curr->_next->_raw.is_pending_at(alarm->_raw.deadline, alarm->_raw.deadline_period))
	{
		curr = curr->_next;
	}

	/* if end of list is reached, append new element */
	if (curr->_next == 0) {
		curr->_next = alarm;
		return;
	}

	/* insert element in middle of list */
	alarm->_next = curr->_next;
	curr->_next  = alarm;
}


void Alarm_scheduler::_unsynchronized_dequeue(Alarm *alarm)
{
	if (!_head) return;

	if (_head == alarm) {
		_head = alarm->_next;
		alarm->_reset();
		return;
	}

	/* find predecessor in alarm queue */
	Alarm *curr;
	for (curr = _head; curr && (curr->_next != alarm); curr = curr->_next);

	/* alarm is not enqueued */
	if (!curr) return;

	/* remove alarm from alarm queue */
	curr->_next = alarm->_next;
	alarm->_reset();
}


bool Alarm::Raw::is_pending_at(uint64_t time, bool time_period) const
{
	return (time_period == deadline_period &&
	        time        >= deadline) ||
	       (time_period != deadline_period &&
	        time        <  deadline);
}


Alarm *Alarm_scheduler::_get_pending_alarm()
{
	Mutex::Guard guard(_mutex);

	if (!_head || !_head->_raw.is_pending_at(_now, _now_period)) {
		return nullptr; }

	/* remove alarm from head of the list */
	Alarm *pending_alarm = _head;
	_head = _head->_next;

	/*
	 * Acquire dispatch mutex to defer destruction until the call of 'on_alarm'
	 * is finished
	 */
	pending_alarm->_dispatch_mutex.acquire();

	/* reset alarm object */
	pending_alarm->_next = nullptr;
	pending_alarm->_active--;

	return pending_alarm;
}


void Alarm_scheduler::handle(Alarm::Time curr_time)
{
	/*
	 * Raise the time counter and if it wraps, update also in which
	 * period of the time counter we are.
	 */
	if (_now > curr_time) {
		_now_period = !_now_period;
	}
	_now = curr_time;

	if (!_min_handle_period.is_pending_at(_now, _now_period)) {
		return;
	}
	Alarm::Time const deadline         = _now + _min_handle_period.period;
	_min_handle_period.deadline        = deadline;
	_min_handle_period.deadline_period = _now > deadline ?
	                                     !_now_period : _now_period;

	Alarm *curr;
	while ((curr = _get_pending_alarm())) {

		uint64_t triggered = 1;

		if (curr->_raw.period) {
			Alarm::Time deadline = curr->_raw.deadline;

			/* schedule next event */
			if (deadline == 0)
				 deadline = curr_time;

			triggered += (curr_time - deadline) / curr->_raw.period;
		}

		/* do not reschedule if alarm function returns 0 */
		bool reschedule = curr->on_alarm(triggered);

		if (reschedule) {

			/*
			 * At this point, the alarm deadline normally is somewhere near
			 * the current time but If the alarm had no deadline by now,
			 * initialize it with the current time.
			 */
			if (curr->_raw.deadline == 0) {
				curr->_raw.deadline        = _now;
				curr->_raw.deadline_period = _now_period;
			}
			/*
			 * Raise the deadline value by one period of the alarm and
			 * if the deadline value wraps thereby, update also in which
			 * period it is located.
			 */
			Alarm::Time const deadline = curr->_raw.deadline +
			                             triggered * curr->_raw.period;
			if (curr->_raw.deadline > deadline) {
				curr->_raw.deadline_period = !curr->_raw.deadline_period;
			}
			curr->_raw.deadline = deadline;

			/* synchronize enqueue operation */
			Mutex::Guard guard(_mutex);
			_unsynchronized_enqueue(curr);
		}

		/* release alarm, resume concurrent destructor operation */
		curr->_dispatch_mutex.release();
	}
}


void Alarm_scheduler::_setup_alarm(Alarm &alarm, Alarm::Time period, Alarm::Time deadline)
{
	/*
	 * If the alarm is already present in the queue, re-consider its queue
	 * position because its deadline might have changed. I.e., if an alarm is
	 * rescheduled with a new timeout before the original timeout triggered.
	 */
	if (alarm._active)
		_unsynchronized_dequeue(&alarm);

	alarm._assign(period, deadline, _now > deadline ? !_now_period : _now_period, this);

	_unsynchronized_enqueue(&alarm);
}


void Alarm_scheduler::schedule_absolute(Alarm *alarm, Alarm::Time timeout)
{
	Mutex::Guard alarm_list_guard(_mutex);

	_setup_alarm(*alarm, 0, timeout);
}


void Alarm_scheduler::schedule(Alarm *alarm, Alarm::Time period)
{
	Mutex::Guard alarm_list_guard(_mutex);

	/*
	 * Refuse to schedule a periodic timeout of 0 because it would trigger
	 * infinitely in the 'handle' function. To account for the case where the
	 * alarm object was already scheduled, we make sure to remove it from the
	 * queue.
	 */
	if (period == 0) {
		_unsynchronized_dequeue(alarm);
		return;
	}

	/* first deadline is overdue */
	_setup_alarm(*alarm, period, _now);
}


void Alarm_scheduler::discard(Alarm *alarm)
{
	/*
	 * Make sure that nobody is inside the '_get_pending_alarm' when
	 * grabbing the '_dispatch_mutex'. This is important when this function
	 * is called from the 'Alarm' destructor. Without the '_dispatch_mutex',
	 * we could take the mutex and proceed with destruction just before
	 * '_get_pending_alarm' tries to grab the mutex. When the destructor is
	 * finished, '_get_pending_alarm' would proceed with operating on a
	 * dangling pointer.
	 */
	Mutex::Guard alarm_list_guard(_mutex);

	if (alarm) {
		Mutex::Guard alarm_guard(alarm->_dispatch_mutex);
		_unsynchronized_dequeue(alarm);
	}
}


bool Alarm_scheduler::next_deadline(Alarm::Time *deadline)
{
	Mutex::Guard alarm_list_guard(_mutex);

	if (!_head) return false;

	if (deadline)
		*deadline = _head->_raw.deadline;

	if (deadline && *deadline < _min_handle_period.deadline) {
		*deadline = _min_handle_period.deadline;
	}
	return true;
}


Alarm_scheduler::~Alarm_scheduler()
{
	Mutex::Guard guard(_mutex);

	while (_head) {

		Alarm *next = _head->_next;

		/* reset alarm object */
		_head->_reset();

		/* remove from list */
		_head = next;
	}
}


Alarm::~Alarm()
{
	if (_scheduler)
		_scheduler->discard(this);
}

