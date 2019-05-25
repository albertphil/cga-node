#pragma once

#include <cga/lib/errors.hpp>
#include <cga/lib/numbers.hpp>
#include <cga/lib/utility.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <cassert>
#include <crypto/blake2/blake2.h>
#include <streambuf>
#include <unordered_map>

namespace cga
{
std::string to_string_hex (uint64_t);
bool from_string_hex (std::string const &, uint64_t &);
// We operate on streams of uint8_t by convention
using stream = std::basic_streambuf<uint8_t>;
// Read a raw byte stream the size of `T' and fill value.
template <typename T>
bool try_read (cga::stream & stream_a, T & value)
{
	static_assert (std::is_standard_layout<T>::value, "Can't stream read non-standard layout types");
	auto amount_read (stream_a.sgetn (reinterpret_cast<uint8_t *> (&value), sizeof (value)));
	return amount_read != sizeof (value);
}
// A wrapper of try_read which throws if there is an error
template <typename T>
void read (cga::stream & stream_a, T & value)
{
	auto error = try_read (stream_a, value);
	if (error)
	{
		throw std::runtime_error ("Failed to read type");
	}
}

template <typename T>
void write (cga::stream & stream_a, T const & value)
{
	static_assert (std::is_standard_layout<T>::value, "Can't stream write non-standard layout types");
	auto amount_written (stream_a.sputn (reinterpret_cast<uint8_t const *> (&value), sizeof (value)));
	assert (amount_written == sizeof (value));
}
class block_visitor;
enum class block_type : uint8_t
{
	invalid = 0,
	not_a_block = 1,
	send = 2,
	receive = 3,
	open = 4,
	change = 5,
	state = 6
};
class block
{
public:
	// Return a digest of the hashables in this block.
	cga::block_hash hash () const;
	// Return a digest of hashables and non-hashables in this block.
	cga::block_hash full_hash () const;
	std::string to_json () const;
	virtual void hash (blake2b_state &) const = 0;
	virtual uint64_t block_work () const = 0;
	virtual void block_work_set (uint64_t) = 0;
	virtual cga::account account () const;
	// Previous block in account's chain, zero for open block
	virtual cga::block_hash previous () const = 0;
	// Source block for open/receive blocks, zero otherwise.
	virtual cga::block_hash source () const;
	// Previous block or account number for open blocks
	virtual cga::block_hash root () const = 0;
	// Link field for state blocks, zero otherwise.
	virtual cga::block_hash link () const;
	virtual cga::account representative () const;
	virtual void serialize (cga::stream &) const = 0;
	virtual void serialize_json (std::string &) const = 0;
	virtual void visit (cga::block_visitor &) const = 0;
	virtual bool operator== (cga::block const &) const = 0;
	virtual cga::block_type type () const = 0;
	virtual cga::signature block_signature () const = 0;
	virtual void signature_set (cga::uint512_union const &) = 0;
	virtual ~block () = default;
	virtual bool valid_predecessor (cga::block const &) const = 0;
	static size_t size (cga::block_type);
};
class send_hashables
{
public:
	send_hashables () = default;
	send_hashables (cga::account const &, cga::block_hash const &, cga::amount const &);
	send_hashables (bool &, cga::stream &);
	send_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	cga::block_hash previous;
	cga::account destination;
	cga::amount balance;
	static size_t constexpr size = sizeof (previous) + sizeof (destination) + sizeof (balance);
};
class send_block : public cga::block
{
public:
	send_block () = default;
	send_block (cga::block_hash const &, cga::account const &, cga::amount const &, cga::raw_key const &, cga::public_key const &, uint64_t);
	send_block (bool &, cga::stream &);
	send_block (bool &, boost::property_tree::ptree const &);
	virtual ~send_block () = default;
	using cga::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	cga::block_hash previous () const override;
	cga::block_hash root () const override;
	void serialize (cga::stream &) const override;
	bool deserialize (cga::stream &);
	void serialize_json (std::string &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (cga::block_visitor &) const override;
	cga::block_type type () const override;
	cga::signature block_signature () const override;
	void signature_set (cga::uint512_union const &) override;
	bool operator== (cga::block const &) const override;
	bool operator== (cga::send_block const &) const;
	bool valid_predecessor (cga::block const &) const override;
	send_hashables hashables;
	cga::signature signature;
	uint64_t work;
	static size_t constexpr size = cga::send_hashables::size + sizeof (signature) + sizeof (work);
};
class receive_hashables
{
public:
	receive_hashables () = default;
	receive_hashables (cga::block_hash const &, cga::block_hash const &);
	receive_hashables (bool &, cga::stream &);
	receive_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	cga::block_hash previous;
	cga::block_hash source;
	static size_t constexpr size = sizeof (previous) + sizeof (source);
};
class receive_block : public cga::block
{
public:
	receive_block () = default;
	receive_block (cga::block_hash const &, cga::block_hash const &, cga::raw_key const &, cga::public_key const &, uint64_t);
	receive_block (bool &, cga::stream &);
	receive_block (bool &, boost::property_tree::ptree const &);
	virtual ~receive_block () = default;
	using cga::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	cga::block_hash previous () const override;
	cga::block_hash source () const override;
	cga::block_hash root () const override;
	void serialize (cga::stream &) const override;
	bool deserialize (cga::stream &);
	void serialize_json (std::string &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (cga::block_visitor &) const override;
	cga::block_type type () const override;
	cga::signature block_signature () const override;
	void signature_set (cga::uint512_union const &) override;
	bool operator== (cga::block const &) const override;
	bool operator== (cga::receive_block const &) const;
	bool valid_predecessor (cga::block const &) const override;
	receive_hashables hashables;
	cga::signature signature;
	uint64_t work;
	static size_t constexpr size = cga::receive_hashables::size + sizeof (signature) + sizeof (work);
};
class open_hashables
{
public:
	open_hashables () = default;
	open_hashables (cga::block_hash const &, cga::account const &, cga::account const &);
	open_hashables (bool &, cga::stream &);
	open_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	cga::block_hash source;
	cga::account representative;
	cga::account account;
	static size_t constexpr size = sizeof (source) + sizeof (representative) + sizeof (account);
};
class open_block : public cga::block
{
public:
	open_block () = default;
	open_block (cga::block_hash const &, cga::account const &, cga::account const &, cga::raw_key const &, cga::public_key const &, uint64_t);
	open_block (cga::block_hash const &, cga::account const &, cga::account const &, std::nullptr_t);
	open_block (bool &, cga::stream &);
	open_block (bool &, boost::property_tree::ptree const &);
	virtual ~open_block () = default;
	using cga::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	cga::block_hash previous () const override;
	cga::account account () const override;
	cga::block_hash source () const override;
	cga::block_hash root () const override;
	cga::account representative () const override;
	void serialize (cga::stream &) const override;
	bool deserialize (cga::stream &);
	void serialize_json (std::string &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (cga::block_visitor &) const override;
	cga::block_type type () const override;
	cga::signature block_signature () const override;
	void signature_set (cga::uint512_union const &) override;
	bool operator== (cga::block const &) const override;
	bool operator== (cga::open_block const &) const;
	bool valid_predecessor (cga::block const &) const override;
	cga::open_hashables hashables;
	cga::signature signature;
	uint64_t work;
	static size_t constexpr size = cga::open_hashables::size + sizeof (signature) + sizeof (work);
};
class change_hashables
{
public:
	change_hashables () = default;
	change_hashables (cga::block_hash const &, cga::account const &);
	change_hashables (bool &, cga::stream &);
	change_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	cga::block_hash previous;
	cga::account representative;
	static size_t constexpr size = sizeof (previous) + sizeof (representative);
};
class change_block : public cga::block
{
public:
	change_block () = default;
	change_block (cga::block_hash const &, cga::account const &, cga::raw_key const &, cga::public_key const &, uint64_t);
	change_block (bool &, cga::stream &);
	change_block (bool &, boost::property_tree::ptree const &);
	virtual ~change_block () = default;
	using cga::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	cga::block_hash previous () const override;
	cga::block_hash root () const override;
	cga::account representative () const override;
	void serialize (cga::stream &) const override;
	bool deserialize (cga::stream &);
	void serialize_json (std::string &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (cga::block_visitor &) const override;
	cga::block_type type () const override;
	cga::signature block_signature () const override;
	void signature_set (cga::uint512_union const &) override;
	bool operator== (cga::block const &) const override;
	bool operator== (cga::change_block const &) const;
	bool valid_predecessor (cga::block const &) const override;
	cga::change_hashables hashables;
	cga::signature signature;
	uint64_t work;
	static size_t constexpr size = cga::change_hashables::size + sizeof (signature) + sizeof (work);
};
class state_hashables
{
public:
	state_hashables () = default;
	state_hashables (cga::account const &, cga::block_hash const &, cga::account const &, cga::amount const &, cga::uint256_union const &);
	state_hashables (bool &, cga::stream &);
	state_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	// Account# / public key that operates this account
	// Uses:
	// Bulk signature validation in advance of further ledger processing
	// Arranging uncomitted transactions by account
	cga::account account;
	// Previous transaction in this chain
	cga::block_hash previous;
	// Representative of this account
	cga::account representative;
	// Current balance of this account
	// Allows lookup of account balance simply by looking at the head block
	cga::amount balance;
	// Link field contains source block_hash if receiving, destination account if sending
	cga::uint256_union link;
	// Serialized size
	static size_t constexpr size = sizeof (account) + sizeof (previous) + sizeof (representative) + sizeof (balance) + sizeof (link);
};
class state_block : public cga::block
{
public:
	state_block () = default;
	state_block (cga::account const &, cga::block_hash const &, cga::account const &, cga::amount const &, cga::uint256_union const &, cga::raw_key const &, cga::public_key const &, uint64_t);
	state_block (bool &, cga::stream &);
	state_block (bool &, boost::property_tree::ptree const &);
	virtual ~state_block () = default;
	using cga::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	cga::block_hash previous () const override;
	cga::account account () const override;
	cga::block_hash root () const override;
	cga::block_hash link () const override;
	cga::account representative () const override;
	void serialize (cga::stream &) const override;
	bool deserialize (cga::stream &);
	void serialize_json (std::string &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (cga::block_visitor &) const override;
	cga::block_type type () const override;
	cga::signature block_signature () const override;
	void signature_set (cga::uint512_union const &) override;
	bool operator== (cga::block const &) const override;
	bool operator== (cga::state_block const &) const;
	bool valid_predecessor (cga::block const &) const override;
	cga::state_hashables hashables;
	cga::signature signature;
	uint64_t work;
	static size_t constexpr size = cga::state_hashables::size + sizeof (signature) + sizeof (work);
};
class block_visitor
{
public:
	virtual void send_block (cga::send_block const &) = 0;
	virtual void receive_block (cga::receive_block const &) = 0;
	virtual void open_block (cga::open_block const &) = 0;
	virtual void change_block (cga::change_block const &) = 0;
	virtual void state_block (cga::state_block const &) = 0;
	virtual ~block_visitor () = default;
};
/**
 * This class serves to find and return unique variants of a block in order to minimize memory usage
 */
class block_uniquer
{
public:
	using value_type = std::pair<const cga::uint256_union, std::weak_ptr<cga::block>>;

	std::shared_ptr<cga::block> unique (std::shared_ptr<cga::block>);
	size_t size ();

private:
	std::mutex mutex;
	std::unordered_map<std::remove_const_t<value_type::first_type>, value_type::second_type> blocks;
	static unsigned constexpr cleanup_count = 2;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (block_uniquer & block_uniquer, const std::string & name);

std::shared_ptr<cga::block> deserialize_block (cga::stream &, cga::block_uniquer * = nullptr);
std::shared_ptr<cga::block> deserialize_block (cga::stream &, cga::block_type, cga::block_uniquer * = nullptr);
std::shared_ptr<cga::block> deserialize_block_json (boost::property_tree::ptree const &, cga::block_uniquer * = nullptr);
void serialize_block (cga::stream &, cga::block const &);
}
