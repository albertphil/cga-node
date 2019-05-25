#pragma once

#include <cga/secure/common.hpp>

namespace cga
{
class block_store;
class stat;

class shared_ptr_block_hash
{
public:
	size_t operator() (std::shared_ptr<cga::block> const &) const;
	bool operator() (std::shared_ptr<cga::block> const &, std::shared_ptr<cga::block> const &) const;
};
using tally_t = std::map<cga::uint128_t, std::shared_ptr<cga::block>, std::greater<cga::uint128_t>>;
class ledger
{
public:
	ledger (cga::block_store &, cga::stat &, cga::uint256_union const & = 1, cga::account const & = 0);
	cga::account account (cga::transaction const &, cga::block_hash const &);
	cga::uint128_t amount (cga::transaction const &, cga::block_hash const &);
	cga::uint128_t balance (cga::transaction const &, cga::block_hash const &);
	cga::uint128_t account_balance (cga::transaction const &, cga::account const &);
	cga::uint128_t account_pending (cga::transaction const &, cga::account const &);
	cga::uint128_t weight (cga::transaction const &, cga::account const &);
	std::shared_ptr<cga::block> successor (cga::transaction const &, cga::uint512_union const &);
	std::shared_ptr<cga::block> forked_block (cga::transaction const &, cga::block const &);
	cga::block_hash latest (cga::transaction const &, cga::account const &);
	cga::block_hash latest_root (cga::transaction const &, cga::account const &);
	cga::block_hash representative (cga::transaction const &, cga::block_hash const &);
	cga::block_hash representative_calculated (cga::transaction const &, cga::block_hash const &);
	bool block_exists (cga::block_hash const &);
	bool block_exists (cga::block_type, cga::block_hash const &);
	std::string block_text (char const *);
	std::string block_text (cga::block_hash const &);
	bool is_send (cga::transaction const &, cga::state_block const &);
	cga::block_hash block_destination (cga::transaction const &, cga::block const &);
	cga::block_hash block_source (cga::transaction const &, cga::block const &);
	cga::process_return process (cga::transaction const &, cga::block const &, cga::signature_verification = cga::signature_verification::unknown);
	void rollback (cga::transaction const &, cga::block_hash const &, std::vector<cga::block_hash> &);
	void rollback (cga::transaction const &, cga::block_hash const &);
	void change_latest (cga::transaction const &, cga::account const &, cga::block_hash const &, cga::account const &, cga::uint128_union const &, uint64_t, bool = false, cga::epoch = cga::epoch::epoch_0);
	void dump_account_chain (cga::account const &);
	bool could_fit (cga::transaction const &, cga::block const &);
	bool is_epoch_link (cga::uint256_union const &);
	static cga::uint128_t const unit;
	cga::block_store & store;
	cga::stat & stats;
	std::unordered_map<cga::account, cga::uint128_t> bootstrap_weights;
	uint64_t bootstrap_weight_max_blocks;
	std::atomic<bool> check_bootstrap_weights;
	cga::uint256_union epoch_link;
	cga::account epoch_signer;
};

std::unique_ptr<seq_con_info_component> collect_seq_con_info (ledger & ledger, const std::string & name);
}
