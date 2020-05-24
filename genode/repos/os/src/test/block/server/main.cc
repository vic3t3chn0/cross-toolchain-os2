/*
 * \brief  Test block session interface - server side
 * \author Stefan Kalkowski
 * \date   2013-12-09
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/attached_rom_dataspace.h>
#include <block/component.h>
#include <block/driver.h>
#include <os/ring_buffer.h>
#include <timer_session/connection.h>


class Driver : public Block::Driver
{
	private:

		/*
		 * Noncopyable
		 */
		Driver(Driver const &);
		Driver &operator = (Driver const &);

		enum { MAX_REQUESTS = 5 };

		typedef Genode::Ring_buffer<Block::Packet_descriptor, MAX_REQUESTS,
		                            Genode::Ring_buffer_unsynchronized> Req_buffer;

		Genode::size_t                    _number;
		Genode::size_t                    _size;
		Req_buffer                        _packets { };
		Genode::Ram_dataspace_capability  _blk_ds;
		unsigned char                    *_blk_buf;

	public:

		Driver(Genode::Env &env, Genode::size_t number, Genode::size_t size)
		: Block::Driver(env.ram()),
		  _number(number), _size(size),
		  _blk_ds(env.ram().alloc(number*size)),
		  _blk_buf(env.rm().attach(_blk_ds)) {}

		void handler()
		{
			while (!_packets.empty()) {
				Block::Packet_descriptor p = _packets.get();
				ack_packet(p);
			}
		}


		/*******************************
		 **  Block::Driver interface  **
		 *******************************/

		void session_invalidated() override {
			while (!_packets.empty()) _packets.get(); }

		Block::Session::Info info() const override
		{
			return { .block_size  = _size,
			         .block_count = _number,
			         .align_log2  = Genode::log2(_size),
			         .writeable   = true };
		}

		void read(Block::sector_t           block_number,
		          Genode::size_t            block_count,
		          char                     *buffer,
		          Block::Packet_descriptor &packet) override
		{
			if (!_packets.avail_capacity())
				throw Block::Driver::Request_congestion();

			Genode::memcpy((void*)buffer, &_blk_buf[block_number*_size],
			               block_count * _size);
			_packets.add(packet);
		}

		void write(Block::sector_t           block_number,
		           Genode::size_t            block_count,
		           const char               *buffer,
		           Block::Packet_descriptor &packet) override
		{
			if (!_packets.avail_capacity())
				throw Block::Driver::Request_congestion();
			Genode::memcpy(&_blk_buf[block_number*_size],
			               (void*)buffer, block_count * _size);
			_packets.add(packet);
		}
};


struct Main
{
	Genode::Env &env;
	Genode::Heap heap { env.ram(), env.rm() };

	class Factory : public Block::Driver_factory
	{
		private:

			/*
			 * Noncopyable
			 */
			Factory(Factory const &);
			Factory &operator = (Factory const &);

		public:

			::Driver *driver = nullptr;

			Factory(Genode::Env &env, Genode::Heap &heap)
			{
				Genode::size_t blk_nr = 1024;
				Genode::size_t blk_sz = 512;

				try {
					Genode::Attached_rom_dataspace config { env, "config" };
					config.xml().attribute("sectors").value(blk_nr);
					config.xml().attribute("block_size").value(blk_sz);
				}
				catch (...) { }

				driver = new (&heap) Driver(env, blk_nr, blk_sz);
			}

			Block::Driver *create() override { return driver; }

			void destroy(Block::Driver *) override { }

	} factory { env, heap };

	Block::Root                    root { env.ep(), heap, env.rm(), factory, true };
	Timer::Connection              timer { env };
	Genode::Signal_handler<Driver> dispatcher { env.ep(), *factory.driver,
	                                            &Driver::handler };

	Main(Genode::Env &env) : env(env)
	{
		timer.sigh(dispatcher);
		timer.trigger_periodic(10000);
		env.parent().announce(env.ep().manage(root));
	}
};


void Component::construct(Genode::Env &env) { static Main server(env); }
