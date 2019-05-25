#pragma once

#include <cga/node/common.hpp>
#include <cga/secure/blockstore.hpp>
#include <cga/secure/ledger.hpp>

#include <atomic>
#include <future>
#include <queue>
#include <stack>
#include <unordered_set>

#include <boost/log/sources/logger.hpp>
#include <boost/thread/thread.hpp>

namespace cga
{
class bootstrap_attempt;
class bootstrap_client;
class node;
enum class sync_result
{
	success,
	error,
	fork
};
class socket : public std::enable_shared_from_this<cga::socket>
{
public:
	socket (std::shared_ptr<cga::node>);
	void async_connect (cga::tcp_endpoint const &, std::function<void(boost::system::error_code const &)>);
	void async_read (std::shared_ptr<std::vector<uint8_t>>, size_t, std::function<void(boost::system::error_code const &, size_t)>);
	void async_write (std::shared_ptr<std::vector<uint8_t>>, std::function<void(boost::system::error_code const &, size_t)>);
	void start (std::chrono::steady_clock::time_point = std::chrono::steady_clock::now () + std::chrono::seconds (5));
	void stop ();
	void close ();
	void checkup ();
	cga::tcp_endpoint remote_endpoint ();
	boost::asio::ip::tcp::socket socket_m;

private:
	std::atomic<uint64_t> cutoff;
	std::shared_ptr<cga::node> node;
};

/**
 * The length of every message header, parsed by cga::message::read_header ()
 * The 2 here represents the size of a std::bitset<16>, which is 2 chars long normally
 */
static const int bootstrap_message_header_size = sizeof (cga::message_header::magic_number) + sizeof (uint8_t) + sizeof (uint8_t) + sizeof (uint8_t) + sizeof (cga::message_type) + 2;

class bootstrap_client;
class pull_info
{
public:
	typedef cga::bulk_pull::count_t count_t;
	pull_info ();
	pull_info (cga::account const &, cga::block_hash const &, cga::block_hash const &, count_t = 0);
	cga::account account;
	cga::block_hash head;
	cga::block_hash end;
	count_t count;
	unsigned attempts;
};
enum class bootstrap_mode
{
	legacy,
	lazy,
	wallet_lazy
};
class frontier_req_client;
class bulk_push_client;
class bulk_pull_account_client;
class bootstrap_attempt : public std::enable_shared_from_this<bootstrap_attempt>
{
public:
	bootstrap_attempt (std::shared_ptr<cga::node> node_a);
	~bootstrap_attempt ();
	void run ();
	std::shared_ptr<cga::bootstrap_client> connection (std::unique_lock<std::mutex> &);
	bool consume_future (std::future<bool> &);
	void populate_connections ();
	bool request_frontier (std::unique_lock<std::mutex> &);
	void request_pull (std::unique_lock<std::mutex> &);
	void request_push (std::unique_lock<std::mutex> &);
	void add_connection (cga::endpoint const &);
	void pool_connection (std::shared_ptr<cga::bootstrap_client>);
	void stop ();
	void requeue_pull (cga::pull_info const &);
	void add_pull (cga::pull_info const &);
	bool still_pulling ();
	unsigned target_connections (size_t pulls_remaining);
	bool should_log ();
	void add_bulk_push_target (cga::block_hash const &, cga::block_hash const &);
	bool process_block (std::shared_ptr<cga::block>, cga::account const &, uint64_t, bool);
	void lazy_run ();
	void lazy_start (cga::block_hash const &);
	void lazy_add (cga::block_hash const &);
	bool lazy_finished ();
	void lazy_pull_flush ();
	void lazy_clear ();
	void request_pending (std::unique_lock<std::mutex> &);
	void requeue_pending (cga::account const &);
	void wallet_run ();
	void wallet_start (std::deque<cga::account> &);
	bool wallet_finished ();
	std::chrono::steady_clock::time_point next_log;
	std::deque<std::weak_ptr<cga::bootstrap_client>> clients;
	std::weak_ptr<cga::bootstrap_client> connection_frontier_request;
	std::weak_ptr<cga::frontier_req_client> frontiers;
	std::weak_ptr<cga::bulk_push_client> push;
	std::deque<cga::pull_info> pulls;
	std::deque<std::shared_ptr<cga::bootstrap_client>> idle;
	std::atomic<unsigned> connections;
	std::atomic<unsigned> pulling;
	std::shared_ptr<cga::node> node;
	std::atomic<unsigned> account_count;
	std::atomic<uint64_t> total_blocks;
	std::atomic<unsigned> runs_count;
	std::vector<std::pair<cga::block_hash, cga::block_hash>> bulk_push_targets;
	std::atomic<bool> stopped;
	cga::bootstrap_mode mode;
	std::mutex mutex;
	std::condition_variable condition;
	// Lazy bootstrap
	std::unordered_set<cga::block_hash> lazy_blocks;
	std::unordered_map<cga::block_hash, std::pair<cga::block_hash, cga::uint128_t>> lazy_state_unknown;
	std::unordered_map<cga::block_hash, cga::uint128_t> lazy_balances;
	std::unordered_set<cga::block_hash> lazy_keys;
	std::deque<cga::block_hash> lazy_pulls;
	std::atomic<uint64_t> lazy_stopped;
	uint64_t lazy_max_pull_blocks = cga::is_test_network ? 2 : 512;
	uint64_t lazy_max_stopped = 256;
	std::mutex lazy_mutex;
	// Wallet lazy bootstrap
	std::deque<cga::account> wallet_accounts;
};
class frontier_req_client : public std::enable_shared_from_this<cga::frontier_req_client>
{
public:
	frontier_req_client (std::shared_ptr<cga::bootstrap_client>);
	~frontier_req_client ();
	void run ();
	void receive_frontier ();
	void received_frontier (boost::system::error_code const &, size_t);
	void unsynced (cga::block_hash const &, cga::block_hash const &);
	void next (cga::transaction const &);
	std::shared_ptr<cga::bootstrap_client> connection;
	cga::account current;
	cga::block_hash frontier;
	unsigned count;
	cga::account landing;
	cga::account faucet;
	std::chrono::steady_clock::time_point start_time;
	std::promise<bool> promise;
	/** A very rough estimate of the cost of `bulk_push`ing missing blocks */
	uint64_t bulk_push_cost;
	std::deque<std::pair<cga::account, cga::block_hash>> accounts;
	static size_t constexpr size_frontier = sizeof (cga::account) + sizeof (cga::block_hash);
};
class bulk_pull_client : public std::enable_shared_from_this<cga::bulk_pull_client>
{
public:
	bulk_pull_client (std::shared_ptr<cga::bootstrap_client>, cga::pull_info const &);
	~bulk_pull_client ();
	void request ();
	void receive_block ();
	void received_type ();
	void received_block (boost::system::error_code const &, size_t, cga::block_type);
	cga::block_hash first ();
	std::shared_ptr<cga::bootstrap_client> connection;
	cga::block_hash expected;
	cga::account known_account;
	cga::pull_info pull;
	uint64_t total_blocks;
	uint64_t unexpected_count;
};
class bootstrap_client : public std::enable_shared_from_this<bootstrap_client>
{
public:
	bootstrap_client (std::shared_ptr<cga::node>, std::shared_ptr<cga::bootstrap_attempt>, cga::tcp_endpoint const &);
	~bootstrap_client ();
	void run ();
	std::shared_ptr<cga::bootstrap_client> shared ();
	void stop (bool force);
	double block_rate () const;
	double elapsed_seconds () const;
	std::shared_ptr<cga::node> node;
	std::shared_ptr<cga::bootstrap_attempt> attempt;
	std::shared_ptr<cga::socket> socket;
	std::shared_ptr<std::vector<uint8_t>> receive_buffer;
	cga::tcp_endpoint endpoint;
	std::chrono::steady_clock::time_point start_time;
	std::atomic<uint64_t> block_count;
	std::atomic<bool> pending_stop;
	std::atomic<bool> hard_stop;
};
class bulk_push_client : public std::enable_shared_from_this<cga::bulk_push_client>
{
public:
	bulk_push_client (std::shared_ptr<cga::bootstrap_client> const &);
	~bulk_push_client ();
	void start ();
	void push (cga::transaction const &);
	void push_block (cga::block const &);
	void send_finished ();
	std::shared_ptr<cga::bootstrap_client> connection;
	std::promise<bool> promise;
	std::pair<cga::block_hash, cga::block_hash> current_target;
};
class bulk_pull_account_client : public std::enable_shared_from_this<cga::bulk_pull_account_client>
{
public:
	bulk_pull_account_client (std::shared_ptr<cga::bootstrap_client>, cga::account const &);
	~bulk_pull_account_client ();
	void request ();
	void receive_pending ();
	std::shared_ptr<cga::bootstrap_client> connection;
	cga::account account;
	uint64_t total_blocks;
};
class bootstrap_initiator
{
public:
	bootstrap_initiator (cga::node &);
	~bootstrap_initiator ();
	void bootstrap (cga::endpoint const &, bool add_to_peers = true);
	void bootstrap ();
	void bootstrap_lazy (cga::block_hash const &, bool = false);
	void bootstrap_wallet (std::deque<cga::account> &);
	void run_bootstrap ();
	void notify_listeners (bool);
	void add_observer (std::function<void(bool)> const &);
	bool in_progress ();
	std::shared_ptr<cga::bootstrap_attempt> current_attempt ();
	void stop ();

private:
	cga::node & node;
	std::shared_ptr<cga::bootstrap_attempt> attempt;
	bool stopped;
	std::mutex mutex;
	std::condition_variable condition;
	std::vector<std::function<void(bool)>> observers;
	boost::thread thread;

	friend std::unique_ptr<seq_con_info_component> collect_seq_con_info (bootstrap_initiator & bootstrap_initiator, const std::string & name);
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (bootstrap_initiator & bootstrap_initiator, const std::string & name);

class bootstrap_server;
class bootstrap_listener
{
public:
	bootstrap_listener (boost::asio::io_context &, uint16_t, cga::node &);
	void start ();
	void stop ();
	void accept_connection ();
	void accept_action (boost::system::error_code const &, std::shared_ptr<cga::socket>);
	std::mutex mutex;
	std::unordered_map<cga::bootstrap_server *, std::weak_ptr<cga::bootstrap_server>> connections;
	cga::tcp_endpoint endpoint ();
	boost::asio::ip::tcp::acceptor acceptor;
	cga::tcp_endpoint local;
	boost::asio::io_context & io_ctx;
	cga::node & node;
	bool on;

private:
	boost::asio::steady_timer defer_acceptor;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (bootstrap_listener & bootstrap_listener, const std::string & name);

class message;
class bootstrap_server : public std::enable_shared_from_this<cga::bootstrap_server>
{
public:
	bootstrap_server (std::shared_ptr<cga::socket>, std::shared_ptr<cga::node>);
	~bootstrap_server ();
	void receive ();
	void receive_header_action (boost::system::error_code const &, size_t);
	void receive_bulk_pull_action (boost::system::error_code const &, size_t, cga::message_header const &);
	void receive_bulk_pull_account_action (boost::system::error_code const &, size_t, cga::message_header const &);
	void receive_frontier_req_action (boost::system::error_code const &, size_t, cga::message_header const &);
	void receive_keepalive_action (boost::system::error_code const &, size_t, cga::message_header const &);
	void add_request (std::unique_ptr<cga::message>);
	void finish_request ();
	void run_next ();
	std::shared_ptr<std::vector<uint8_t>> receive_buffer;
	std::shared_ptr<cga::socket> socket;
	std::shared_ptr<cga::node> node;
	std::mutex mutex;
	std::queue<std::unique_ptr<cga::message>> requests;
};
class bulk_pull;
class bulk_pull_server : public std::enable_shared_from_this<cga::bulk_pull_server>
{
public:
	bulk_pull_server (std::shared_ptr<cga::bootstrap_server> const &, std::unique_ptr<cga::bulk_pull>);
	void set_current_end ();
	std::shared_ptr<cga::block> get_next ();
	void send_next ();
	void sent_action (boost::system::error_code const &, size_t);
	void send_finished ();
	void no_block_sent (boost::system::error_code const &, size_t);
	std::shared_ptr<cga::bootstrap_server> connection;
	std::unique_ptr<cga::bulk_pull> request;
	std::shared_ptr<std::vector<uint8_t>> send_buffer;
	cga::block_hash current;
	bool include_start;
	cga::bulk_pull::count_t max_count;
	cga::bulk_pull::count_t sent_count;
};
class bulk_pull_account;
class bulk_pull_account_server : public std::enable_shared_from_this<cga::bulk_pull_account_server>
{
public:
	bulk_pull_account_server (std::shared_ptr<cga::bootstrap_server> const &, std::unique_ptr<cga::bulk_pull_account>);
	void set_params ();
	std::pair<std::unique_ptr<cga::pending_key>, std::unique_ptr<cga::pending_info>> get_next ();
	void send_frontier ();
	void send_next_block ();
	void sent_action (boost::system::error_code const &, size_t);
	void send_finished ();
	void complete (boost::system::error_code const &, size_t);
	std::shared_ptr<cga::bootstrap_server> connection;
	std::unique_ptr<cga::bulk_pull_account> request;
	std::shared_ptr<std::vector<uint8_t>> send_buffer;
	std::unordered_set<cga::uint256_union> deduplication;
	cga::pending_key current_key;
	bool pending_address_only;
	bool pending_include_address;
	bool invalid_request;
};
class bulk_push_server : public std::enable_shared_from_this<cga::bulk_push_server>
{
public:
	bulk_push_server (std::shared_ptr<cga::bootstrap_server> const &);
	void receive ();
	void received_type ();
	void received_block (boost::system::error_code const &, size_t, cga::block_type);
	std::shared_ptr<std::vector<uint8_t>> receive_buffer;
	std::shared_ptr<cga::bootstrap_server> connection;
};
class frontier_req;
class frontier_req_server : public std::enable_shared_from_this<cga::frontier_req_server>
{
public:
	frontier_req_server (std::shared_ptr<cga::bootstrap_server> const &, std::unique_ptr<cga::frontier_req>);
	void send_next ();
	void sent_action (boost::system::error_code const &, size_t);
	void send_finished ();
	void no_block_sent (boost::system::error_code const &, size_t);
	void next ();
	std::shared_ptr<cga::bootstrap_server> connection;
	cga::account current;
	cga::block_hash frontier;
	std::unique_ptr<cga::frontier_req> request;
	std::shared_ptr<std::vector<uint8_t>> send_buffer;
	size_t count;
	std::deque<std::pair<cga::account, cga::block_hash>> accounts;
};
}
