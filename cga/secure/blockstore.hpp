#pragma once

#include <cga/secure/common.hpp>
#include <stack>

namespace cga
{
class block_sideband
{
public:
	block_sideband () = default;
	block_sideband (cga::block_type, cga::account const &, cga::block_hash const &, cga::amount const &, uint64_t, uint64_t);
	void serialize (cga::stream &) const;
	bool deserialize (cga::stream &);
	static size_t size (cga::block_type);
	cga::block_type type;
	cga::block_hash successor;
	cga::account account;
	cga::amount balance;
	uint64_t height;
	uint64_t timestamp;
};
class transaction;
class block_store;

/**
 * Summation visitor for blocks, supporting amount and balance computations. These
 * computations are mutually dependant. The natural solution is to use mutual recursion
 * between balance and amount visitors, but this leads to very deep stacks. Hence, the
 * summation visitor uses an iterative approach.
 */
class summation_visitor : public cga::block_visitor
{
	enum summation_type
	{
		invalid = 0,
		balance = 1,
		amount = 2
	};

	/** Represents an invocation frame */
	class frame
	{
	public:
		frame (summation_type type_a, cga::block_hash balance_hash_a, cga::block_hash amount_hash_a) :
		type (type_a), balance_hash (balance_hash_a), amount_hash (amount_hash_a)
		{
		}

		/** The summation type guides the block visitor handlers */
		summation_type type{ invalid };
		/** Accumulated balance or amount */
		cga::uint128_t sum{ 0 };
		/** The current balance hash */
		cga::block_hash balance_hash{ 0 };
		/** The current amount hash */
		cga::block_hash amount_hash{ 0 };
		/** If true, this frame is awaiting an invocation result */
		bool awaiting_result{ false };
		/** Set by the invoked frame, representing the return value */
		cga::uint128_t incoming_result{ 0 };
	};

public:
	summation_visitor (cga::transaction const &, cga::block_store &);
	virtual ~summation_visitor () = default;
	/** Computes the balance as of \p block_hash */
	cga::uint128_t compute_balance (cga::block_hash const & block_hash);
	/** Computes the amount delta between \p block_hash and its predecessor */
	cga::uint128_t compute_amount (cga::block_hash const & block_hash);

protected:
	cga::transaction const & transaction;
	cga::block_store & store;

	/** The final result */
	cga::uint128_t result{ 0 };
	/** The current invocation frame */
	frame * current{ nullptr };
	/** Invocation frames */
	std::stack<frame> frames;
	/** Push a copy of \p hash of the given summation \p type */
	cga::summation_visitor::frame push (cga::summation_visitor::summation_type type, cga::block_hash const & hash);
	void sum_add (cga::uint128_t addend_a);
	void sum_set (cga::uint128_t value_a);
	/** The epilogue yields the result to previous frame, if any */
	void epilogue ();

	cga::uint128_t compute_internal (cga::summation_visitor::summation_type type, cga::block_hash const &);
	void send_block (cga::send_block const &) override;
	void receive_block (cga::receive_block const &) override;
	void open_block (cga::open_block const &) override;
	void change_block (cga::change_block const &) override;
	void state_block (cga::state_block const &) override;
};

/**
 * Determine the representative for this block
 */
class representative_visitor : public cga::block_visitor
{
public:
	representative_visitor (cga::transaction const & transaction_a, cga::block_store & store_a);
	virtual ~representative_visitor () = default;
	void compute (cga::block_hash const & hash_a);
	void send_block (cga::send_block const & block_a) override;
	void receive_block (cga::receive_block const & block_a) override;
	void open_block (cga::open_block const & block_a) override;
	void change_block (cga::change_block const & block_a) override;
	void state_block (cga::state_block const & block_a) override;
	cga::transaction const & transaction;
	cga::block_store & store;
	cga::block_hash current;
	cga::block_hash result;
};
template <typename T, typename U>
class store_iterator_impl
{
public:
	virtual ~store_iterator_impl () = default;
	virtual cga::store_iterator_impl<T, U> & operator++ () = 0;
	virtual bool operator== (cga::store_iterator_impl<T, U> const & other_a) const = 0;
	virtual bool is_end_sentinal () const = 0;
	virtual void fill (std::pair<T, U> &) const = 0;
	cga::store_iterator_impl<T, U> & operator= (cga::store_iterator_impl<T, U> const &) = delete;
	bool operator== (cga::store_iterator_impl<T, U> const * other_a) const
	{
		return (other_a != nullptr && *this == *other_a) || (other_a == nullptr && is_end_sentinal ());
	}
	bool operator!= (cga::store_iterator_impl<T, U> const & other_a) const
	{
		return !(*this == other_a);
	}
};
/**
 * Iterates the key/value pairs of a transaction
 */
template <typename T, typename U>
class store_iterator
{
public:
	store_iterator (std::nullptr_t)
	{
	}
	store_iterator (std::unique_ptr<cga::store_iterator_impl<T, U>> impl_a) :
	impl (std::move (impl_a))
	{
		impl->fill (current);
	}
	store_iterator (cga::store_iterator<T, U> && other_a) :
	current (std::move (other_a.current)),
	impl (std::move (other_a.impl))
	{
	}
	cga::store_iterator<T, U> & operator++ ()
	{
		++*impl;
		impl->fill (current);
		return *this;
	}
	cga::store_iterator<T, U> & operator= (cga::store_iterator<T, U> && other_a)
	{
		impl = std::move (other_a.impl);
		current = std::move (other_a.current);
		return *this;
	}
	cga::store_iterator<T, U> & operator= (cga::store_iterator<T, U> const &) = delete;
	std::pair<T, U> * operator-> ()
	{
		return &current;
	}
	bool operator== (cga::store_iterator<T, U> const & other_a) const
	{
		return (impl == nullptr && other_a.impl == nullptr) || (impl != nullptr && *impl == other_a.impl.get ()) || (other_a.impl != nullptr && *other_a.impl == impl.get ());
	}
	bool operator!= (cga::store_iterator<T, U> const & other_a) const
	{
		return !(*this == other_a);
	}

private:
	std::pair<T, U> current;
	std::unique_ptr<cga::store_iterator_impl<T, U>> impl;
};

class block_predecessor_set;

class transaction_impl
{
public:
	virtual ~transaction_impl () = default;
};
/**
 * RAII wrapper of MDB_txn where the constructor starts the transaction
 * and the destructor commits it.
 */
class transaction
{
public:
	std::unique_ptr<cga::transaction_impl> impl;
};

/**
 * Manages block storage and iteration
 */
class block_store
{
public:
	virtual ~block_store () = default;
	virtual void initialize (cga::transaction const &, cga::genesis const &) = 0;
	virtual void block_put (cga::transaction const &, cga::block_hash const &, cga::block const &, cga::block_sideband const &, cga::epoch version = cga::epoch::epoch_0) = 0;
	virtual cga::block_hash block_successor (cga::transaction const &, cga::block_hash const &) = 0;
	virtual void block_successor_clear (cga::transaction const &, cga::block_hash const &) = 0;
	virtual std::shared_ptr<cga::block> block_get (cga::transaction const &, cga::block_hash const &, cga::block_sideband * = nullptr) = 0;
	virtual std::shared_ptr<cga::block> block_random (cga::transaction const &) = 0;
	virtual void block_del (cga::transaction const &, cga::block_hash const &) = 0;
	virtual bool block_exists (cga::transaction const &, cga::block_hash const &) = 0;
	virtual bool block_exists (cga::transaction const &, cga::block_type, cga::block_hash const &) = 0;
	virtual cga::block_counts block_count (cga::transaction const &) = 0;
	virtual bool root_exists (cga::transaction const &, cga::uint256_union const &) = 0;
	virtual bool source_exists (cga::transaction const &, cga::block_hash const &) = 0;
	virtual cga::account block_account (cga::transaction const &, cga::block_hash const &) = 0;

	virtual void frontier_put (cga::transaction const &, cga::block_hash const &, cga::account const &) = 0;
	virtual cga::account frontier_get (cga::transaction const &, cga::block_hash const &) = 0;
	virtual void frontier_del (cga::transaction const &, cga::block_hash const &) = 0;

	virtual void account_put (cga::transaction const &, cga::account const &, cga::account_info const &) = 0;
	virtual bool account_get (cga::transaction const &, cga::account const &, cga::account_info &) = 0;
	virtual void account_del (cga::transaction const &, cga::account const &) = 0;
	virtual bool account_exists (cga::transaction const &, cga::account const &) = 0;
	virtual size_t account_count (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_v0_begin (cga::transaction const &, cga::account const &) = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_v0_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_v0_end () = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_v1_begin (cga::transaction const &, cga::account const &) = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_v1_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_v1_end () = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_begin (cga::transaction const &, cga::account const &) = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::account, cga::account_info> latest_end () = 0;

	virtual void pending_put (cga::transaction const &, cga::pending_key const &, cga::pending_info const &) = 0;
	virtual void pending_del (cga::transaction const &, cga::pending_key const &) = 0;
	virtual bool pending_get (cga::transaction const &, cga::pending_key const &, cga::pending_info &) = 0;
	virtual bool pending_exists (cga::transaction const &, cga::pending_key const &) = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_v0_begin (cga::transaction const &, cga::pending_key const &) = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_v0_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_v0_end () = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_v1_begin (cga::transaction const &, cga::pending_key const &) = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_v1_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_v1_end () = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_begin (cga::transaction const &, cga::pending_key const &) = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::pending_key, cga::pending_info> pending_end () = 0;

	virtual bool block_info_get (cga::transaction const &, cga::block_hash const &, cga::block_info &) = 0;
	virtual cga::uint128_t block_balance (cga::transaction const &, cga::block_hash const &) = 0;
	virtual cga::epoch block_version (cga::transaction const &, cga::block_hash const &) = 0;

	virtual cga::uint128_t representation_get (cga::transaction const &, cga::account const &) = 0;
	virtual void representation_put (cga::transaction const &, cga::account const &, cga::uint128_t const &) = 0;
	virtual void representation_add (cga::transaction const &, cga::account const &, cga::uint128_t const &) = 0;
	virtual cga::store_iterator<cga::account, cga::uint128_union> representation_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::account, cga::uint128_union> representation_end () = 0;

	virtual void unchecked_clear (cga::transaction const &) = 0;
	virtual void unchecked_put (cga::transaction const &, cga::unchecked_key const &, cga::unchecked_info const &) = 0;
	virtual void unchecked_put (cga::transaction const &, cga::block_hash const &, std::shared_ptr<cga::block> const &) = 0;
	virtual std::vector<cga::unchecked_info> unchecked_get (cga::transaction const &, cga::block_hash const &) = 0;
	virtual bool unchecked_exists (cga::transaction const &, cga::unchecked_key const &) = 0;
	virtual void unchecked_del (cga::transaction const &, cga::unchecked_key const &) = 0;
	virtual cga::store_iterator<cga::unchecked_key, cga::unchecked_info> unchecked_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::unchecked_key, cga::unchecked_info> unchecked_begin (cga::transaction const &, cga::unchecked_key const &) = 0;
	virtual cga::store_iterator<cga::unchecked_key, cga::unchecked_info> unchecked_end () = 0;
	virtual size_t unchecked_count (cga::transaction const &) = 0;

	// Return latest vote for an account from store
	virtual std::shared_ptr<cga::vote> vote_get (cga::transaction const &, cga::account const &) = 0;
	// Populate vote with the next sequence number
	virtual std::shared_ptr<cga::vote> vote_generate (cga::transaction const &, cga::account const &, cga::raw_key const &, std::shared_ptr<cga::block>) = 0;
	virtual std::shared_ptr<cga::vote> vote_generate (cga::transaction const &, cga::account const &, cga::raw_key const &, std::vector<cga::block_hash>) = 0;
	// Return either vote or the stored vote with a higher sequence number
	virtual std::shared_ptr<cga::vote> vote_max (cga::transaction const &, std::shared_ptr<cga::vote>) = 0;
	// Return latest vote for an account considering the vote cache
	virtual std::shared_ptr<cga::vote> vote_current (cga::transaction const &, cga::account const &) = 0;
	virtual void flush (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::account, std::shared_ptr<cga::vote>> vote_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<cga::account, std::shared_ptr<cga::vote>> vote_end () = 0;

	virtual void online_weight_put (cga::transaction const &, uint64_t, cga::amount const &) = 0;
	virtual void online_weight_del (cga::transaction const &, uint64_t) = 0;
	virtual cga::store_iterator<uint64_t, cga::amount> online_weight_begin (cga::transaction const &) = 0;
	virtual cga::store_iterator<uint64_t, cga::amount> online_weight_end () = 0;
	virtual size_t online_weight_count (cga::transaction const &) const = 0;
	virtual void online_weight_clear (cga::transaction const &) = 0;

	virtual void version_put (cga::transaction const &, int) = 0;
	virtual int version_get (cga::transaction const &) = 0;

	virtual void peer_put (cga::transaction const & transaction_a, cga::endpoint_key const & endpoint_a) = 0;
	virtual void peer_del (cga::transaction const & transaction_a, cga::endpoint_key const & endpoint_a) = 0;
	virtual bool peer_exists (cga::transaction const & transaction_a, cga::endpoint_key const & endpoint_a) const = 0;
	virtual size_t peer_count (cga::transaction const & transaction_a) const = 0;
	virtual void peer_clear (cga::transaction const & transaction_a) = 0;
	virtual cga::store_iterator<cga::endpoint_key, cga::no_value> peers_begin (cga::transaction const & transaction_a) = 0;
	virtual cga::store_iterator<cga::endpoint_key, cga::no_value> peers_end () = 0;

	// Requires a write transaction
	virtual cga::raw_key get_node_id (cga::transaction const &) = 0;

	/** Deletes the node ID from the store */
	virtual void delete_node_id (cga::transaction const &) = 0;

	/** Start read-write transaction */
	virtual cga::transaction tx_begin_write () = 0;

	/** Start read-only transaction */
	virtual cga::transaction tx_begin_read () = 0;

	/**
	 * Start a read-only or read-write transaction
	 * @param write If true, start a read-write transaction
	 */
	virtual cga::transaction tx_begin (bool write = false) = 0;
};
}
