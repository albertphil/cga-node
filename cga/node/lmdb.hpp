#pragma once

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <lmdb/libraries/liblmdb/lmdb.h>

#include <cga/lib/numbers.hpp>
#include <cga/node/logging.hpp>
#include <cga/secure/blockstore.hpp>
#include <cga/secure/common.hpp>

#include <thread>

namespace cga
{
class mdb_env;
class mdb_txn : public transaction_impl
{
public:
	mdb_txn (cga::mdb_env const &, bool = false);
	mdb_txn (cga::mdb_txn const &) = delete;
	mdb_txn (cga::mdb_txn &&) = default;
	~mdb_txn ();
	cga::mdb_txn & operator= (cga::mdb_txn const &) = delete;
	cga::mdb_txn & operator= (cga::mdb_txn &&) = default;
	operator MDB_txn * () const;
	MDB_txn * handle;
};
/**
 * RAII wrapper for MDB_env
 */
class mdb_env
{
public:
	mdb_env (bool &, boost::filesystem::path const &, int max_dbs = 128, size_t map_size = 128ULL * 1024 * 1024 * 1024);
	~mdb_env ();
	operator MDB_env * () const;
	cga::transaction tx_begin (bool = false) const;
	MDB_txn * tx (cga::transaction const &) const;
	MDB_env * environment;
};

/**
 * Encapsulates MDB_val and provides uint256_union conversion of the data.
 */
class mdb_val
{
public:
	mdb_val (cga::epoch = cga::epoch::unspecified);
	mdb_val (cga::account_info const &);
	mdb_val (cga::block_info const &);
	mdb_val (MDB_val const &, cga::epoch = cga::epoch::unspecified);
	mdb_val (cga::pending_info const &);
	mdb_val (cga::pending_key const &);
	mdb_val (cga::unchecked_info const &);
	mdb_val (size_t, void *);
	mdb_val (cga::uint128_union const &);
	mdb_val (cga::uint256_union const &);
	mdb_val (cga::endpoint_key const &);
	mdb_val (std::shared_ptr<cga::block> const &);
	mdb_val (std::shared_ptr<cga::vote> const &);
	mdb_val (uint64_t);
	void * data () const;
	size_t size () const;
	explicit operator cga::account_info () const;
	explicit operator cga::block_info () const;
	explicit operator cga::pending_info () const;
	explicit operator cga::pending_key () const;
	explicit operator cga::unchecked_info () const;
	explicit operator cga::uint128_union () const;
	explicit operator cga::uint256_union () const;
	explicit operator std::array<char, 64> () const;
	explicit operator cga::endpoint_key () const;
	explicit operator cga::no_value () const;
	explicit operator std::shared_ptr<cga::block> () const;
	explicit operator std::shared_ptr<cga::send_block> () const;
	explicit operator std::shared_ptr<cga::receive_block> () const;
	explicit operator std::shared_ptr<cga::open_block> () const;
	explicit operator std::shared_ptr<cga::change_block> () const;
	explicit operator std::shared_ptr<cga::state_block> () const;
	explicit operator std::shared_ptr<cga::vote> () const;
	explicit operator uint64_t () const;
	operator MDB_val * () const;
	operator MDB_val const & () const;
	MDB_val value;
	std::shared_ptr<std::vector<uint8_t>> buffer;
	cga::epoch epoch{ cga::epoch::unspecified };
};
class block_store;

template <typename T, typename U>
class mdb_iterator : public store_iterator_impl<T, U>
{
public:
	mdb_iterator (cga::transaction const & transaction_a, MDB_dbi db_a, cga::epoch = cga::epoch::unspecified);
	mdb_iterator (std::nullptr_t, cga::epoch = cga::epoch::unspecified);
	mdb_iterator (cga::transaction const & transaction_a, MDB_dbi db_a, MDB_val const & val_a, cga::epoch = cga::epoch::unspecified);
	mdb_iterator (cga::mdb_iterator<T, U> && other_a);
	mdb_iterator (cga::mdb_iterator<T, U> const &) = delete;
	~mdb_iterator ();
	cga::store_iterator_impl<T, U> & operator++ () override;
	std::pair<cga::mdb_val, cga::mdb_val> * operator-> ();
	bool operator== (cga::store_iterator_impl<T, U> const & other_a) const override;
	bool is_end_sentinal () const override;
	void fill (std::pair<T, U> &) const override;
	void clear ();
	cga::mdb_iterator<T, U> & operator= (cga::mdb_iterator<T, U> && other_a);
	cga::store_iterator_impl<T, U> & operator= (cga::store_iterator_impl<T, U> const &) = delete;
	MDB_cursor * cursor;
	std::pair<cga::mdb_val, cga::mdb_val> current;

private:
	MDB_txn * tx (cga::transaction const &) const;
};

/**
 * Iterates the key/value pairs of two stores merged together
 */
template <typename T, typename U>
class mdb_merge_iterator : public store_iterator_impl<T, U>
{
public:
	mdb_merge_iterator (cga::transaction const &, MDB_dbi, MDB_dbi);
	mdb_merge_iterator (std::nullptr_t);
	mdb_merge_iterator (cga::transaction const &, MDB_dbi, MDB_dbi, MDB_val const &);
	mdb_merge_iterator (cga::mdb_merge_iterator<T, U> &&);
	mdb_merge_iterator (cga::mdb_merge_iterator<T, U> const &) = delete;
	~mdb_merge_iterator ();
	cga::store_iterator_impl<T, U> & operator++ () override;
	std::pair<cga::mdb_val, cga::mdb_val> * operator-> ();
	bool operator== (cga::store_iterator_impl<T, U> const &) const override;
	bool is_end_sentinal () const override;
	void fill (std::pair<T, U> &) const override;
	void clear ();
	cga::mdb_merge_iterator<T, U> & operator= (cga::mdb_merge_iterator<T, U> &&) = default;
	cga::mdb_merge_iterator<T, U> & operator= (cga::mdb_merge_iterator<T, U> const &) = delete;

private:
	cga::mdb_iterator<T, U> & least_iterator () const;
	std::unique_ptr<cga::mdb_iterator<T, U>> impl1;
	std::unique_ptr<cga::mdb_iterator<T, U>> impl2;
};

class logging;
/**
 * mdb implementation of the block store
 */
class mdb_store : public block_store
{
	friend class cga::block_predecessor_set;

public:
	mdb_store (bool &, cga::logging &, boost::filesystem::path const &, int lmdb_max_dbs = 128, bool drop_unchecked = false, size_t batch_size = 512);
	~mdb_store ();

	cga::transaction tx_begin_write () override;
	cga::transaction tx_begin_read () override;
	cga::transaction tx_begin (bool write = false) override;

	void initialize (cga::transaction const &, cga::genesis const &) override;
	void block_put (cga::transaction const &, cga::block_hash const &, cga::block const &, cga::block_sideband const &, cga::epoch version = cga::epoch::epoch_0) override;
	size_t block_successor_offset (cga::transaction const &, MDB_val, cga::block_type);
	cga::block_hash block_successor (cga::transaction const &, cga::block_hash const &) override;
	void block_successor_clear (cga::transaction const &, cga::block_hash const &) override;
	std::shared_ptr<cga::block> block_get (cga::transaction const &, cga::block_hash const &, cga::block_sideband * = nullptr) override;
	std::shared_ptr<cga::block> block_random (cga::transaction const &) override;
	void block_del (cga::transaction const &, cga::block_hash const &) override;
	bool block_exists (cga::transaction const &, cga::block_hash const &) override;
	bool block_exists (cga::transaction const &, cga::block_type, cga::block_hash const &) override;
	cga::block_counts block_count (cga::transaction const &) override;
	bool root_exists (cga::transaction const &, cga::uint256_union const &) override;
	bool source_exists (cga::transaction const &, cga::block_hash const &) override;
	cga::account block_account (cga::transaction const &, cga::block_hash const &) override;

	void frontier_put (cga::transaction const &, cga::block_hash const &, cga::account const &) override;
	cga::account frontier_get (cga::transaction const &, cga::block_hash const &) override;
	void frontier_del (cga::transaction const &, cga::block_hash const &) override;

	void account_put (cga::transaction const &, cga::account const &, cga::account_info const &) override;
	bool account_get (cga::transaction const &, cga::account const &, cga::account_info &) override;
	void account_del (cga::transaction const &, cga::account const &) override;
	bool account_exists (cga::transaction const &, cga::account const &) override;
	size_t account_count (cga::transaction const &) override;
	cga::store_iterator<cga::account, cga::account_info> latest_v0_begin (cga::transaction const &, cga::account const &) override;
	cga::store_iterator<cga::account, cga::account_info> latest_v0_begin (cga::transaction const &) override;
	cga::store_iterator<cga::account, cga::account_info> latest_v0_end () override;
	cga::store_iterator<cga::account, cga::account_info> latest_v1_begin (cga::transaction const &, cga::account const &) override;
	cga::store_iterator<cga::account, cga::account_info> latest_v1_begin (cga::transaction const &) override;
	cga::store_iterator<cga::account, cga::account_info> latest_v1_end () override;
	cga::store_iterator<cga::account, cga::account_info> latest_begin (cga::transaction const &, cga::account const &) override;
	cga::store_iterator<cga::account, cga::account_info> latest_begin (cga::transaction const &) override;
	cga::store_iterator<cga::account, cga::account_info> latest_end () override;

	void pending_put (cga::transaction const &, cga::pending_key const &, cga::pending_info const &) override;
	void pending_del (cga::transaction const &, cga::pending_key const &) override;
	bool pending_get (cga::transaction const &, cga::pending_key const &, cga::pending_info &) override;
	bool pending_exists (cga::transaction const &, cga::pending_key const &) override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_v0_begin (cga::transaction const &, cga::pending_key const &) override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_v0_begin (cga::transaction const &) override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_v0_end () override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_v1_begin (cga::transaction const &, cga::pending_key const &) override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_v1_begin (cga::transaction const &) override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_v1_end () override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_begin (cga::transaction const &, cga::pending_key const &) override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_begin (cga::transaction const &) override;
	cga::store_iterator<cga::pending_key, cga::pending_info> pending_end () override;

	bool block_info_get (cga::transaction const &, cga::block_hash const &, cga::block_info &) override;
	cga::uint128_t block_balance (cga::transaction const &, cga::block_hash const &) override;
	cga::epoch block_version (cga::transaction const &, cga::block_hash const &) override;

	cga::uint128_t representation_get (cga::transaction const &, cga::account const &) override;
	void representation_put (cga::transaction const &, cga::account const &, cga::uint128_t const &) override;
	void representation_add (cga::transaction const &, cga::account const &, cga::uint128_t const &) override;
	cga::store_iterator<cga::account, cga::uint128_union> representation_begin (cga::transaction const &) override;
	cga::store_iterator<cga::account, cga::uint128_union> representation_end () override;

	void unchecked_clear (cga::transaction const &) override;
	void unchecked_put (cga::transaction const &, cga::unchecked_key const &, cga::unchecked_info const &) override;
	void unchecked_put (cga::transaction const &, cga::block_hash const &, std::shared_ptr<cga::block> const &) override;
	std::vector<cga::unchecked_info> unchecked_get (cga::transaction const &, cga::block_hash const &) override;
	bool unchecked_exists (cga::transaction const &, cga::unchecked_key const &) override;
	void unchecked_del (cga::transaction const &, cga::unchecked_key const &) override;
	cga::store_iterator<cga::unchecked_key, cga::unchecked_info> unchecked_begin (cga::transaction const &) override;
	cga::store_iterator<cga::unchecked_key, cga::unchecked_info> unchecked_begin (cga::transaction const &, cga::unchecked_key const &) override;
	cga::store_iterator<cga::unchecked_key, cga::unchecked_info> unchecked_end () override;
	size_t unchecked_count (cga::transaction const &) override;

	// Return latest vote for an account from store
	std::shared_ptr<cga::vote> vote_get (cga::transaction const &, cga::account const &) override;
	// Populate vote with the next sequence number
	std::shared_ptr<cga::vote> vote_generate (cga::transaction const &, cga::account const &, cga::raw_key const &, std::shared_ptr<cga::block>) override;
	std::shared_ptr<cga::vote> vote_generate (cga::transaction const &, cga::account const &, cga::raw_key const &, std::vector<cga::block_hash>) override;
	// Return either vote or the stored vote with a higher sequence number
	std::shared_ptr<cga::vote> vote_max (cga::transaction const &, std::shared_ptr<cga::vote>) override;
	// Return latest vote for an account considering the vote cache
	std::shared_ptr<cga::vote> vote_current (cga::transaction const &, cga::account const &) override;
	void flush (cga::transaction const &) override;
	cga::store_iterator<cga::account, std::shared_ptr<cga::vote>> vote_begin (cga::transaction const &) override;
	cga::store_iterator<cga::account, std::shared_ptr<cga::vote>> vote_end () override;

	void online_weight_put (cga::transaction const &, uint64_t, cga::amount const &) override;
	void online_weight_del (cga::transaction const &, uint64_t) override;
	cga::store_iterator<uint64_t, cga::amount> online_weight_begin (cga::transaction const &) override;
	cga::store_iterator<uint64_t, cga::amount> online_weight_end () override;
	size_t online_weight_count (cga::transaction const &) const override;
	void online_weight_clear (cga::transaction const &) override;

	std::mutex cache_mutex;
	std::unordered_map<cga::account, std::shared_ptr<cga::vote>> vote_cache_l1;
	std::unordered_map<cga::account, std::shared_ptr<cga::vote>> vote_cache_l2;

	void version_put (cga::transaction const &, int) override;
	int version_get (cga::transaction const &) override;
	void do_upgrades (cga::transaction const &, bool &);
	void upgrade_v1_to_v2 (cga::transaction const &);
	void upgrade_v2_to_v3 (cga::transaction const &);
	void upgrade_v3_to_v4 (cga::transaction const &);
	void upgrade_v4_to_v5 (cga::transaction const &);
	void upgrade_v5_to_v6 (cga::transaction const &);
	void upgrade_v6_to_v7 (cga::transaction const &);
	void upgrade_v7_to_v8 (cga::transaction const &);
	void upgrade_v8_to_v9 (cga::transaction const &);
	void upgrade_v9_to_v10 (cga::transaction const &);
	void upgrade_v10_to_v11 (cga::transaction const &);
	void upgrade_v11_to_v12 (cga::transaction const &);
	void do_slow_upgrades (size_t const);
	void upgrade_v12_to_v13 (size_t const);
	bool full_sideband (cga::transaction const &);

	// Requires a write transaction
	cga::raw_key get_node_id (cga::transaction const &) override;

	/** Deletes the node ID from the store */
	void delete_node_id (cga::transaction const &) override;

	void peer_put (cga::transaction const & transaction_a, cga::endpoint_key const & endpoint_a) override;
	bool peer_exists (cga::transaction const & transaction_a, cga::endpoint_key const & endpoint_a) const override;
	void peer_del (cga::transaction const & transaction_a, cga::endpoint_key const & endpoint_a) override;
	size_t peer_count (cga::transaction const & transaction_a) const override;
	void peer_clear (cga::transaction const & transaction_a) override;

	cga::store_iterator<cga::endpoint_key, cga::no_value> peers_begin (cga::transaction const & transaction_a) override;
	cga::store_iterator<cga::endpoint_key, cga::no_value> peers_end () override;

	void stop ();

	cga::logging & logging;

	cga::mdb_env env;

	/**
	 * Maps head block to owning account
	 * cga::block_hash -> cga::account
	 */
	MDB_dbi frontiers{ 0 };

	/**
	 * Maps account v1 to account information, head, rep, open, balance, timestamp and block count.
	 * cga::account -> cga::block_hash, cga::block_hash, cga::block_hash, cga::amount, uint64_t, uint64_t
	 */
	MDB_dbi accounts_v0{ 0 };

	/**
	 * Maps account v0 to account information, head, rep, open, balance, timestamp and block count.
	 * cga::account -> cga::block_hash, cga::block_hash, cga::block_hash, cga::amount, uint64_t, uint64_t
	 */
	MDB_dbi accounts_v1{ 0 };

	/**
	 * Maps block hash to send block.
	 * cga::block_hash -> cga::send_block
	 */
	MDB_dbi send_blocks{ 0 };

	/**
	 * Maps block hash to receive block.
	 * cga::block_hash -> cga::receive_block
	 */
	MDB_dbi receive_blocks{ 0 };

	/**
	 * Maps block hash to open block.
	 * cga::block_hash -> cga::open_block
	 */
	MDB_dbi open_blocks{ 0 };

	/**
	 * Maps block hash to change block.
	 * cga::block_hash -> cga::change_block
	 */
	MDB_dbi change_blocks{ 0 };

	/**
	 * Maps block hash to v0 state block.
	 * cga::block_hash -> cga::state_block
	 */
	MDB_dbi state_blocks_v0{ 0 };

	/**
	 * Maps block hash to v1 state block.
	 * cga::block_hash -> cga::state_block
	 */
	MDB_dbi state_blocks_v1{ 0 };

	/**
	 * Maps min_version 0 (destination account, pending block) to (source account, amount).
	 * cga::account, cga::block_hash -> cga::account, cga::amount
	 */
	MDB_dbi pending_v0{ 0 };

	/**
	 * Maps min_version 1 (destination account, pending block) to (source account, amount).
	 * cga::account, cga::block_hash -> cga::account, cga::amount
	 */
	MDB_dbi pending_v1{ 0 };

	/**
	 * Maps block hash to account and balance.
	 * block_hash -> cga::account, cga::amount
	 */
	MDB_dbi blocks_info{ 0 };

	/**
	 * Representative weights.
	 * cga::account -> cga::uint128_t
	 */
	MDB_dbi representation{ 0 };

	/**
	 * Unchecked bootstrap blocks info.
	 * cga::block_hash -> cga::unchecked_info
	 */
	MDB_dbi unchecked{ 0 };

	/**
	 * Highest vote observed for account.
	 * cga::account -> uint64_t
	 */
	MDB_dbi vote{ 0 };

	/**
	 * Samples of online vote weight
	 * uint64_t -> cga::amount
	 */
	MDB_dbi online_weight{ 0 };

	/**
	 * Meta information about block store, such as versions.
	 * cga::uint256_union (arbitrary key) -> blob
	 */
	MDB_dbi meta{ 0 };

	/*
	 * Endpoints for peers
	 * cga::endpoint_key -> no_value
	*/
	MDB_dbi peers{ 0 };

private:
	bool entry_has_sideband (MDB_val, cga::block_type);
	cga::account block_account_computed (cga::transaction const &, cga::block_hash const &);
	cga::uint128_t block_balance_computed (cga::transaction const &, cga::block_hash const &);
	MDB_dbi block_database (cga::block_type, cga::epoch);
	template <typename T>
	std::shared_ptr<cga::block> block_random (cga::transaction const &, MDB_dbi);
	MDB_val block_raw_get (cga::transaction const &, cga::block_hash const &, cga::block_type &);
	boost::optional<MDB_val> block_raw_get_by_type (cga::transaction const &, cga::block_hash const &, cga::block_type &);
	void block_raw_put (cga::transaction const &, MDB_dbi, cga::block_hash const &, MDB_val);
	void clear (MDB_dbi);
	std::atomic<bool> stopped{ false };
	std::thread upgrades;
};
class wallet_value
{
public:
	wallet_value () = default;
	wallet_value (cga::mdb_val const &);
	wallet_value (cga::uint256_union const &, uint64_t);
	cga::mdb_val val () const;
	cga::private_key key;
	uint64_t work;
};
}
