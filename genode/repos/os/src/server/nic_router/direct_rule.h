/*
 * \brief  Routing rule for direct traffic between two interfaces
 * \author Martin Stein
 * \date   2016-08-19
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _DIRECT_RULE_H_
#define _DIRECT_RULE_H_

/* local includes */
#include <ipv4_address_prefix.h>
#include <list.h>

/* Genode includes */
#include <util/list.h>
#include <util/xml_node.h>

namespace Genode { class Xml_node; }

namespace Net {

	class                     Direct_rule_base;
	template <typename> class Direct_rule;
	template <typename> class Direct_rule_list;
}


class Net::Direct_rule_base
{
	protected:

		Ipv4_address_prefix const _dst;

	public:

		struct Invalid : Genode::Exception { };

		Direct_rule_base(Genode::Xml_node const node);


		/*********
		 ** log **
		 *********/

		void print(Genode::Output &output) const;


		/***************
		 ** Accessors **
		 ***************/

		Ipv4_address_prefix const &dst() const { return _dst; }
};


template <typename T>
struct Net::Direct_rule : Direct_rule_base,
                          Direct_rule_list<T>::Element
{
	Direct_rule(Genode::Xml_node const node) : Direct_rule_base(node) { }
};


template <typename T>
struct Net::Direct_rule_list : List<T>
{
	using Base = List<T>;

	struct No_match : Genode::Exception { };

	T const &longest_prefix_match(Ipv4_address const &ip) const
	{
		/* first match is sufficient as the list is prefix-size-sorted */
		for (T const *curr = Base::first(); curr; curr = curr->next()) {
			if (curr->dst().prefix_matches(ip)) {
				return *curr; }
		}
		throw No_match();
	}

	void insert(T &rule)
	{
		/* ensure that the list stays prefix-size-sorted (descending) */
		T *behind = nullptr;
		for (T *curr = Base::first(); curr; curr = curr->next()) {
			if (rule.dst().prefix >= curr->dst().prefix) {
				break; }

			behind = curr;
		}
		Base::insert(&rule, behind);
	}
};

#endif /* _RULE_H_ */
