#pragma once

#include <chrono>
#include <cga/lib/errors.hpp>
#include <cga/node/node.hpp>

namespace cga
{
/** Test-system related error codes */
enum class error_system
{
	generic = 1,
	deadline_expired
};
class system
{
public:
	system (uint16_t, uint16_t);
	~system ();
	void generate_activity (cga::node &, std::vector<cga::account> &);
	void generate_mass_activity (uint32_t, cga::node &);
	void generate_usage_traffic (uint32_t, uint32_t, size_t);
	void generate_usage_traffic (uint32_t, uint32_t);
	cga::account get_random_account (std::vector<cga::account> &);
	cga::uint128_t get_random_amount (cga::transaction const &, cga::node &, cga::account const &);
	void generate_rollback (cga::node &, std::vector<cga::account> &);
	void generate_change_known (cga::node &, std::vector<cga::account> &);
	void generate_change_unknown (cga::node &, std::vector<cga::account> &);
	void generate_receive (cga::node &);
	void generate_send_new (cga::node &, std::vector<cga::account> &);
	void generate_send_existing (cga::node &, std::vector<cga::account> &);
	std::shared_ptr<cga::wallet> wallet (size_t);
	cga::account account (cga::transaction const &, size_t);
	/**
	 * Polls, sleep if there's no work to be done (default 50ms), then check the deadline
	 * @returns 0 or cga::deadline_expired
	 */
	std::error_code poll (const std::chrono::nanoseconds & sleep_time = std::chrono::milliseconds (50));
	void stop ();
	void deadline_set (const std::chrono::duration<double, std::nano> & delta);
	boost::asio::io_context io_ctx;
	cga::alarm alarm;
	std::vector<std::shared_ptr<cga::node>> nodes;
	cga::logging logging;
	cga::work_pool work;
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> deadline{ std::chrono::steady_clock::time_point::max () };
	double deadline_scaling_factor{ 1.0 };
};
class landing_store
{
public:
	landing_store ();
	landing_store (cga::account const &, cga::account const &, uint64_t, uint64_t);
	landing_store (bool &, std::istream &);
	cga::account source;
	cga::account destination;
	uint64_t start;
	uint64_t last;
	void serialize (std::ostream &) const;
	bool deserialize (std::istream &);
	bool operator== (cga::landing_store const &) const;
};
class landing
{
public:
	landing (cga::node &, std::shared_ptr<cga::wallet>, cga::landing_store &, boost::filesystem::path const &);
	void write_store ();
	cga::uint128_t distribution_amount (uint64_t);
	void distribute_one ();
	void distribute_ongoing ();
	boost::filesystem::path path;
	cga::landing_store & store;
	std::shared_ptr<cga::wallet> wallet;
	cga::node & node;
	static int constexpr interval_exponent = 10;
	static std::chrono::seconds constexpr distribution_interval = std::chrono::seconds (1 << interval_exponent); // 1024 seconds
	static std::chrono::seconds constexpr sleep_seconds = std::chrono::seconds (7);
};
}
REGISTER_ERROR_CODES (cga, error_system);
