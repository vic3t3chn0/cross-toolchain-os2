/*
 * \brief  List of assignments
 * \author Norman Feske
 * \date   2018-09-27
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _ASSIGN_LIST_H_
#define _ASSIGN_LIST_H_

/* local includes */
#include <types.h>
#include <assign.h>

namespace Window_layouter { class Assign_list; }


class Window_layouter::Assign_list : Noncopyable
{
	private:

		Allocator &_alloc;

		List_model<Assign> _assignments { };

		struct Update_policy : List_model<Assign>::Update_policy
		{
			Allocator &_alloc;

			Update_policy(Allocator &alloc) : _alloc(alloc) { }

			void destroy_element(Assign &elem) { destroy(_alloc, &elem); }

			Assign &create_element(Xml_node node)
			{
				return *new (_alloc) Assign(node);
			}

			void update_element(Assign &assign, Xml_node node)
			{
				assign.update(node);
			}

			static bool element_matches_xml_node(Assign const &elem, Xml_node node)
			{
				return elem.matches(node);
			}

			bool node_is_element(Xml_node node)
			{
				return node.has_type("assign");
			}
		};

	public:

		Assign_list(Allocator &alloc) : _alloc(alloc) { }

		void update_from_xml(Xml_node node)
		{
			Update_policy policy(_alloc);
			_assignments.update_from_xml(policy, node);
		}

		void assign_windows(Window_list &windows)
		{
			_assignments.for_each([&] (Assign &assign) {

				windows.for_each_window([&] (Window &window) {

					auto fn = [&] (Registry<Assign::Member> &registry) {
						window.assignment(registry); };

					assign.with_matching_members_registry(window.label(), fn);
				});
			});
		}

		template <typename FN>
		void for_each_wildcard_member(FN const &fn) const
		{
			_assignments.for_each([&] (Assign const &assign) {
				assign.for_each_wildcard_member([&] (Assign::Member const &member) {
					fn(assign, member); }); });
		}

		template <typename FN>
		void for_each_wildcard_assigned_window(FN const &fn)
		{
			_assignments.for_each([&] (Assign &assign) {
				assign.for_each_wildcard_member([&] (Assign::Member &member) {
					fn(member.window); }); });
		}

		/**
		 * Return true if any window is assigned via a wildcard
		 *
		 * In this case, a new 'rules' report should be generated that turns
		 * the wildcard matches into exact assignments.
		 */
		bool matching_wildcards() const
		{
			bool result = false;
			for_each_wildcard_member([&] (Assign const &, Assign::Member const &) {
				result = true; });
			return result;
		}

		template <typename FN>
		void for_each(FN const &fn) { _assignments.for_each(fn); }

		template <typename FN>
		void for_each(FN const &fn) const { _assignments.for_each(fn); }
};

#endif /* _ASSIGN_LIST_H_ */
