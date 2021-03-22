#pragma once

#include <cga/lib/blocks.hpp>
#include <cga/node/lmdb.hpp>
#include <cga/secure/utility.hpp>

namespace cga
{
class account_info_v1
{
public:
	account_info_v1 ();
	account_info_v1 (MDB_val const &);
	account_info_v1 (cga::account_info_v1 const &) = default;
	account_info_v1 (cga::block_hash const &, cga::block_hash const &, cga::amount const &, uint64_t);
	void serialize (cga::stream &) const;
	bool deserialize (cga::stream &);
	cga::mdb_val val () const;
	cga::block_hash head;
	cga::block_hash rep_block;
	cga::amount balance;
	uint64_t modified;
};
class pending_info_v3
{
public:
	pending_info_v3 ();
	pending_info_v3 (MDB_val const &);
	pending_info_v3 (cga::account const &, cga::amount const &, cga::account const &);
	void serialize (cga::stream &) const;
	bool deserialize (cga::stream &);
	bool operator== (cga::pending_info_v3 const &) const;
	cga::mdb_val val () const;
	cga::account source;
	cga::amount amount;
	cga::account destination;
};
// Latest information about an account
class account_info_v5
{
public:
	account_info_v5 ();
	account_info_v5 (MDB_val const &);
	account_info_v5 (cga::account_info_v5 const &) = default;
	account_info_v5 (cga::block_hash const &, cga::block_hash const &, cga::block_hash const &, cga::amount const &, uint64_t);
	void serialize (cga::stream &) const;
	bool deserialize (cga::stream &);
	cga::mdb_val val () const;
	cga::block_hash head;
	cga::block_hash rep_block;
	cga::block_hash open_block;
	cga::amount balance;
	uint64_t modified;
};
}
