#pragma once

#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>
#include <cga/lib/config.hpp>
#include <cga/lib/numbers.hpp>
#include <cga/lib/utility.hpp>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>

namespace cga
{
class block;
bool work_validate (cga::block_hash const &, uint64_t, uint64_t * = nullptr);
bool work_validate (cga::block const &, uint64_t * = nullptr);
uint64_t work_value (cga::block_hash const &, uint64_t);
class opencl_work;
class work_item
{
public:
	cga::uint256_union item;
	std::function<void(boost::optional<uint64_t> const &)> callback;
	uint64_t difficulty;
};
class work_pool
{
public:
	work_pool (unsigned, std::function<boost::optional<uint64_t> (cga::uint256_union const &)> = nullptr);
	~work_pool ();
	void loop (uint64_t);
	void stop ();
	void cancel (cga::uint256_union const &);
	void generate (cga::uint256_union const &, std::function<void(boost::optional<uint64_t> const &)>, uint64_t = cga::work_pool::publish_threshold);
	uint64_t generate (cga::uint256_union const &, uint64_t = cga::work_pool::publish_threshold);
	std::atomic<int> ticket;
	bool done;
	std::vector<boost::thread> threads;
	std::list<cga::work_item> pending;
	std::mutex mutex;
	std::condition_variable producer_condition;
	std::function<boost::optional<uint64_t> (cga::uint256_union const &)> opencl;
	cga::observer_set<bool> work_observers;
	// Local work threshold for rate-limiting publishing blocks. ~5 seconds of work.
	static uint64_t const publish_test_threshold = 0xff00000000000000;
	static uint64_t const publish_full_threshold = 0xffffffc000000000;
	static uint64_t const publish_threshold = cga::is_test_network ? publish_test_threshold : publish_full_threshold;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (work_pool & work_pool, const std::string & name);
}
