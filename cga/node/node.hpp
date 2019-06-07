#pragma once

#include <cga/lib/work.hpp>
#include <cga/node/blockprocessor.hpp>
#include <cga/node/bootstrap.hpp>
#include <cga/node/logging.hpp>
#include <cga/node/nodeconfig.hpp>
#include <cga/node/peers.hpp>
#include <cga/node/portmapping.hpp>
#include <cga/node/signatures.hpp>
#include <cga/node/stats.hpp>
#include <cga/node/wallet.hpp>
#include <cga/secure/ledger.hpp>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>

#include <boost/asio/thread_pool.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/thread/thread.hpp>

#define xstr(a) ver_str (a)
#define ver_str(a) #a

/**
* Returns build version information
*/
static const char * CGA_MAJOR_MINOR_VERSION = xstr (CGA_VERSION_MAJOR) "." xstr (CGA_VERSION_MINOR);
static const char * CGA_MAJOR_MINOR_RC_VERSION = xstr (CGA_VERSION_MAJOR) "." xstr (CGA_VERSION_MINOR) "RC" xstr (CGA_VERSION_PATCH);

namespace cga
{
class node;
class election_status
{
public:
	std::shared_ptr<cga::block> winner;
	cga::amount tally;
	std::chrono::milliseconds election_end;
	std::chrono::milliseconds election_duration;
};
class vote_info
{
public:
	std::chrono::steady_clock::time_point time;
	uint64_t sequence;
	cga::block_hash hash;
};
class election_vote_result
{
public:
	election_vote_result ();
	election_vote_result (bool, bool);
	bool replay;
	bool processed;
};
class election : public std::enable_shared_from_this<cga::election>
{
	std::function<void(std::shared_ptr<cga::block>)> confirmation_action;
	void confirm_once (cga::transaction const &, bool = false);
	void confirm_back (cga::transaction const &);

public:
	election (cga::node &, std::shared_ptr<cga::block>, std::function<void(std::shared_ptr<cga::block>)> const &);
	cga::election_vote_result vote (cga::account, uint64_t, cga::block_hash);
	cga::tally_t tally (cga::transaction const &);
	// Check if we have vote quorum
	bool have_quorum (cga::tally_t const &, cga::uint128_t);
	// Change our winner to agree with the network
	void compute_rep_votes (cga::transaction const &);
	// Confirm this block if quorum is met
	void confirm_if_quorum (cga::transaction const &);
	void log_votes (cga::tally_t const &);
	bool publish (std::shared_ptr<cga::block> block_a);
	size_t last_votes_size ();
	void stop ();
	cga::node & node;
	std::unordered_map<cga::account, cga::vote_info> last_votes;
	std::unordered_map<cga::block_hash, std::shared_ptr<cga::block>> blocks;
	std::chrono::steady_clock::time_point election_start;
	cga::election_status status;
	std::atomic<bool> confirmed;
	bool stopped;
	std::unordered_map<cga::block_hash, cga::uint128_t> last_tally;
	unsigned announcements;
};
class conflict_info
{
public:
	cga::uint512_union root;
	uint64_t difficulty;
	std::shared_ptr<cga::election> election;
};
// Core class for determining consensus
// Holds all active blocks i.e. recently added blocks that need confirmation
class active_transactions
{
public:
	active_transactions (cga::node &);
	~active_transactions ();
	// Start an election for a block
	// Call action with confirmed block, may be different than what we started with
	// clang-format off
	bool start (std::shared_ptr<cga::block>, std::function<void(std::shared_ptr<cga::block>)> const & = [](std::shared_ptr<cga::block>) {});
	// clang-format on
	// If this returns true, the vote is a replay
	// If this returns false, the vote may or may not be a replay
	bool vote (std::shared_ptr<cga::vote>, bool = false);
	// Is the root of this block in the roots container
	bool active (cga::block const &);
	void update_difficulty (cga::block const &);
	std::deque<std::shared_ptr<cga::block>> list_blocks (bool = false);
	void erase (cga::block const &);
	bool empty ();
	size_t size ();
	void stop ();
	bool publish (std::shared_ptr<cga::block> block_a);
	boost::multi_index_container<
	cga::conflict_info,
	boost::multi_index::indexed_by<
	boost::multi_index::hashed_unique<
	boost::multi_index::member<cga::conflict_info, cga::uint512_union, &cga::conflict_info::root>>,
	boost::multi_index::ordered_non_unique<
	boost::multi_index::member<cga::conflict_info, uint64_t, &cga::conflict_info::difficulty>,
	std::greater<uint64_t>>>>
	roots;
	std::unordered_map<cga::block_hash, std::shared_ptr<cga::election>> blocks;
	std::deque<cga::election_status> list_confirmed ();
	std::deque<cga::election_status> confirmed;
	cga::node & node;
	std::mutex mutex;
	// Maximum number of conflicts to vote on per interval, lowest root hash first
	static unsigned constexpr announcements_per_interval = 32;
	// Minimum number of block announcements
	static unsigned constexpr announcement_min = 2;
	// Threshold to start logging blocks haven't yet been confirmed
	static unsigned constexpr announcement_long = 20;
	static unsigned constexpr request_interval_ms = cga::is_test_network ? 10 : 16000;
	static size_t constexpr election_history_size = 2048;
	static size_t constexpr max_broadcast_queue = 1000;

private:
	// Call action with confirmed block, may be different than what we started with
	// clang-format off
	bool add (std::shared_ptr<cga::block>, std::function<void(std::shared_ptr<cga::block>)> const & = [](std::shared_ptr<cga::block>) {});
	// clang-format on
	void request_loop ();
	void request_confirm (std::unique_lock<std::mutex> &);
	std::condition_variable condition;
	bool started;
	bool stopped;
	boost::thread thread;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (active_transactions & active_transactions, const std::string & name);

class operation
{
public:
	bool operator> (cga::operation const &) const;
	std::chrono::steady_clock::time_point wakeup;
	std::function<void()> function;
};
class alarm
{
public:
	alarm (boost::asio::io_context &);
	~alarm ();
	void add (std::chrono::steady_clock::time_point const &, std::function<void()> const &);
	void run ();
	boost::asio::io_context & io_ctx;
	std::mutex mutex;
	std::condition_variable condition;
	std::priority_queue<operation, std::vector<operation>, std::greater<operation>> operations;
	boost::thread thread;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (alarm & alarm, const std::string & name);

class gap_information
{
public:
	std::chrono::steady_clock::time_point arrival;
	cga::block_hash hash;
	std::unordered_set<cga::account> voters;
};
class gap_cache
{
public:
	gap_cache (cga::node &);
	void add (cga::transaction const &, cga::block_hash const &, std::chrono::steady_clock::time_point = std::chrono::steady_clock::now ());
	void vote (std::shared_ptr<cga::vote>);
	cga::uint128_t bootstrap_threshold (cga::transaction const &);
	size_t size ();
	boost::multi_index_container<
	cga::gap_information,
	boost::multi_index::indexed_by<
	boost::multi_index::ordered_non_unique<boost::multi_index::member<gap_information, std::chrono::steady_clock::time_point, &gap_information::arrival>>,
	boost::multi_index::hashed_unique<boost::multi_index::member<gap_information, cga::block_hash, &gap_information::hash>>>>
	blocks;
	size_t const max = 256;
	std::mutex mutex;
	cga::node & node;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (gap_cache & gap_cache, const std::string & name);

class work_pool;
class send_info
{
public:
	uint8_t const * data;
	size_t size;
	cga::endpoint endpoint;
	std::function<void(boost::system::error_code const &, size_t)> callback;
};
class block_arrival_info
{
public:
	std::chrono::steady_clock::time_point arrival;
	cga::block_hash hash;
};
// This class tracks blocks that are probably live because they arrived in a UDP packet
// This gives a fairly reliable way to differentiate between blocks being inserted via bootstrap or new, live blocks.
class block_arrival
{
public:
	// Return `true' to indicated an error if the block has already been inserted
	bool add (cga::block_hash const &);
	bool recent (cga::block_hash const &);
	boost::multi_index_container<
	cga::block_arrival_info,
	boost::multi_index::indexed_by<
	boost::multi_index::ordered_non_unique<boost::multi_index::member<cga::block_arrival_info, std::chrono::steady_clock::time_point, &cga::block_arrival_info::arrival>>,
	boost::multi_index::hashed_unique<boost::multi_index::member<cga::block_arrival_info, cga::block_hash, &cga::block_arrival_info::hash>>>>
	arrival;
	std::mutex mutex;
	static size_t constexpr arrival_size_min = 8 * 1024;
	static std::chrono::seconds constexpr arrival_time_min = std::chrono::seconds (300);
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (block_arrival & block_arrival, const std::string & name);

class online_reps
{
public:
	online_reps (cga::ledger &, cga::uint128_t);
	void observe (cga::account const &);
	void sample ();
	cga::uint128_t online_stake ();
	std::vector<cga::account> list ();
	static uint64_t constexpr weight_period = 5 * 60; // 5 minutes
	// The maximum amount of samples for a 2 week period on live or 3 days on beta
	static uint64_t constexpr weight_samples = cga::is_live_network ? 4032 : 864;

private:
	cga::uint128_t trend (cga::transaction &);
	std::mutex mutex;
	cga::ledger & ledger;
	std::unordered_set<cga::account> reps;
	cga::uint128_t online;
	cga::uint128_t minimum;

	friend std::unique_ptr<seq_con_info_component> collect_seq_con_info (online_reps & online_reps, const std::string & name);
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (online_reps & online_reps, const std::string & name);

class udp_data
{
public:
	uint8_t * buffer;
	size_t size;
	cga::endpoint endpoint;
};
/**
  * A circular buffer for servicing UDP datagrams. This container follows a producer/consumer model where the operating system is producing data in to buffers which are serviced by internal threads.
  * If buffers are not serviced fast enough they're internally dropped.
  * This container has a maximum space to hold N buffers of M size and will allocate them in round-robin order.
  * All public methods are thread-safe
*/
class udp_buffer
{
public:
	// Stats - Statistics
	// Size - Size of each individual buffer
	// Count - Number of buffers to allocate
	udp_buffer (cga::stat & stats, size_t, size_t);
	// Return a buffer where UDP data can be put
	// Method will attempt to return the first free buffer
	// If there are no free buffers, an unserviced buffer will be dequeued and returned
	// Function will block if there are no free or unserviced buffers
	// Return nullptr if the container has stopped
	cga::udp_data * allocate ();
	// Queue a buffer that has been filled with UDP data and notify servicing threads
	void enqueue (cga::udp_data *);
	// Return a buffer that has been filled with UDP data
	// Function will block until a buffer has been added
	// Return nullptr if the container has stopped
	cga::udp_data * dequeue ();
	// Return a buffer to the freelist after is has been serviced
	void release (cga::udp_data *);
	// Stop container and notify waiting threads
	void stop ();

private:
	cga::stat & stats;
	std::mutex mutex;
	std::condition_variable condition;
	boost::circular_buffer<cga::udp_data *> free;
	boost::circular_buffer<cga::udp_data *> full;
	std::vector<uint8_t> slab;
	std::vector<cga::udp_data> entries;
	bool stopped;
};
class network
{
public:
	network (cga::node &, uint16_t);
	~network ();
	void receive ();
	void process_packets ();
	void start ();
	void stop ();
	void receive_action (cga::udp_data *, cga::endpoint const &);
	void rpc_action (boost::system::error_code const &, size_t);
	void republish_vote (std::shared_ptr<cga::vote>);
	void republish_block (std::shared_ptr<cga::block>);
	void republish_block (std::shared_ptr<cga::block>, cga::endpoint const &);
	static unsigned const broadcast_interval_ms = 10;
	void republish_block_batch (std::deque<std::shared_ptr<cga::block>>, unsigned = broadcast_interval_ms);
	void republish (cga::block_hash const &, std::shared_ptr<std::vector<uint8_t>>, cga::endpoint);
	void confirm_send (cga::confirm_ack const &, std::shared_ptr<std::vector<uint8_t>>, cga::endpoint const &);
	void merge_peers (std::array<cga::endpoint, 8> const &);
	void send_keepalive (cga::endpoint const &);
	void send_node_id_handshake (cga::endpoint const &, boost::optional<cga::uint256_union> const & query, boost::optional<cga::uint256_union> const & respond_to);
	void broadcast_confirm_req (std::shared_ptr<cga::block>);
	void broadcast_confirm_req_base (std::shared_ptr<cga::block>, std::shared_ptr<std::vector<cga::peer_information>>, unsigned, bool = false);
	void broadcast_confirm_req_batch (std::unordered_map<cga::endpoint, std::vector<std::pair<cga::block_hash, cga::block_hash>>>, unsigned = broadcast_interval_ms, bool = false);
	void broadcast_confirm_req_batch (std::deque<std::pair<std::shared_ptr<cga::block>, std::shared_ptr<std::vector<cga::peer_information>>>>, unsigned = broadcast_interval_ms);
	void send_confirm_req (cga::endpoint const &, std::shared_ptr<cga::block>);
	void send_confirm_req_hashes (cga::endpoint const &, std::vector<std::pair<cga::block_hash, cga::block_hash>> const &);
	void confirm_hashes (cga::transaction const &, cga::endpoint const &, std::vector<cga::block_hash>);
	bool send_votes_cache (cga::block_hash const &, cga::endpoint const &);
	void send_buffer (uint8_t const *, size_t, cga::endpoint const &, std::function<void(boost::system::error_code const &, size_t)>);
	cga::endpoint endpoint ();
	cga::udp_buffer buffer_container;
	boost::asio::ip::udp::socket socket;
	std::mutex socket_mutex;
	boost::asio::ip::udp::resolver resolver;
	std::vector<boost::thread> packet_processing_threads;
	cga::node & node;
	std::atomic<bool> on;
	static uint16_t const node_port = cga::is_live_network ? 7032 : 54000;
	static size_t const buffer_size = 512;
	static size_t const confirm_req_hashes_max = 6;
};

class node_init
{
public:
	node_init ();
	bool error ();
	bool block_store_init;
	bool wallets_store_init;
	bool wallet_init;
};
class node_observers
{
public:
	cga::observer_set<std::shared_ptr<cga::block>, cga::account const &, cga::uint128_t const &, bool> blocks;
	cga::observer_set<bool> wallet;
	cga::observer_set<cga::transaction const &, std::shared_ptr<cga::vote>, cga::endpoint const &> vote;
	cga::observer_set<cga::account const &, bool> account_balance;
	cga::observer_set<cga::endpoint const &> endpoint;
	cga::observer_set<> disconnect;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (node_observers & node_observers, const std::string & name);

class vote_processor
{
public:
	vote_processor (cga::node &);
	void vote (std::shared_ptr<cga::vote>, cga::endpoint);
	// node.active.mutex lock required
	cga::vote_code vote_blocking (cga::transaction const &, std::shared_ptr<cga::vote>, cga::endpoint, bool = false);
	void verify_votes (std::deque<std::pair<std::shared_ptr<cga::vote>, cga::endpoint>> &);
	void flush ();
	void calculate_weights ();
	cga::node & node;
	void stop ();

private:
	void process_loop ();
	std::deque<std::pair<std::shared_ptr<cga::vote>, cga::endpoint>> votes;
	// Representatives levels for random early detection
	std::unordered_set<cga::account> representatives_1;
	std::unordered_set<cga::account> representatives_2;
	std::unordered_set<cga::account> representatives_3;
	std::condition_variable condition;
	std::mutex mutex;
	bool started;
	bool stopped;
	bool active;
	boost::thread thread;

	friend std::unique_ptr<seq_con_info_component> collect_seq_con_info (vote_processor & vote_processor, const std::string & name);
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (vote_processor & vote_processor, const std::string & name);

// The network is crawled for representatives by occasionally sending a unicast confirm_req for a specific block and watching to see if it's acknowledged with a vote.
class rep_crawler
{
public:
	void add (cga::block_hash const &);
	void remove (cga::block_hash const &);
	bool exists (cga::block_hash const &);
	std::mutex mutex;
	std::unordered_set<cga::block_hash> active;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (rep_crawler & rep_crawler, const std::string & name);
std::unique_ptr<seq_con_info_component> collect_seq_con_info (block_processor & block_processor, const std::string & name);

class node : public std::enable_shared_from_this<cga::node>
{
public:
	node (cga::node_init &, boost::asio::io_context &, uint16_t, boost::filesystem::path const &, cga::alarm &, cga::logging const &, cga::work_pool &);
	node (cga::node_init &, boost::asio::io_context &, boost::filesystem::path const &, cga::alarm &, cga::node_config const &, cga::work_pool &, cga::node_flags = cga::node_flags ());
	~node ();
	template <typename T>
	void background (T action_a)
	{
		alarm.io_ctx.post (action_a);
	}
	void send_keepalive (cga::endpoint const &);
	bool copy_with_compaction (boost::filesystem::path const &);
	void keepalive (std::string const &, uint16_t, bool = false);
	void start ();
	void stop ();
	std::shared_ptr<cga::node> shared ();
	int store_version ();
	void process_confirmed (std::shared_ptr<cga::block>, uint8_t = 0);
	void process_message (cga::message &, cga::endpoint const &);
	void process_active (std::shared_ptr<cga::block>);
	cga::process_return process (cga::block const &);
	void keepalive_preconfigured (std::vector<std::string> const &);
	cga::block_hash latest (cga::account const &);
	cga::uint128_t balance (cga::account const &);
	std::shared_ptr<cga::block> block (cga::block_hash const &);
	std::pair<cga::uint128_t, cga::uint128_t> balance_pending (cga::account const &);
	cga::uint128_t weight (cga::account const &);
	cga::account representative (cga::account const &);
	void ongoing_keepalive ();
	void ongoing_syn_cookie_cleanup ();
	void ongoing_rep_crawl ();
	void ongoing_rep_calculation ();
	void ongoing_bootstrap ();
	void ongoing_store_flush ();
	void ongoing_peer_store ();
	void ongoing_unchecked_cleanup ();
	void backup_wallet ();
	void search_pending ();
	void bootstrap_wallet ();
	void unchecked_cleanup ();
	int price (cga::uint128_t const &, int);
	void work_generate_blocking (cga::block &, uint64_t = cga::work_pool::publish_threshold);
	uint64_t work_generate_blocking (cga::uint256_union const &, uint64_t = cga::work_pool::publish_threshold);
	void work_generate (cga::uint256_union const &, std::function<void(uint64_t)>, uint64_t = cga::work_pool::publish_threshold);
	void add_initial_peers ();
	void block_confirm (std::shared_ptr<cga::block>);
	void process_fork (cga::transaction const &, std::shared_ptr<cga::block>);
	bool validate_block_by_previous (cga::transaction const &, std::shared_ptr<cga::block>);
	void do_rpc_callback (boost::asio::ip::tcp::resolver::iterator i_a, std::string const &, uint16_t, std::shared_ptr<std::string>, std::shared_ptr<std::string>, std::shared_ptr<boost::asio::ip::tcp::resolver>);
	cga::uint128_t delta ();
	void ongoing_online_weight_calculation ();
	void ongoing_online_weight_calculation_queue ();
	boost::asio::io_context & io_ctx;
	cga::node_config config;
	cga::node_flags flags;
	cga::alarm & alarm;
	cga::work_pool & work;
	boost::log::sources::logger_mt log;
	std::unique_ptr<cga::block_store> store_impl;
	cga::block_store & store;
	std::unique_ptr<cga::wallets_store> wallets_store_impl;
	cga::wallets_store & wallets_store;
	cga::gap_cache gap_cache;
	cga::ledger ledger;
	cga::active_transactions active;
	cga::network network;
	cga::bootstrap_initiator bootstrap_initiator;
	cga::bootstrap_listener bootstrap;
	cga::peer_container peers;
	boost::filesystem::path application_path;
	cga::node_observers observers;
	cga::wallets wallets;
	cga::port_mapping port_mapping;
	cga::signature_checker checker;
	cga::vote_processor vote_processor;
	cga::rep_crawler rep_crawler;
	unsigned warmed_up;
	cga::block_processor block_processor;
	boost::thread block_processor_thread;
	cga::block_arrival block_arrival;
	cga::online_reps online_reps;
	cga::votes_cache votes_cache;
	cga::stat stats;
	cga::keypair node_id;
	cga::block_uniquer block_uniquer;
	cga::vote_uniquer vote_uniquer;
	const std::chrono::steady_clock::time_point startup_time;
	static double constexpr price_max = 16.0;
	static double constexpr free_cutoff = 1024.0;
	static std::chrono::seconds constexpr period = cga::is_test_network ? std::chrono::seconds (1) : std::chrono::seconds (60);
	static std::chrono::seconds constexpr cutoff = period * 5;
	static std::chrono::seconds constexpr syn_cookie_cutoff = std::chrono::seconds (5);
	static std::chrono::minutes constexpr backup_interval = std::chrono::minutes (5);
	static std::chrono::seconds constexpr search_pending_interval = cga::is_test_network ? std::chrono::seconds (1) : std::chrono::seconds (5 * 60);
	static std::chrono::seconds constexpr peer_interval = search_pending_interval;
	static std::chrono::hours constexpr unchecked_cleanup_interval = std::chrono::hours (1);
	static std::chrono::milliseconds constexpr process_confirmed_interval = cga::is_test_network ? std::chrono::milliseconds (50) : std::chrono::milliseconds (500);
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (node & node, const std::string & name);

class thread_runner
{
public:
	thread_runner (boost::asio::io_context &, unsigned);
	~thread_runner ();
	void join ();
	std::vector<boost::thread> threads;
};
class inactive_node
{
public:
	inactive_node (boost::filesystem::path const & path = cga::working_path (), uint16_t = 24000);
	~inactive_node ();
	boost::filesystem::path path;
	std::shared_ptr<boost::asio::io_context> io_context;
	cga::alarm alarm;
	cga::logging logging;
	cga::node_init init;
	cga::work_pool work;
	uint16_t peering_port;
	std::shared_ptr<cga::node> node;
};
}
