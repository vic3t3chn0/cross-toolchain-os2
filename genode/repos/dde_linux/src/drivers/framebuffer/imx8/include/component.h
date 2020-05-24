/*
 * \brief  i.MX8 framebuffer driver session component
 * \author Stefan Kalkowski
 * \author Christian Prochaska
 * \date   2015-10-16
 */

/*
 * Copyright (C) 2015-2019 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#ifndef __COMPONENT_H__
#define __COMPONENT_H__

/* Genode includes */
#include <base/component.h>
#include <base/rpc_server.h>
#include <root/component.h>
#include <dataspace/capability.h>
#include <framebuffer_session/framebuffer_session.h>
#include <timer_session/connection.h>
#include <util/reconstructible.h>
#include <base/attached_dataspace.h>
#include <base/attached_ram_dataspace.h>
#include <base/attached_rom_dataspace.h>
#include <os/reporter.h>
#include <os/pixel_rgb565.h>
#include <os/pixel_rgb888.h>
#include <os/dither_painter.h>

#include <lx_emul_c.h>

namespace Framebuffer {
	class Driver;
	class Session_component;
	class Root;
}


class Framebuffer::Driver
{
	private:

		struct Configuration
		{
			struct lx_c_fb_config _lx = { 16, 64, 64, 4,
			                              nullptr, 0, nullptr };
		} _config;

		Session_component             &_session;
		Timer::Connection              _timer;
		Genode::Reporter               _reporter;

		Genode::Signal_context_capability _config_sigh;

		drm_display_mode * _preferred_mode(drm_connector *connector,
		                                   unsigned &brightness);

	public:

		Driver(Genode::Env & env, Session_component &session)
		: _session(session), _timer(env),
		  _reporter(env, "connectors") {}

		int      width()   const { return _config._lx.width;  }
		int      height()  const { return _config._lx.height; }
		int      bpp()     const { return _config._lx.bpp;    }

		void *   fb_addr() const { return _config._lx.addr;   }
		unsigned pitch()   const { return _config._lx.pitch;  }

		void finish_initialization();

		void update_mode();

		void generate_report();

		/**
		 * Register signal handler used for config updates
		 *
		 * The signal handler is artificially triggered as a side effect
		 * of connector changes.
		 */
		void config_sigh(Genode::Signal_context_capability sigh)
		{
			_config_sigh = sigh;
		}

		void trigger_reconfiguration()
		{
			/*
			 * Trigger the reprocessing of the configuration following the
			 * same ontrol flow as used for external config changes.
			 */
			if (_config_sigh.valid())
				Genode::Signal_transmitter(_config_sigh).submit();
			else
				Genode::warning("config signal handler unexpectedly undefined");
		}
};


class Framebuffer::Session_component : public Genode::Rpc_object<Session>
{
	private:

		template <typename T> using Lazy = Genode::Constructible<T>;

		Driver                               _driver;
		Genode::Attached_rom_dataspace      &_config;
		Genode::Signal_context_capability    _mode_sigh;
		Timer::Connection                    _timer;
		Genode::Ram_allocator               &_ram;
		Genode::Attached_ram_dataspace       _ds;
		bool                                 _in_mode_change = true;

	public:

		Session_component(Genode::Env &env,
		                  Genode::Attached_rom_dataspace &config)
		: _driver(env, *this), _config(config), _timer(env),
		  _ram(env.ram()), _ds(env.ram(), env.rm(), 0) {}

		Driver & driver() { return _driver; }

		void config_changed()
		{
			_config.update();
			if (!_config.valid()) return;

			_in_mode_change = true;

			_driver.update_mode();

			if (_mode_sigh.valid())
				Genode::Signal_transmitter(_mode_sigh).submit();
		}

		Genode::Xml_node config() { return _config.xml(); }

		int force_width_from_config()
		{
			return _config.xml().attribute_value<unsigned>("force_width", 0);
		}

		int force_height_from_config()
		{
			return _config.xml().attribute_value<unsigned>("force_height", 0);
		}

		/***********************************
		 ** Framebuffer session interface **
		 ***********************************/

		Genode::Dataspace_capability dataspace() override
		{
			_ds.realloc(&_ram, _driver.width() * _driver.height() *
			                   Mode::bytes_per_pixel(Mode::RGB565));
			_in_mode_change = false;
			return _ds.cap();
		}

		Mode mode() const override {
			return Mode(_driver.width(), _driver.height(), Mode::RGB565); }

		void mode_sigh(Genode::Signal_context_capability sigh) override {
			_mode_sigh = sigh; }

		void sync_sigh(Genode::Signal_context_capability sigh) override
		{
			_timer.sigh(sigh);
			_timer.trigger_periodic(10*1000);
		}

		void refresh(int x, int y, int w, int h) override
		{
			using namespace Genode;

			if (!_driver.fb_addr()         ||
				!_ds.local_addr<void>() ||
				_in_mode_change) return;

			int width      = _driver.width();
			int height     = _driver.height();

			/* clip specified coordinates against screen boundaries */
			int x2 = min(x + w - 1, width  - 1),
			    y2 = min(y + h - 1, height - 1);
			int x1 = max(x, 0),
			    y1 = max(y, 0);
			if (x1 > x2 || y1 > y2) return;

			/* copy pixels from back buffer to physical frame buffer */
			Genode::Pixel_rgb565 * src = _ds.local_addr<Genode::Pixel_rgb565>();
			Genode::Pixel_rgb888 * dst = (Genode::Pixel_rgb888*)_driver.fb_addr();

			for (int row = y1; row <= y2; row++) {
				int line_offset = width * row;
				Genode::Pixel_rgb565 const * s = src + line_offset + x1;
				Genode::Pixel_rgb888 * d = dst + line_offset + x1;
				for (int col = x1; col <= x2; col++) {
					Genode::Pixel_rgb565 const px = *s++;
					*d++ = Genode::Pixel_rgb888(px.r(), px.g(), px.b(), px.a());
				}
			}
		}
};


struct Framebuffer::Root
: Genode::Root_component<Framebuffer::Session_component, Genode::Single_client>
{
	Session_component session; /* single session */

	Session_component *_create_session(const char *) override {
		return &session; }

	void _destroy_session(Session_component *) override { }

	Root(Genode::Env &env, Genode::Allocator &alloc,
	     Genode::Attached_rom_dataspace &config)
	: Genode::Root_component<Session_component,
	                         Genode::Single_client>(env.ep(), alloc),
	  session(env, config) { }
};

#endif /* __COMPONENT_H__ */
