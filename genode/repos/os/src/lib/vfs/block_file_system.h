/*
 * \brief  Block device file system
 * \author Josef Soentgen
 * \author Norman Feske
 * \date   2013-12-20
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__VFS__BLOCK_FILE_SYSTEM_H_
#define _INCLUDE__VFS__BLOCK_FILE_SYSTEM_H_

#include <base/allocator_avl.h>
#include <block_session/connection.h>
#include <vfs/single_file_system.h>

namespace Vfs { class Block_file_system; }


class Vfs::Block_file_system : public Single_file_system
{
	private:

		Vfs::Env &_env;

		typedef Genode::String<64> Label;
		Label _label;

		/*
		 * Serialize access to packet stream of the block session
		 */
		Mutex _mutex { };

		char                 *_block_buffer;
		unsigned              _block_buffer_count;

		Genode::Allocator_avl _tx_block_alloc { &_env.alloc() };

		Block::Connection<> _block {
			_env.env(), &_tx_block_alloc, 128*1024, _label.string() };

		Block::Session::Info const _info { _block.info() };

		Block::Session::Tx::Source *_tx_source;

		bool _writeable;

		Genode::Signal_receiver           _signal_receiver { };
		Genode::Signal_context            _signal_context  { };
		Genode::Signal_context_capability _source_submit_cap;

		/*
		 * Noncopyable
		 */
		Block_file_system(Block_file_system const &);
		Block_file_system &operator = (Block_file_system const &);

		class Block_vfs_handle : public Single_vfs_handle
		{
			private:

				Genode::Allocator                 &_alloc;
				Label                             &_label;
				Mutex                             &_mutex;
				char                              *_block_buffer;
				unsigned                          &_block_buffer_count;
				Genode::Allocator_avl             &_tx_block_alloc;
				Block::Connection<>               &_block;
				Genode::size_t               const _block_size;
				Block::sector_t              const _block_count;
				Block::Session::Tx::Source        *_tx_source;
				bool                         const _writeable;
				Genode::Signal_receiver           &_signal_receiver;
				Genode::Signal_context            &_signal_context;
				Genode::Signal_context_capability &_source_submit_cap;

				/*
				 * Noncopyable
				 */
				Block_vfs_handle(Block_vfs_handle const &);
				Block_vfs_handle &operator = (Block_vfs_handle const &);

				file_size _block_io(file_size nr, void *buf, file_size sz,
				                    bool write, bool bulk = false)
				{
					Block::Packet_descriptor::Opcode op;
					op = write ? Block::Packet_descriptor::WRITE : Block::Packet_descriptor::READ;

					file_size packet_size  = bulk ? sz : _block_size;
					file_size packet_count = bulk ? (sz / _block_size) : 1;

					Block::Packet_descriptor packet;

					/* sanity check */
					if (packet_count > _block_buffer_count) {
						packet_size  = _block_buffer_count * _block_size;
						packet_count = _block_buffer_count;
					}

					while (true) {
						try {
							Mutex::Guard guard(_mutex);

							packet = _block.alloc_packet(packet_size);
							break;
						} catch (Block::Session::Tx::Source::Packet_alloc_failed) {
							if (!_tx_source->ready_to_submit())
								_signal_receiver.wait_for_signal();
							else {
								if (packet_count > 1) {
									packet_size  /= 2;
									packet_count /= 2;
								}
							}
						}
					}
					Mutex::Guard guard(_mutex);

					Block::Packet_descriptor p(packet, op, nr, packet_count);

					if (write)
						Genode::memcpy(_tx_source->packet_content(p), buf, packet_size);

					_tx_source->submit_packet(p);
					p = _tx_source->get_acked_packet();

					if (!p.succeeded()) {
						Genode::error("Could not read block(s)");
						_tx_source->release_packet(p);
						return 0;
					}

					if (!write)
						Genode::memcpy(buf, _tx_source->packet_content(p), packet_size);

					_tx_source->release_packet(p);
					return packet_size;
				}

			public:

				Block_vfs_handle(Directory_service                 &ds,
				                 File_io_service                   &fs,
				                 Genode::Allocator                 &alloc,
				                 Label                             &label,
				                 Mutex                             &mutex,
				                 char                              *block_buffer,
				                 unsigned                          &block_buffer_count,
				                 Genode::Allocator_avl             &tx_block_alloc,
				                 Block::Connection<>               &block,
				                 Genode::size_t                     block_size,
				                 Block::sector_t                    block_count,
				                 Block::Session::Tx::Source        *tx_source,
				                 bool                               writeable,
				                 Genode::Signal_receiver           &signal_receiver,
				                 Genode::Signal_context            &signal_context,
				                 Genode::Signal_context_capability &source_submit_cap)
				: Single_vfs_handle(ds, fs, alloc, 0),
				  _alloc(alloc),
				  _label(label),
				  _mutex(mutex),
				  _block_buffer(block_buffer),
				  _block_buffer_count(block_buffer_count),
				  _tx_block_alloc(tx_block_alloc),
				  _block(block),
				  _block_size(block_size),
				  _block_count(block_count),
				  _tx_source(tx_source),
				  _writeable(writeable),
				  _signal_receiver(signal_receiver),
				  _signal_context(signal_context),
				  _source_submit_cap(source_submit_cap)
				{ }

				Read_result read(char *dst, file_size count,
			                     file_size &out_count) override
				{
					file_size seek_offset = seek();

					file_size read = 0;
					while (count > 0) {
						file_size displ   = 0;
						file_size length  = 0;
						file_size nbytes  = 0;
						file_size blk_nr  = seek_offset / _block_size;

						displ = seek_offset % _block_size;

						if ((displ + count) > _block_size)
							length = (_block_size - displ);
						else
							length = count;

						/*
						 * We take a shortcut and read the blocks all at once if the
						 * offset is aligned on a block boundary and the count is a
						 * multiple of the block size, e.g. 4K reads will be read at
						 * once.
						 *
						 * XXX this is quite hackish because we have to omit partial
						 * blocks at the end.
						 */
						if (displ == 0 && !(count < _block_size)) {
							file_size bytes_left = count - (count % _block_size);

							nbytes = _block_io(blk_nr, dst + read, bytes_left, false, true);
							if (nbytes == 0) {
								Genode::error("error while reading block:", blk_nr, " from block device");
								return READ_ERR_INVALID;
							}

							read  += nbytes;
							count -= nbytes;
							seek_offset += nbytes;

							continue;
						}

						nbytes = _block_io(blk_nr, _block_buffer, _block_size, false);
						if ((unsigned)nbytes != _block_size) {
							Genode::error("error while reading block:", blk_nr, " from block device");
							return READ_ERR_INVALID;
						}

						Genode::memcpy(dst + read, _block_buffer + displ, length);

						read  += length;
						count -= length;
						seek_offset += length;
					}

					out_count = read;

					return READ_OK;

				}

				Write_result write(char const *buf, file_size count,
				                   file_size &out_count) override
				{
					if (!_writeable) {
						Genode::error("block device is not writeable");
						return WRITE_ERR_INVALID;
					}

					file_size seek_offset = seek();

					file_size written = 0;
					while (count > 0) {
						file_size displ   = 0;
						file_size length  = 0;
						file_size nbytes  = 0;
						file_size blk_nr  = seek_offset / _block_size;

						displ = seek_offset % _block_size;

						if ((displ + count) > _block_size)
							length = (_block_size - displ);
						else
							length = count;

						/*
						 * We take a shortcut and write as much as possible without
						 * using the block buffer if the offset is aligned on a block
						 * boundary and the count is a multiple of the block size,
						 * e.g. 4K writes will be written at once.
						 *
						 * XXX this is quite hackish because we have to omit partial
						 * blocks at the end.
						 */
						if (displ == 0 && !(count < _block_size)) {
							file_size bytes_left = count - (count % _block_size);

							nbytes = _block_io(blk_nr, (void*)(buf + written),
							                   bytes_left, true, true);
							if (nbytes == 0) {
								Genode::error("error while write block:", blk_nr, " to block device");
								return WRITE_ERR_INVALID;
							}

							written     += nbytes;
							count       -= nbytes;
							seek_offset += nbytes;

							continue;
						}

						/*
						 * The offset is not aligned on a block boundary. Therefore
						 * we need to read the block to the block buffer first and
						 * put the buffer content at the right offset before we can
						 * write the whole block back. In addition if length is less
						 * than block size, we also have to read the block first.
						 */
						if (displ > 0 || length < _block_size)
							_block_io(blk_nr, _block_buffer, _block_size, false);

						Genode::memcpy(_block_buffer + displ, buf + written, length);

						nbytes = _block_io(blk_nr, _block_buffer, _block_size, true);
						if ((unsigned)nbytes != _block_size) {
							Genode::error("error while writing block:", blk_nr, " to block_device");
							return WRITE_ERR_INVALID;
						}

						written     += length;
						count       -= length;
						seek_offset += length;
					}

					out_count = written;

					return WRITE_OK;

				}

				Sync_result sync() override
				{
					/*
					 * Just in case bail if we cannot submit the packet.
					 * (Since the plugin operates in a synchronous fashion
					 * that should not happen.)
					 */
					if (!_tx_source->ready_to_submit()) {
						Genode::error("vfs_block: could not sync blocks");
						return SYNC_ERR_INVALID;
					}

					Block::Packet_descriptor p =
						Block::Session::sync_all_packet_descriptor(
							_block.info(), Block::Session::Tag { 0 });

					_tx_source->submit_packet(p);
					p = _tx_source->get_acked_packet();
					_tx_source->release_packet(p);

					if (!p.succeeded()) {
						Genode::error("vfs_block: syncing blocks failed");
						return SYNC_ERR_INVALID;
					}

					return SYNC_OK;
				}

				bool read_ready() override { return true; }
		};

	public:

		Block_file_system(Vfs::Env &env, Genode::Xml_node config)
		:
			Single_file_system(Node_type::CONTINUOUS_FILE, name(),
			                   Node_rwx::rw(), config),
			_env(env),
			_label(config.attribute_value("label", Label())),
			_block_buffer(0),
			_block_buffer_count(config.attribute_value("block_buffer_count", 1UL)),
			_tx_source(_block.tx()),
			_writeable(_info.writeable),
			_source_submit_cap(_signal_receiver.manage(&_signal_context))
		{
			_block_buffer = new (_env.alloc())
				char[_block_buffer_count * _info.block_size];

			_block.tx_channel()->sigh_ready_to_submit(_source_submit_cap);
		}

		~Block_file_system()
		{
			_signal_receiver.dissolve(&_signal_context);

			destroy(_env.alloc(), _block_buffer);
		}

		static char const *name()   { return "block"; }
		char const *type() override { return "block"; }

		/*********************************
		 ** Directory service interface **
		 *********************************/

		Open_result open(char const  *path, unsigned,
		                 Vfs_handle **out_handle,
		                 Allocator   &alloc) override
		{
			if (!_single_file(path))
				return OPEN_ERR_UNACCESSIBLE;

			try {
				*out_handle = new (alloc) Block_vfs_handle(*this, *this, alloc,
				                                           _label, _mutex,
				                                           _block_buffer,
				                                           _block_buffer_count,
				                                           _tx_block_alloc,
				                                           _block,
				                                           _info.block_size,
				                                           _info.block_count,
				                                           _tx_source,
				                                           _info.writeable,
				                                           _signal_receiver,
				                                           _signal_context,
				                                           _source_submit_cap);
				return OPEN_OK;
			}
			catch (Genode::Out_of_ram)  { return OPEN_ERR_OUT_OF_RAM; }
			catch (Genode::Out_of_caps) { return OPEN_ERR_OUT_OF_CAPS; }
		}

		Stat_result stat(char const *path, Stat &out) override
		{
			Stat_result const result = Single_file_system::stat(path, out);
			out.size = _info.block_count * _info.block_size;
			return result;
		}


		/********************************
		 ** File I/O service interface **
		 ********************************/

		Ftruncate_result ftruncate(Vfs_handle *, file_size) override
		{
			return FTRUNCATE_OK;
		}

		Ioctl_result ioctl(Vfs_handle *, Ioctl_opcode opcode, Ioctl_arg,
		                   Ioctl_out &out) override
		{
			switch (opcode) {
			case IOCTL_OP_DIOCGMEDIASIZE:

				out.diocgmediasize.size = _info.block_count * _info.block_size;
				return IOCTL_OK;

			default:

				Genode::warning("invalid ioctl request ", (int)opcode);
				break;
			}

			/* never reached */
			return IOCTL_ERR_INVALID;
		}
};

#endif /* _INCLUDE__VFS__BLOCK_FILE_SYSTEM_H_ */
