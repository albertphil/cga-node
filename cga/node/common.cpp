
#include <cga/node/common.hpp>

#include <cga/lib/work.hpp>
#include <cga/node/wallet.hpp>

#include <boost/endian/conversion.hpp>

std::array<uint8_t, 2> constexpr cga::message_header::magic_number;
std::bitset<16> constexpr cga::message_header::block_type_mask;

cga::message_header::message_header (cga::message_type type_a) :
version_max (cga::protocol_version),
version_using (cga::protocol_version),
version_min (cga::protocol_version_min),
type (type_a)
{
}

cga::message_header::message_header (bool & error_a, cga::stream & stream_a)
{
	if (!error_a)
	{
		error_a = deserialize (stream_a);
	}
}

void cga::message_header::serialize (cga::stream & stream_a) const
{
	cga::write (stream_a, cga::message_header::magic_number);
	cga::write (stream_a, version_max);
	cga::write (stream_a, version_using);
	cga::write (stream_a, version_min);
	cga::write (stream_a, type);
	cga::write (stream_a, static_cast<uint16_t> (extensions.to_ullong ()));
}

bool cga::message_header::deserialize (cga::stream & stream_a)
{
	uint16_t extensions_l;
	std::array<uint8_t, 2> magic_number_l;
	auto error (false);
	try
	{
		read (stream_a, magic_number_l);
		if (magic_number_l != magic_number)
		{
			throw std::runtime_error ("Magic numbers do not match");
		}

		cga::read (stream_a, version_max);
		cga::read (stream_a, version_using);
		cga::read (stream_a, version_min);
		cga::read (stream_a, type);
		cga::read (stream_a, extensions_l);
		extensions = extensions_l;
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

cga::message::message (cga::message_type type_a) :
header (type_a)
{
}

cga::message::message (cga::message_header const & header_a) :
header (header_a)
{
}

cga::block_type cga::message_header::block_type () const
{
	return static_cast<cga::block_type> (((extensions & block_type_mask) >> 8).to_ullong ());
}

void cga::message_header::block_type_set (cga::block_type type_a)
{
	extensions &= ~block_type_mask;
	extensions |= std::bitset<16> (static_cast<unsigned long long> (type_a) << 8);
}

bool cga::message_header::bulk_pull_is_count_present () const
{
	auto result (false);
	if (type == cga::message_type::bulk_pull)
	{
		if (extensions.test (bulk_pull_count_present_flag))
		{
			result = true;
		}
	}

	return result;
}

size_t cga::message_header::payload_length_bytes () const
{
	switch (type)
	{
		case cga::message_type::bulk_pull:
		{
			return cga::bulk_pull::size + (bulk_pull_is_count_present () ? cga::bulk_pull::extended_parameters_size : 0);
		}
		case cga::message_type::bulk_push:
		{
			// bulk_push doesn't have a payload
			return 0;
		}
		case cga::message_type::frontier_req:
		{
			return cga::frontier_req::size;
		}
		case cga::message_type::bulk_pull_account:
		{
			return cga::bulk_pull_account::size;
		}
		case cga::message_type::keepalive:
		{
			return cga::keepalive::size;
		}
		// Add realtime network messages once they get framing support; currently the
		// realtime messages all fit in a datagram from which they're deserialized.
		default:
		{
			assert (false);
			return 0;
		}
	}
}

// MTU - IP header - UDP header
const size_t cga::message_parser::max_safe_udp_message_size = 508;

std::string cga::message_parser::status_string ()
{
	switch (status)
	{
		case cga::message_parser::parse_status::success:
		{
			return "success";
		}
		case cga::message_parser::parse_status::insufficient_work:
		{
			return "insufficient_work";
		}
		case cga::message_parser::parse_status::invalid_header:
		{
			return "invalid_header";
		}
		case cga::message_parser::parse_status::invalid_message_type:
		{
			return "invalid_message_type";
		}
		case cga::message_parser::parse_status::invalid_keepalive_message:
		{
			return "invalid_keepalive_message";
		}
		case cga::message_parser::parse_status::invalid_publish_message:
		{
			return "invalid_publish_message";
		}
		case cga::message_parser::parse_status::invalid_confirm_req_message:
		{
			return "invalid_confirm_req_message";
		}
		case cga::message_parser::parse_status::invalid_confirm_ack_message:
		{
			return "invalid_confirm_ack_message";
		}
		case cga::message_parser::parse_status::invalid_node_id_handshake_message:
		{
			return "invalid_node_id_handshake_message";
		}
		case cga::message_parser::parse_status::outdated_version:
		{
			return "outdated_version";
		}
		case cga::message_parser::parse_status::invalid_magic:
		{
			return "invalid_magic";
		}
		case cga::message_parser::parse_status::invalid_network:
		{
			return "invalid_network";
		}
	}

	assert (false);

	return "[unknown parse_status]";
}

cga::message_parser::message_parser (cga::block_uniquer & block_uniquer_a, cga::vote_uniquer & vote_uniquer_a, cga::message_visitor & visitor_a, cga::work_pool & pool_a) :
block_uniquer (block_uniquer_a),
vote_uniquer (vote_uniquer_a),
visitor (visitor_a),
pool (pool_a),
status (parse_status::success)
{
}

void cga::message_parser::deserialize_buffer (uint8_t const * buffer_a, size_t size_a)
{
	status = parse_status::success;
	auto error (false);
	if (size_a <= max_safe_udp_message_size)
	{
		// Guaranteed to be deliverable
		cga::bufferstream stream (buffer_a, size_a);
		cga::message_header header (error, stream);
		if (!error)
		{
			if (cga::is_beta_network && header.version_using < cga::protocol_version_reasonable_min)
			{
				status = parse_status::outdated_version;
			}
			else if (header.version_using < cga::protocol_version_min)
			{
				status = parse_status::outdated_version;
			}
			else if (!header.valid_magic ())
			{
				status = parse_status::invalid_magic;
			}
			else if (!header.valid_network ())
			{
				status = parse_status::invalid_network;
			}
			else
			{
				switch (header.type)
				{
					case cga::message_type::keepalive:
					{
						deserialize_keepalive (stream, header);
						break;
					}
					case cga::message_type::publish:
					{
						deserialize_publish (stream, header);
						break;
					}
					case cga::message_type::confirm_req:
					{
						deserialize_confirm_req (stream, header);
						break;
					}
					case cga::message_type::confirm_ack:
					{
						deserialize_confirm_ack (stream, header);
						break;
					}
					case cga::message_type::node_id_handshake:
					{
						deserialize_node_id_handshake (stream, header);
						break;
					}
					default:
					{
						status = parse_status::invalid_message_type;
						break;
					}
				}
			}
		}
		else
		{
			status = parse_status::invalid_header;
		}
	}
}

void cga::message_parser::deserialize_keepalive (cga::stream & stream_a, cga::message_header const & header_a)
{
	auto error (false);
	cga::keepalive incoming (error, stream_a, header_a);
	if (!error && at_end (stream_a))
	{
		visitor.keepalive (incoming);
	}
	else
	{
		status = parse_status::invalid_keepalive_message;
	}
}

void cga::message_parser::deserialize_publish (cga::stream & stream_a, cga::message_header const & header_a)
{
	auto error (false);
	cga::publish incoming (error, stream_a, header_a, &block_uniquer);
	if (!error && at_end (stream_a))
	{
		if (!cga::work_validate (*incoming.block))
		{
			visitor.publish (incoming);
		}
		else
		{
			status = parse_status::insufficient_work;
		}
	}
	else
	{
		status = parse_status::invalid_publish_message;
	}
}

void cga::message_parser::deserialize_confirm_req (cga::stream & stream_a, cga::message_header const & header_a)
{
	auto error (false);
	cga::confirm_req incoming (error, stream_a, header_a, &block_uniquer);
	if (!error && at_end (stream_a))
	{
		if (incoming.block == nullptr || !cga::work_validate (*incoming.block))
		{
			visitor.confirm_req (incoming);
		}
		else
		{
			status = parse_status::insufficient_work;
		}
	}
	else
	{
		status = parse_status::invalid_confirm_req_message;
	}
}

void cga::message_parser::deserialize_confirm_ack (cga::stream & stream_a, cga::message_header const & header_a)
{
	auto error (false);
	cga::confirm_ack incoming (error, stream_a, header_a, &vote_uniquer);
	if (!error && at_end (stream_a))
	{
		for (auto & vote_block : incoming.vote->blocks)
		{
			if (!vote_block.which ())
			{
				auto block (boost::get<std::shared_ptr<cga::block>> (vote_block));
				if (cga::work_validate (*block))
				{
					status = parse_status::insufficient_work;
					break;
				}
			}
		}
		if (status == parse_status::success)
		{
			visitor.confirm_ack (incoming);
		}
	}
	else
	{
		status = parse_status::invalid_confirm_ack_message;
	}
}

void cga::message_parser::deserialize_node_id_handshake (cga::stream & stream_a, cga::message_header const & header_a)
{
	bool error_l (false);
	cga::node_id_handshake incoming (error_l, stream_a, header_a);
	if (!error_l && at_end (stream_a))
	{
		visitor.node_id_handshake (incoming);
	}
	else
	{
		status = parse_status::invalid_node_id_handshake_message;
	}
}

bool cga::message_parser::at_end (cga::stream & stream_a)
{
	uint8_t junk;
	auto end (cga::try_read (stream_a, junk));
	return end;
}

cga::keepalive::keepalive () :
message (cga::message_type::keepalive)
{
	cga::endpoint endpoint (boost::asio::ip::address_v6{}, 0);
	for (auto i (peers.begin ()), n (peers.end ()); i != n; ++i)
	{
		*i = endpoint;
	}
}

cga::keepalive::keepalive (bool & error_a, cga::stream & stream_a, cga::message_header const & header_a) :
message (header_a)
{
	if (!error_a)
	{
		error_a = deserialize (stream_a);
	}
}

void cga::keepalive::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.keepalive (*this);
}

void cga::keepalive::serialize (cga::stream & stream_a) const
{
	header.serialize (stream_a);
	for (auto i (peers.begin ()), j (peers.end ()); i != j; ++i)
	{
		assert (i->address ().is_v6 ());
		auto bytes (i->address ().to_v6 ().to_bytes ());
		write (stream_a, bytes);
		write (stream_a, i->port ());
	}
}

bool cga::keepalive::deserialize (cga::stream & stream_a)
{
	assert (header.type == cga::message_type::keepalive);
	auto error (false);
	for (auto i (peers.begin ()), j (peers.end ()); i != j && !error; ++i)
	{
		std::array<uint8_t, 16> address;
		uint16_t port;
		if (!try_read (stream_a, address) && !try_read (stream_a, port))
		{
			*i = cga::endpoint (boost::asio::ip::address_v6 (address), port);
		}
		else
		{
			error = true;
		}
	}
	return error;
}

bool cga::keepalive::operator== (cga::keepalive const & other_a) const
{
	return peers == other_a.peers;
}

cga::publish::publish (bool & error_a, cga::stream & stream_a, cga::message_header const & header_a, cga::block_uniquer * uniquer_a) :
message (header_a)
{
	if (!error_a)
	{
		error_a = deserialize (stream_a, uniquer_a);
	}
}

cga::publish::publish (std::shared_ptr<cga::block> block_a) :
message (cga::message_type::publish),
block (block_a)
{
	header.block_type_set (block->type ());
}

void cga::publish::serialize (cga::stream & stream_a) const
{
	assert (block != nullptr);
	header.serialize (stream_a);
	block->serialize (stream_a);
}

bool cga::publish::deserialize (cga::stream & stream_a, cga::block_uniquer * uniquer_a)
{
	assert (header.type == cga::message_type::publish);
	block = cga::deserialize_block (stream_a, header.block_type (), uniquer_a);
	auto result (block == nullptr);
	return result;
}

void cga::publish::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.publish (*this);
}

bool cga::publish::operator== (cga::publish const & other_a) const
{
	return *block == *other_a.block;
}

cga::confirm_req::confirm_req (bool & error_a, cga::stream & stream_a, cga::message_header const & header_a, cga::block_uniquer * uniquer_a) :
message (header_a)
{
	if (!error_a)
	{
		error_a = deserialize (stream_a, uniquer_a);
	}
}

cga::confirm_req::confirm_req (std::shared_ptr<cga::block> block_a) :
message (cga::message_type::confirm_req),
block (block_a)
{
	header.block_type_set (block->type ());
}
cga::confirm_req::confirm_req (std::vector<std::pair<cga::block_hash, cga::block_hash>> const & roots_hashes_a) :
message (cga::message_type::confirm_req),
roots_hashes (roots_hashes_a)
{
	// not_a_block (1) block type for hashes + roots request
	header.block_type_set (cga::block_type::not_a_block);
}

cga::confirm_req::confirm_req (cga::block_hash const & hash_a, cga::block_hash const & root_a) :
message (cga::message_type::confirm_req),
roots_hashes (std::vector<std::pair<cga::block_hash, cga::block_hash>> (1, std::make_pair (hash_a, root_a)))
{
	assert (!roots_hashes.empty ());
	// not_a_block (1) block type for hashes + roots request
	header.block_type_set (cga::block_type::not_a_block);
}

void cga::confirm_req::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.confirm_req (*this);
}

void cga::confirm_req::serialize (cga::stream & stream_a) const
{
	header.serialize (stream_a);
	if (header.block_type () == cga::block_type::not_a_block)
	{
		assert (!roots_hashes.empty ());
		// Calculate size
		assert (roots_hashes.size () <= 32);
		auto count = static_cast<uint8_t> (roots_hashes.size ());
		write (stream_a, count);
		// Write hashes & roots
		for (auto & root_hash : roots_hashes)
		{
			write (stream_a, root_hash.first);
			write (stream_a, root_hash.second);
		}
	}
	else
	{
		assert (block != nullptr);
		block->serialize (stream_a);
	}
}

bool cga::confirm_req::deserialize (cga::stream & stream_a, cga::block_uniquer * uniquer_a)
{
	bool result (false);
	assert (header.type == cga::message_type::confirm_req);
	try
	{
		if (header.block_type () == cga::block_type::not_a_block)
		{
			uint8_t count (0);
			read (stream_a, count);
			for (auto i (0); i != count && !result; ++i)
			{
				cga::block_hash block_hash (0);
				cga::block_hash root (0);
				read (stream_a, block_hash);
				if (!block_hash.is_zero ())
				{
					read (stream_a, root);
					if (!root.is_zero ())
					{
						roots_hashes.push_back (std::make_pair (block_hash, root));
					}
				}
			}

			result = roots_hashes.empty () || (roots_hashes.size () != count);
		}
		else
		{
			block = cga::deserialize_block (stream_a, header.block_type (), uniquer_a);
			result = block == nullptr;
		}
	}
	catch (const std::runtime_error & error)
	{
		result = true;
	}

	return result;
}

bool cga::confirm_req::operator== (cga::confirm_req const & other_a) const
{
	bool equal (false);
	if (block != nullptr && other_a.block != nullptr)
	{
		equal = *block == *other_a.block;
	}
	else if (!roots_hashes.empty () && !other_a.roots_hashes.empty ())
	{
		equal = roots_hashes == other_a.roots_hashes;
	}
	return equal;
}

std::string cga::confirm_req::roots_string () const
{
	std::string result;
	for (auto & root_hash : roots_hashes)
	{
		result += root_hash.first.to_string ();
		result += ":";
		result += root_hash.second.to_string ();
		result += ", ";
	}
	return result;
}

cga::confirm_ack::confirm_ack (bool & error_a, cga::stream & stream_a, cga::message_header const & header_a, cga::vote_uniquer * uniquer_a) :
message (header_a),
vote (std::make_shared<cga::vote> (error_a, stream_a, header.block_type ()))
{
	if (!error_a && uniquer_a)
	{
		vote = uniquer_a->unique (vote);
	}
}

cga::confirm_ack::confirm_ack (std::shared_ptr<cga::vote> vote_a) :
message (cga::message_type::confirm_ack),
vote (vote_a)
{
	assert (!vote_a->blocks.empty ());
	auto & first_vote_block (vote_a->blocks[0]);
	if (first_vote_block.which ())
	{
		header.block_type_set (cga::block_type::not_a_block);
	}
	else
	{
		header.block_type_set (boost::get<std::shared_ptr<cga::block>> (first_vote_block)->type ());
	}
}

void cga::confirm_ack::serialize (cga::stream & stream_a) const
{
	assert (header.block_type () == cga::block_type::not_a_block || header.block_type () == cga::block_type::send || header.block_type () == cga::block_type::receive || header.block_type () == cga::block_type::open || header.block_type () == cga::block_type::change || header.block_type () == cga::block_type::state);
	header.serialize (stream_a);
	vote->serialize (stream_a, header.block_type ());
}

bool cga::confirm_ack::operator== (cga::confirm_ack const & other_a) const
{
	auto result (*vote == *other_a.vote);
	return result;
}

void cga::confirm_ack::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.confirm_ack (*this);
}

cga::frontier_req::frontier_req () :
message (cga::message_type::frontier_req)
{
}

cga::frontier_req::frontier_req (bool & error_a, cga::stream & stream_a, cga::message_header const & header_a) :
message (header_a)
{
	if (!error_a)
	{
		error_a = deserialize (stream_a);
	}
}

void cga::frontier_req::serialize (cga::stream & stream_a) const
{
	header.serialize (stream_a);
	write (stream_a, start.bytes);
	write (stream_a, age);
	write (stream_a, count);
}

bool cga::frontier_req::deserialize (cga::stream & stream_a)
{
	assert (header.type == cga::message_type::frontier_req);
	auto error (false);
	try
	{
		cga::read (stream_a, start.bytes);
		cga::read (stream_a, age);
		cga::read (stream_a, count);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

void cga::frontier_req::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.frontier_req (*this);
}

bool cga::frontier_req::operator== (cga::frontier_req const & other_a) const
{
	return start == other_a.start && age == other_a.age && count == other_a.count;
}

cga::bulk_pull::bulk_pull () :
message (cga::message_type::bulk_pull),
count (0)
{
}

cga::bulk_pull::bulk_pull (bool & error_a, cga::stream & stream_a, cga::message_header const & header_a) :
message (header_a),
count (0)
{
	if (!error_a)
	{
		error_a = deserialize (stream_a);
	}
}

void cga::bulk_pull::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.bulk_pull (*this);
}

void cga::bulk_pull::serialize (cga::stream & stream_a) const
{
	/*
	 * Ensure the "count_present" flag is set if there
	 * is a limit specifed.  Additionally, do not allow
	 * the "count_present" flag with a value of 0, since
	 * that is a sentinel which we use to mean "all blocks"
	 * and that is the behavior of not having the flag set
	 * so it is wasteful to do this.
	 */
	assert ((count == 0 && !is_count_present ()) || (count != 0 && is_count_present ()));

	header.serialize (stream_a);
	write (stream_a, start);
	write (stream_a, end);

	if (is_count_present ())
	{
		std::array<uint8_t, extended_parameters_size> count_buffer{ { 0 } };
		decltype (count) count_little_endian;
		static_assert (sizeof (count_little_endian) < (count_buffer.size () - 1), "count must fit within buffer");

		count_little_endian = boost::endian::native_to_little (count);
		memcpy (count_buffer.data () + 1, &count_little_endian, sizeof (count_little_endian));

		write (stream_a, count_buffer);
	}
}

bool cga::bulk_pull::deserialize (cga::stream & stream_a)
{
	assert (header.type == cga::message_type::bulk_pull);
	auto error (false);
	try
	{
		cga::read (stream_a, start);
		cga::read (stream_a, end);

		if (is_count_present ())
		{
			std::array<uint8_t, extended_parameters_size> extended_parameters_buffers;
			static_assert (sizeof (count) < (extended_parameters_buffers.size () - 1), "count must fit within buffer");

			cga::read (stream_a, extended_parameters_buffers);
			if (extended_parameters_buffers.front () != 0)
			{
				error = true;
			}
			else
			{
				memcpy (&count, extended_parameters_buffers.data () + 1, sizeof (count));
				boost::endian::little_to_native_inplace (count);
			}
		}
		else
		{
			count = 0;
		}
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

bool cga::bulk_pull::is_count_present () const
{
	return header.extensions.test (count_present_flag);
}

void cga::bulk_pull::set_count_present (bool value_a)
{
	header.extensions.set (count_present_flag, value_a);
}

cga::bulk_pull_account::bulk_pull_account () :
message (cga::message_type::bulk_pull_account)
{
}

cga::bulk_pull_account::bulk_pull_account (bool & error_a, cga::stream & stream_a, cga::message_header const & header_a) :
message (header_a)
{
	if (!error_a)
	{
		error_a = deserialize (stream_a);
	}
}

void cga::bulk_pull_account::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.bulk_pull_account (*this);
}

void cga::bulk_pull_account::serialize (cga::stream & stream_a) const
{
	header.serialize (stream_a);
	write (stream_a, account);
	write (stream_a, minimum_amount);
	write (stream_a, flags);
}

bool cga::bulk_pull_account::deserialize (cga::stream & stream_a)
{
	assert (header.type == cga::message_type::bulk_pull_account);
	auto error (false);
	try
	{
		cga::read (stream_a, account);
		cga::read (stream_a, minimum_amount);
		cga::read (stream_a, flags);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

cga::bulk_push::bulk_push () :
message (cga::message_type::bulk_push)
{
}

cga::bulk_push::bulk_push (cga::message_header const & header_a) :
message (header_a)
{
}

bool cga::bulk_push::deserialize (cga::stream & stream_a)
{
	assert (header.type == cga::message_type::bulk_push);
	return false;
}

void cga::bulk_push::serialize (cga::stream & stream_a) const
{
	header.serialize (stream_a);
}

void cga::bulk_push::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.bulk_push (*this);
}

size_t constexpr cga::node_id_handshake::query_flag;
size_t constexpr cga::node_id_handshake::response_flag;

cga::node_id_handshake::node_id_handshake (bool & error_a, cga::stream & stream_a, cga::message_header const & header_a) :
message (header_a),
query (boost::none),
response (boost::none)
{
	error_a = deserialize (stream_a);
}

cga::node_id_handshake::node_id_handshake (boost::optional<cga::uint256_union> query, boost::optional<std::pair<cga::account, cga::signature>> response) :
message (cga::message_type::node_id_handshake),
query (query),
response (response)
{
	if (query)
	{
		set_query_flag (true);
	}
	if (response)
	{
		set_response_flag (true);
	}
}

void cga::node_id_handshake::serialize (cga::stream & stream_a) const
{
	header.serialize (stream_a);
	if (query)
	{
		write (stream_a, *query);
	}
	if (response)
	{
		write (stream_a, response->first);
		write (stream_a, response->second);
	}
}

bool cga::node_id_handshake::deserialize (cga::stream & stream_a)
{
	assert (header.type == cga::message_type::node_id_handshake);
	auto error (false);
	try
	{
		if (is_query_flag ())
		{
			cga::uint256_union query_hash;
			read (stream_a, query_hash);
			query = query_hash;
		}

		if (is_response_flag ())
		{
			cga::account response_account;
			read (stream_a, response_account);
			cga::signature response_signature;
			read (stream_a, response_signature);
			response = std::make_pair (response_account, response_signature);
		}
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

bool cga::node_id_handshake::operator== (cga::node_id_handshake const & other_a) const
{
	auto result (*query == *other_a.query && *response == *other_a.response);
	return result;
}

bool cga::node_id_handshake::is_query_flag () const
{
	return header.extensions.test (query_flag);
}

void cga::node_id_handshake::set_query_flag (bool value_a)
{
	header.extensions.set (query_flag, value_a);
}

bool cga::node_id_handshake::is_response_flag () const
{
	return header.extensions.test (response_flag);
}

void cga::node_id_handshake::set_response_flag (bool value_a)
{
	header.extensions.set (response_flag, value_a);
}

void cga::node_id_handshake::visit (cga::message_visitor & visitor_a) const
{
	visitor_a.node_id_handshake (*this);
}

cga::message_visitor::~message_visitor ()
{
}

bool cga::parse_port (std::string const & string_a, uint16_t & port_a)
{
	bool result = false;
	try
	{
		port_a = boost::lexical_cast<uint16_t> (string_a);
	}
	catch (...)
	{
		result = true;
	}
	return result;
}

bool cga::parse_address_port (std::string const & string, boost::asio::ip::address & address_a, uint16_t & port_a)
{
	auto result (false);
	auto port_position (string.rfind (':'));
	if (port_position != std::string::npos && port_position > 0)
	{
		std::string port_string (string.substr (port_position + 1));
		try
		{
			uint16_t port;
			result = parse_port (port_string, port);
			if (!result)
			{
				boost::system::error_code ec;
				auto address (boost::asio::ip::address_v6::from_string (string.substr (0, port_position), ec));
				if (!ec)
				{
					address_a = address;
					port_a = port;
				}
				else
				{
					result = true;
				}
			}
			else
			{
				result = true;
			}
		}
		catch (...)
		{
			result = true;
		}
	}
	else
	{
		result = true;
	}
	return result;
}

bool cga::parse_endpoint (std::string const & string, cga::endpoint & endpoint_a)
{
	boost::asio::ip::address address;
	uint16_t port;
	auto result (parse_address_port (string, address, port));
	if (!result)
	{
		endpoint_a = cga::endpoint (address, port);
	}
	return result;
}

bool cga::parse_tcp_endpoint (std::string const & string, cga::tcp_endpoint & endpoint_a)
{
	boost::asio::ip::address address;
	uint16_t port;
	auto result (parse_address_port (string, address, port));
	if (!result)
	{
		endpoint_a = cga::tcp_endpoint (address, port);
	}
	return result;
}
