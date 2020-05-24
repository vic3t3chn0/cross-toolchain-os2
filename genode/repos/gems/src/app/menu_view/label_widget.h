/*
 * \brief  Widget that displays a single line of plain text
 * \author Norman Feske
 * \date   2009-09-11
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _LABEL_WIDGET_H_
#define _LABEL_WIDGET_H_

/* local includes */
#include <widget_factory.h>
#include <text_selection.h>
#include <animated_color.h>

namespace Menu_view { struct Label_widget; }

struct Menu_view::Label_widget : Widget, Cursor::Glyph_position
{
	Text_painter::Font const *_font = nullptr;

	enum { LABEL_MAX_LEN = 256 };

	typedef String<200> Text;
	Text _text { };

	Animated_color _color;

	int _min_width  = 0;
	int _min_height = 0;

	Cursor::Model_update_policy         _cursor_update_policy;
	Text_selection::Model_update_policy _selection_update_policy;

	List_model<Cursor>         _cursors    { };
	List_model<Text_selection> _selections { };

	Label_widget(Widget_factory &factory, Xml_node node, Unique_id unique_id)
	:
		Widget(factory, node, unique_id),
		_color(factory.animator),
		_cursor_update_policy(factory, *this),
		_selection_update_policy(factory.alloc, *this)
	{ }

	~Label_widget()
	{
		_cursors   .destroy_all_elements(_cursor_update_policy);
		_selections.destroy_all_elements(_selection_update_policy);
	}

	void update(Xml_node node) override
	{
		_font       = _factory.styles.font(node);
		_text       = Text("");
		_min_width  = 0;
		_min_height = 0;

		_factory.styles.with_label_style(node, [&] (Label_style style) {
			_color.fade_to(style.color, Animated_color::Steps{80}); });

		if (node.has_attribute("text")) {
			_text       = node.attribute_value("text", _text);
			_text       = Xml_unquoted(_text);
			_min_height = _font->height();
		}

		unsigned const min_ex = node.attribute_value("min_ex", 0U);
		if (min_ex) {
			Glyph_painter::Fixpoint_number min_w_px = _font->string_width("x");
			min_w_px.value *= min_ex;
			_min_width = min_w_px.decimal();
		}

		_cursors   .update_from_xml(_cursor_update_policy,    node);
		_selections.update_from_xml(_selection_update_policy, node);
	}

	Area min_size() const override
	{
		if (!_font)
			return Area(0, 0);

		return Area(max(_font->string_width(_text.string()).decimal(), _min_width),
		            _min_height);
	}

	void draw(Surface<Pixel_rgb888> &pixel_surface,
	          Surface<Pixel_alpha8> &alpha_surface,
	          Point at) const override
	{
		if (!_font) return;

		Area text_size = min_size();

		int const dx = (int)geometry().w() - text_size.w(),
		          dy = (int)geometry().h() - text_size.h();

		Point const centered = at + Point(dx/2, dy/2);

		_selections.for_each([&] (Text_selection const &selection) {
			selection.draw(pixel_surface, alpha_surface, at, text_size.h()); });

		Color const color = _color.color();
		int   const alpha = color.a;

		if (alpha) {
			Text_painter::paint(pixel_surface,
			                    Text_painter::Position(centered.x(), centered.y()),
			                    *_font, color, _text.string());

			Text_painter::paint(alpha_surface,
			                    Text_painter::Position(centered.x(), centered.y()),
			                    *_font, Color(alpha, alpha, alpha, alpha), _text.string());
		}

		_cursors.for_each([&] (Cursor const &cursor) {
			cursor.draw(pixel_surface, alpha_surface, at, text_size.h()); });
	}

	/**
	 * Cursor::Glyph_position interface
	 */
	int xpos_of_glyph(unsigned at) const override
	{
		return _font->string_width(_text.string(), at).decimal();
	}

	unsigned _char_index_at_xpos(unsigned xpos) const
	{
		return _font->index_at_xpos(_text.string(), xpos);
	}

	Hovered hovered(Point at) const override
	{
		Unique_id const hovered_id = Widget::hovered(at).unique_id;

		if (!hovered_id.valid())
			return Hovered { .unique_id = hovered_id, .detail = { } };

		return { .unique_id = hovered_id,
		         .detail    = { _char_index_at_xpos(at.x()) } };
	}

	void gen_hover_model(Xml_generator &xml, Point at) const override
	{
		if (_inner_geometry().contains(at)) {

			xml.node(_type_name.string(), [&]() {

				_gen_common_hover_attr(xml);

				xml.attribute("at", _char_index_at_xpos(at.x()));
			});
		}
	}

	private:

		/**
		 * Noncopyable
		 */
		Label_widget(Label_widget const &);
		Label_widget &operator = (Label_widget const &);
};

#endif /* _LABEL_WIDGET_H_ */
