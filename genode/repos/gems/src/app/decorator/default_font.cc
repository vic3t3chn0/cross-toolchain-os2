/*
 * \brief  Accessor for the default font
 * \author Norman Feske
 * \date   2015-09-16
 */

/*
 * Copyright (C) 2015-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <nitpicker_gfx/tff_font.h>

/* local includes */
#include "canvas.h"


/**
 * Statically linked binary data
 */
extern char _binary_droidsansb10_tff_start[];


/**
 * Return default font
 */
Decorator::Font &Decorator::default_font()
{
	static Tff_font::Static_glyph_buffer<4096> glyph_buffer { };

	static Tff_font font { _binary_droidsansb10_tff_start, glyph_buffer };

	return font;
}

