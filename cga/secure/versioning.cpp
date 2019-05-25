#include <cga/secure/versioning.hpp>

cga::account_info_v1::account_info_v1 () :
head (0),
rep_block (0),
balance (0),
modified (0)
{
}

cga::account_info_v1::account_info_v1 (MDB_val const & val_a)
{
	assert (val_a.mv_size == sizeof (*this));
	static_assert (sizeof (head) + sizeof (rep_block) + sizeof (balance) + sizeof (modified) == sizeof (*this), "Class not packed");
	std::copy (reinterpret_cast<uint8_t const *> (val_a.mv_data), reinterpret_cast<uint8_t const *> (val_a.mv_data) + sizeof (*this), reinterpret_cast<uint8_t *> (this));
}

cga::account_info_v1::account_info_v1 (cga::block_hash const & head_a, cga::block_hash const & rep_block_a, cga::amount const & balance_a, uint64_t modified_a) :
head (head_a),
rep_block (rep_block_a),
balance (balance_a),
modified (modified_a)
{
}

void cga::account_info_v1::serialize (cga::stream & stream_a) const
{
	write (stream_a, head.bytes);
	write (stream_a, rep_block.bytes);
	write (stream_a, balance.bytes);
	write (stream_a, modified);
}

bool cga::account_info_v1::deserialize (cga::stream & stream_a)
{
	auto error (false);
	try
	{
		read (stream_a, head.bytes);
		read (stream_a, rep_block.bytes);
		read (stream_a, balance.bytes);
		read (stream_a, modified);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

cga::mdb_val cga::account_info_v1::val () const
{
	return cga::mdb_val (sizeof (*this), const_cast<cga::account_info_v1 *> (this));
}

cga::pending_info_v3::pending_info_v3 () :
source (0),
amount (0),
destination (0)
{
}

cga::pending_info_v3::pending_info_v3 (MDB_val const & val_a)
{
	assert (val_a.mv_size == sizeof (*this));
	static_assert (sizeof (source) + sizeof (amount) + sizeof (destination) == sizeof (*this), "Packed class");
	std::copy (reinterpret_cast<uint8_t const *> (val_a.mv_data), reinterpret_cast<uint8_t const *> (val_a.mv_data) + sizeof (*this), reinterpret_cast<uint8_t *> (this));
}

cga::pending_info_v3::pending_info_v3 (cga::account const & source_a, cga::amount const & amount_a, cga::account const & destination_a) :
source (source_a),
amount (amount_a),
destination (destination_a)
{
}

void cga::pending_info_v3::serialize (cga::stream & stream_a) const
{
	cga::write (stream_a, source.bytes);
	cga::write (stream_a, amount.bytes);
	cga::write (stream_a, destination.bytes);
}

bool cga::pending_info_v3::deserialize (cga::stream & stream_a)
{
	auto error (false);
	try
	{
		read (stream_a, source.bytes);
		read (stream_a, amount.bytes);
		read (stream_a, destination.bytes);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

bool cga::pending_info_v3::operator== (cga::pending_info_v3 const & other_a) const
{
	return source == other_a.source && amount == other_a.amount && destination == other_a.destination;
}

cga::mdb_val cga::pending_info_v3::val () const
{
	return cga::mdb_val (sizeof (*this), const_cast<cga::pending_info_v3 *> (this));
}

cga::account_info_v5::account_info_v5 () :
head (0),
rep_block (0),
open_block (0),
balance (0),
modified (0)
{
}

cga::account_info_v5::account_info_v5 (MDB_val const & val_a)
{
	assert (val_a.mv_size == sizeof (*this));
	static_assert (sizeof (head) + sizeof (rep_block) + sizeof (open_block) + sizeof (balance) + sizeof (modified) == sizeof (*this), "Class not packed");
	std::copy (reinterpret_cast<uint8_t const *> (val_a.mv_data), reinterpret_cast<uint8_t const *> (val_a.mv_data) + sizeof (*this), reinterpret_cast<uint8_t *> (this));
}

cga::account_info_v5::account_info_v5 (cga::block_hash const & head_a, cga::block_hash const & rep_block_a, cga::block_hash const & open_block_a, cga::amount const & balance_a, uint64_t modified_a) :
head (head_a),
rep_block (rep_block_a),
open_block (open_block_a),
balance (balance_a),
modified (modified_a)
{
}

void cga::account_info_v5::serialize (cga::stream & stream_a) const
{
	write (stream_a, head.bytes);
	write (stream_a, rep_block.bytes);
	write (stream_a, open_block.bytes);
	write (stream_a, balance.bytes);
	write (stream_a, modified);
}

bool cga::account_info_v5::deserialize (cga::stream & stream_a)
{
	auto error (false);
	try
	{
		read (stream_a, head.bytes);
		read (stream_a, rep_block.bytes);
		read (stream_a, open_block.bytes);
		read (stream_a, balance.bytes);
		read (stream_a, modified);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

cga::mdb_val cga::account_info_v5::val () const
{
	return cga::mdb_val (sizeof (*this), const_cast<cga::account_info_v5 *> (this));
}
