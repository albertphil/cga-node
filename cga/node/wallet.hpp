#pragma once

#include <boost/thread/thread.hpp>
#include <cga/node/lmdb.hpp>
#include <cga/node/openclwork.hpp>
#include <cga/secure/blockstore.hpp>
#include <cga/secure/common.hpp>

#include <mutex>
#include <unordered_set>

namespace cga
{
// The fan spreads a key out over the heap to decrease the likelihood of it being recovered by memory inspection
class fan
{
public:
	fan (cga::uint256_union const &, size_t);
	void value (cga::raw_key &);
	void value_set (cga::raw_key const &);
	std::vector<std::unique_ptr<cga::uint256_union>> values;

private:
	std::mutex mutex;
	void value_get (cga::raw_key &);
};
class node_config;
class kdf
{
public:
	void phs (cga::raw_key &, std::string const &, cga::uint256_union const &);
	std::mutex mutex;
};
enum class key_type
{
	not_a_type,
	unknown,
	adhoc,
	deterministic
};
class wallet_store
{
public:
	wallet_store (bool &, cga::kdf &, cga::transaction &, cga::account, unsigned, std::string const &);
	wallet_store (bool &, cga::kdf &, cga::transaction &, cga::account, unsigned, std::string const &, std::string const &);
	std::vector<cga::account> accounts (cga::transaction const &);
	void initialize (cga::transaction const &, bool &, std::string const &);
	cga::uint256_union check (cga::transaction const &);
	bool rekey (cga::transaction const &, std::string const &);
	bool valid_password (cga::transaction const &);
	bool attempt_password (cga::transaction const &, std::string const &);
	void wallet_key (cga::raw_key &, cga::transaction const &);
	void seed (cga::raw_key &, cga::transaction const &);
	void seed_set (cga::transaction const &, cga::raw_key const &);
	cga::key_type key_type (cga::wallet_value const &);
	cga::public_key deterministic_insert (cga::transaction const &);
	cga::public_key deterministic_insert (cga::transaction const &, uint32_t const);
	void deterministic_key (cga::raw_key &, cga::transaction const &, uint32_t);
	uint32_t deterministic_index_get (cga::transaction const &);
	void deterministic_index_set (cga::transaction const &, uint32_t);
	void deterministic_clear (cga::transaction const &);
	cga::uint256_union salt (cga::transaction const &);
	bool is_representative (cga::transaction const &);
	cga::account representative (cga::transaction const &);
	void representative_set (cga::transaction const &, cga::account const &);
	cga::public_key insert_adhoc (cga::transaction const &, cga::raw_key const &);
	void insert_watch (cga::transaction const &, cga::public_key const &);
	void erase (cga::transaction const &, cga::public_key const &);
	cga::wallet_value entry_get_raw (cga::transaction const &, cga::public_key const &);
	void entry_put_raw (cga::transaction const &, cga::public_key const &, cga::wallet_value const &);
	bool fetch (cga::transaction const &, cga::public_key const &, cga::raw_key &);
	bool exists (cga::transaction const &, cga::public_key const &);
	void destroy (cga::transaction const &);
	cga::store_iterator<cga::uint256_union, cga::wallet_value> find (cga::transaction const &, cga::uint256_union const &);
	cga::store_iterator<cga::uint256_union, cga::wallet_value> begin (cga::transaction const &, cga::uint256_union const &);
	cga::store_iterator<cga::uint256_union, cga::wallet_value> begin (cga::transaction const &);
	cga::store_iterator<cga::uint256_union, cga::wallet_value> end ();
	void derive_key (cga::raw_key &, cga::transaction const &, std::string const &);
	void serialize_json (cga::transaction const &, std::string &);
	void write_backup (cga::transaction const &, boost::filesystem::path const &);
	bool move (cga::transaction const &, cga::wallet_store &, std::vector<cga::public_key> const &);
	bool import (cga::transaction const &, cga::wallet_store &);
	bool work_get (cga::transaction const &, cga::public_key const &, uint64_t &);
	void work_put (cga::transaction const &, cga::public_key const &, uint64_t);
	unsigned version (cga::transaction const &);
	void version_put (cga::transaction const &, unsigned);
	void upgrade_v1_v2 (cga::transaction const &);
	void upgrade_v2_v3 (cga::transaction const &);
	void upgrade_v3_v4 (cga::transaction const &);
	cga::fan password;
	cga::fan wallet_key_mem;
	static unsigned const version_1 = 1;
	static unsigned const version_2 = 2;
	static unsigned const version_3 = 3;
	static unsigned const version_4 = 4;
	unsigned const version_current = version_4;
	static cga::uint256_union const version_special;
	static cga::uint256_union const wallet_key_special;
	static cga::uint256_union const salt_special;
	static cga::uint256_union const check_special;
	static cga::uint256_union const representative_special;
	static cga::uint256_union const seed_special;
	static cga::uint256_union const deterministic_index_special;
	static size_t const check_iv_index;
	static size_t const seed_iv_index;
	static int const special_count;
	static unsigned const kdf_full_work = 64 * 1024;
	static unsigned const kdf_test_work = 8;
	static unsigned const kdf_work = cga::is_test_network ? kdf_test_work : kdf_full_work;
	cga::kdf & kdf;
	MDB_dbi handle;
	std::recursive_mutex mutex;

private:
	MDB_txn * tx (cga::transaction const &) const;
};
class wallets;
// A wallet is a set of account keys encrypted by a common encryption key
class wallet : public std::enable_shared_from_this<cga::wallet>
{
public:
	std::shared_ptr<cga::block> change_action (cga::account const &, cga::account const &, uint64_t = 0, bool = true);
	std::shared_ptr<cga::block> receive_action (cga::block const &, cga::account const &, cga::uint128_union const &, uint64_t = 0, bool = true);
	std::shared_ptr<cga::block> send_action (cga::account const &, cga::account const &, cga::uint128_t const &, uint64_t = 0, bool = true, boost::optional<std::string> = {});
	wallet (bool &, cga::transaction &, cga::wallets &, std::string const &);
	wallet (bool &, cga::transaction &, cga::wallets &, std::string const &, std::string const &);
	void enter_initial_password ();
	bool enter_password (cga::transaction const &, std::string const &);
	cga::public_key insert_adhoc (cga::raw_key const &, bool = true);
	cga::public_key insert_adhoc (cga::transaction const &, cga::raw_key const &, bool = true);
	void insert_watch (cga::transaction const &, cga::public_key const &);
	cga::public_key deterministic_insert (cga::transaction const &, bool = true);
	cga::public_key deterministic_insert (uint32_t, bool = true);
	cga::public_key deterministic_insert (bool = true);
	bool exists (cga::public_key const &);
	bool import (std::string const &, std::string const &);
	void serialize (std::string &);
	bool change_sync (cga::account const &, cga::account const &);
	void change_async (cga::account const &, cga::account const &, std::function<void(std::shared_ptr<cga::block>)> const &, uint64_t = 0, bool = true);
	bool receive_sync (std::shared_ptr<cga::block>, cga::account const &, cga::uint128_t const &);
	void receive_async (std::shared_ptr<cga::block>, cga::account const &, cga::uint128_t const &, std::function<void(std::shared_ptr<cga::block>)> const &, uint64_t = 0, bool = true);
	cga::block_hash send_sync (cga::account const &, cga::account const &, cga::uint128_t const &);
	void send_async (cga::account const &, cga::account const &, cga::uint128_t const &, std::function<void(std::shared_ptr<cga::block>)> const &, uint64_t = 0, bool = true, boost::optional<std::string> = {});
	void work_apply (cga::account const &, std::function<void(uint64_t)>);
	void work_cache_blocking (cga::account const &, cga::block_hash const &);
	void work_update (cga::transaction const &, cga::account const &, cga::block_hash const &, uint64_t);
	void work_ensure (cga::account const &, cga::block_hash const &);
	bool search_pending ();
	void init_free_accounts (cga::transaction const &);
	uint32_t deterministic_check (cga::transaction const & transaction_a, uint32_t index);
	/** Changes the wallet seed and returns the first account */
	cga::public_key change_seed (cga::transaction const & transaction_a, cga::raw_key const & prv_a, uint32_t count = 0);
	void deterministic_restore (cga::transaction const & transaction_a);
	bool live ();
	std::unordered_set<cga::account> free_accounts;
	std::function<void(bool, bool)> lock_observer;
	cga::wallet_store store;
	cga::wallets & wallets;
	std::mutex representatives_mutex;
	std::unordered_set<cga::account> representatives;
};
class node;

/**
 * The wallets set is all the wallets a node controls.
 * A node may contain multiple wallets independently encrypted and operated.
 */
class wallets
{
public:
	wallets (bool &, cga::node &);
	~wallets ();
	std::shared_ptr<cga::wallet> open (cga::uint256_union const &);
	std::shared_ptr<cga::wallet> create (cga::uint256_union const &);
	bool search_pending (cga::uint256_union const &);
	void search_pending_all ();
	void destroy (cga::uint256_union const &);
	void reload ();
	void do_wallet_actions ();
	void queue_wallet_action (cga::uint128_t const &, std::shared_ptr<cga::wallet>, std::function<void(cga::wallet &)> const &);
	void foreach_representative (cga::transaction const &, std::function<void(cga::public_key const &, cga::raw_key const &)> const &);
	bool exists (cga::transaction const &, cga::public_key const &);
	void stop ();
	void clear_send_ids (cga::transaction const &);
	void compute_reps ();
	void ongoing_compute_reps ();
	void split_if_needed (cga::transaction &, cga::block_store &);
	void move_table (std::string const &, MDB_txn *, MDB_txn *);
	std::function<void(bool)> observer;
	std::unordered_map<cga::uint256_union, std::shared_ptr<cga::wallet>> items;
	std::multimap<cga::uint128_t, std::pair<std::shared_ptr<cga::wallet>, std::function<void(cga::wallet &)>>, std::greater<cga::uint128_t>> actions;
	std::mutex mutex;
	std::mutex action_mutex;
	std::condition_variable condition;
	cga::kdf kdf;
	MDB_dbi handle;
	MDB_dbi send_action_ids;
	cga::node & node;
	cga::mdb_env & env;
	std::atomic<bool> stopped;
	boost::thread thread;
	static cga::uint128_t const generate_priority;
	static cga::uint128_t const high_priority;
	std::atomic<uint64_t> reps_count{ 0 };

	/** Start read-write transaction */
	cga::transaction tx_begin_write ();

	/** Start read-only transaction */
	cga::transaction tx_begin_read ();

	/**
	 * Start a read-only or read-write transaction
	 * @param write If true, start a read-write transaction
	 */
	cga::transaction tx_begin (bool write = false);
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (wallets & wallets, const std::string & name);

class wallets_store
{
public:
	virtual ~wallets_store () = default;
};
class mdb_wallets_store : public wallets_store
{
public:
	mdb_wallets_store (bool &, boost::filesystem::path const &, int lmdb_max_dbs = 128);
	cga::mdb_env environment;
};
}
