/*
 * \brief  Text selection
 * \author Norman Feske
 * \date   2020-01-15
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _TEXT_SELECTION_H_
#define _TEXT_SELECTION_H_

/* Genode includes */
#include <util/xml_generator.h>
#include <util/list_model.h>

/* local includes */
#include <types.h>
#include <widget_factory.h>
#include <cursor.h>

namespace Menu_view { struct Text_selection; }

class Menu_view::Text_selection : List_model<Text_selection>::Element
{
	public:

		using Glyph_position = Cursor::Glyph_position;

	private:

		friend class List_model<Text_selection>;
		friend class List<Text_selection>;

		Glyph_position &_glyph_position;

		enum { NAME_MAX_LEN = 32 };
		typedef String<NAME_MAX_LEN> Name;

		Name const _name;

		struct Range
		{
			unsigned xpos_px;
			unsigned width_px;
		};

		Range _range { 0, 0 };

		static Name _node_name(Xml_node node)
		{
			return node.attribute_value("name", Name(node.type()));
		}

		Range _range_from_xml_node(Xml_node node)
		{
			unsigned const at     = node.attribute_value("at",     0u),
			               length = node.attribute_value("length", 0u);

			unsigned const x1 = _glyph_position.xpos_of_glyph(at),
			               x2 = _glyph_position.xpos_of_glyph(at + length);

			unsigned const width = (x2 >= x1 ? x2 - x1 : 0);

			return { .xpos_px = x1, .width_px = width };
		}

	public:

		Text_selection(Xml_node node, Glyph_position &glyph_position)
		:
			_glyph_position(glyph_position), _name(_node_name(node))
		{ }

		void draw(Surface<Pixel_rgb888> &pixel_surface,
		          Surface<Pixel_alpha8> &,
		          Point at, unsigned height) const
		{
			Color const color(0xcf, 0x69, 0x69, 180);
			Box_painter::paint(pixel_surface,
			                   Rect(at + Point(_range.xpos_px, 0),
			                        Area(_range.width_px, height)),
			                   color);
		}

		struct Model_update_policy : List_model<Text_selection>::Update_policy
		{
			Allocator      &_alloc;
			Glyph_position &_glyph_position;

			Model_update_policy(Allocator &alloc, Glyph_position &glyph_position)
			:
				_alloc(alloc), _glyph_position(glyph_position)
			{ }

			void destroy_element(Text_selection &t) { destroy(_alloc, &t); }

			Text_selection &create_element(Xml_node node)
			{
				return *new (_alloc)
					Text_selection(node, _glyph_position);
			}

			void update_element(Text_selection &t, Xml_node node)
			{
				t._range = t._range_from_xml_node(node);
			}

			static bool element_matches_xml_node(Text_selection const &t, Xml_node node)
			{
				return node.has_type("selection") && _node_name(node) == t._name;
			}

			static bool node_is_element(Xml_node node)
			{
				return node.has_type("selection");
			}
		};
};

#endif /* _TEXT_SELECTION_ */
