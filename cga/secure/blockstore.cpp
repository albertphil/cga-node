#include <cga/node/common.hpp>
#include <cga/node/wallet.hpp>
#include <cga/secure/blockstore.hpp>

#include <boost/polymorphic_cast.hpp>

#include <boost/endian/conversion.hpp>

cga::block_sideband::block_sideband (cga::block_type type_a, cga::account const & account_a, cga::block_hash const & successor_a, cga::amount const & balance_a, uint64_t height_a, uint64_t timestamp_a) :
type (type_a),
successor (successor_a),
account (account_a),
balance (balance_a),
height (height_a),
timestamp (timestamp_a)
{
}

size_t cga::block_sideband::size (cga::block_type type_a)
{
	size_t result (0);
	result += sizeof (successor);
	if (type_a != cga::block_type::state && type_a != cga::block_type::open)
	{
		result += sizeof (account);
	}
	if (type_a != cga::block_type::open)
	{
		result += sizeof (height);
	}
	if (type_a == cga::block_type::receive || type_a == cga::block_type::change || type_a == cga::block_type::open)
	{
		result += sizeof (balance);
	}
	result += sizeof (timestamp);
	return result;
}

void cga::block_sideband::serialize (cga::stream & stream_a) const
{
	cga::write (stream_a, successor.bytes);
	if (type != cga::block_type::state && type != cga::block_type::open)
	{
		cga::write (stream_a, account.bytes);
	}
	if (type != cga::block_type::open)
	{
		cga::write (stream_a, boost::endian::native_to_big (height));
	}
	if (type == cga::block_type::receive || type == cga::block_type::change || type == cga::block_type::open)
	{
		cga::write (stream_a, balance.bytes);
	}
	cga::write (stream_a, boost::endian::native_to_big (timestamp));
}

bool cga::block_sideband::deserialize (cga::stream & stream_a)
{
	bool result (false);
	try
	{
		cga::read (stream_a, successor.bytes);
		if (type != cga::block_type::state && type != cga::block_type::open)
		{
			cga::read (stream_a, account.bytes);
		}
		if (type != cga::block_type::open)
		{
			cga::read (stream_a, height);
			boost::endian::big_to_native_inplace (height);
		}
		else
		{
			height = 1;
		}
		if (type == cga::block_type::receive || type == cga::block_type::change || type == cga::block_type::open)
		{
			cga::read (stream_a, balance.bytes);
		}
		cga::read (stream_a, timestamp);
		boost::endian::big_to_native_inplace (timestamp);
	}
	catch (std::runtime_error &)
	{
		result = true;
	}

	return result;
}

cga::summation_visitor::summation_visitor (cga::transaction const & transaction_a, cga::block_store & store_a) :
transaction (transaction_a),
store (store_a)
{
}

void cga::summation_visitor::send_block (cga::send_block const & block_a)
{
	assert (current->type != summation_type::invalid && current != nullptr);
	if (current->type == summation_type::amount)
	{
		sum_set (block_a.hashables.balance.number ());
		current->balance_hash = block_a.hashables.previous;
		current->amount_hash = 0;
	}
	else
	{
		sum_add (block_a.hashables.balance.number ());
		current->balance_hash = 0;
	}
}

void cga::summation_visitor::state_block (cga::state_block const & block_a)
{
	assert (current->type != summation_type::invalid && current != nullptr);
	sum_set (block_a.hashables.balance.number ());
	if (current->type == summation_type::amount)
	{
		current->balance_hash = block_a.hashables.previous;
		current->amount_hash = 0;
	}
	else
	{
		current->balance_hash = 0;
	}
}

void cga::summation_visitor::receive_block (cga::receive_block const & block_a)
{
	assert (current->type != summation_type::invalid && current != nullptr);
	if (current->type == summation_type::amount)
	{
		current->amount_hash = block_a.hashables.source;
	}
	else
	{
		cga::block_info block_info;
		if (!store.block_info_get (transaction, block_a.hash (), block_info))
		{
			sum_add (block_info.balance.number ());
			current->balance_hash = 0;
		}
		else
		{
			current->amount_hash = block_a.hashables.source;
			current->balance_hash = block_a.hashables.previous;
		}
	}
}

void cga::summation_visitor::open_block (cga::open_block const & block_a)
{
	assert (current->type != summation_type::invalid && current != nullptr);
	if (current->type == summation_type::amount)
	{
		if (block_a.hashables.source != cga::genesis_account)
		{
			current->amount_hash = block_a.hashables.source;
		}
		else
		{
			sum_set (cga::genesis_amount);
			current->amount_hash = 0;
		}
	}
	else
	{
		current->amount_hash = block_a.hashables.source;
		current->balance_hash = 0;
	}
}

void cga::summation_visitor::change_block (cga::change_block const & block_a)
{
	assert (current->type != summation_type::invalid && current != nullptr);
	if (current->type == summation_type::amount)
	{
		sum_set (0);
		current->amount_hash = 0;
	}
	else
	{
		cga::block_info block_info;
		if (!store.block_info_get (transaction, block_a.hash (), block_info))
		{
			sum_add (block_info.balance.number ());
			current->balance_hash = 0;
		}
		else
		{
			current->balance_hash = block_a.hashables.previous;
		}
	}
}

cga::summation_visitor::frame cga::summation_visitor::push (cga::summation_visitor::summation_type type_a, cga::block_hash const & hash_a)
{
	frames.emplace (type_a, type_a == summation_type::balance ? hash_a : 0, type_a == summation_type::amount ? hash_a : 0);
	return frames.top ();
}

void cga::summation_visitor::sum_add (cga::uint128_t addend_a)
{
	current->sum += addend_a;
	result = current->sum;
}

void cga::summation_visitor::sum_set (cga::uint128_t value_a)
{
	current->sum = value_a;
	result = current->sum;
}

cga::uint128_t cga::summation_visitor::compute_internal (cga::summation_visitor::summation_type type_a, cga::block_hash const & hash_a)
{
	push (type_a, hash_a);

	/*
	 Invocation loop representing balance and amount computations calling each other.
	 This is usually better done by recursion or something like boost::coroutine2, but
	 segmented stacks are not supported on all platforms so we do it manually to avoid
	 stack overflow (the mutual calls are not tail-recursive so we cannot rely on the
	 compiler optimizing that into a loop, though a future alternative is to do a
	 CPS-style implementation to enforce tail calls.)
	*/
	while (frames.size () > 0)
	{
		current = &frames.top ();
		assert (current->type != summation_type::invalid && current != nullptr);

		if (current->type == summation_type::balance)
		{
			if (current->awaiting_result)
			{
				sum_add (current->incoming_result);
				current->awaiting_result = false;
			}

			while (!current->awaiting_result && (!current->balance_hash.is_zero () || !current->amount_hash.is_zero ()))
			{
				if (!current->amount_hash.is_zero ())
				{
					// Compute amount
					current->awaiting_result = true;
					push (summation_type::amount, current->amount_hash);
					current->amount_hash = 0;
				}
				else
				{
					auto block (store.block_get (transaction, current->balance_hash));
					assert (block != nullptr);
					block->visit (*this);
				}
			}

			epilogue ();
		}
		else if (current->type == summation_type::amount)
		{
			if (current->awaiting_result)
			{
				sum_set (current->sum < current->incoming_result ? current->incoming_result - current->sum : current->sum - current->incoming_result);
				current->awaiting_result = false;
			}

			while (!current->awaiting_result && (!current->amount_hash.is_zero () || !current->balance_hash.is_zero ()))
			{
				if (!current->amount_hash.is_zero ())
				{
					auto block (store.block_get (transaction, current->amount_hash));
					if (block != nullptr)
					{
						block->visit (*this);
					}
					else
					{
						if (current->amount_hash == cga::genesis_account)
						{
							sum_set (std::numeric_limits<cga::uint128_t>::max ());
							current->amount_hash = 0;
						}
						else
						{
							assert (false);
							sum_set (0);
							current->amount_hash = 0;
						}
					}
				}
				else
				{
					// Compute balance
					current->awaiting_result = true;
					push (summation_type::balance, current->balance_hash);
					current->balance_hash = 0;
				}
			}

			epilogue ();
		}
	}

	return result;
}

void cga::summation_visitor::epilogue ()
{
	if (!current->awaiting_result)
	{
		frames.pop ();
		if (frames.size () > 0)
		{
			frames.top ().incoming_result = current->sum;
		}
	}
}

cga::uint128_t cga::summation_visitor::compute_amount (cga::block_hash const & block_hash)
{
	return compute_internal (summation_type::amount, block_hash);
}

cga::uint128_t cga::summation_visitor::compute_balance (cga::block_hash const & block_hash)
{
	return compute_internal (summation_type::balance, block_hash);
}

cga::representative_visitor::representative_visitor (cga::transaction const & transaction_a, cga::block_store & store_a) :
transaction (transaction_a),
store (store_a),
result (0)
{
}

void cga::representative_visitor::compute (cga::block_hash const & hash_a)
{
	current = hash_a;
	while (result.is_zero ())
	{
		auto block (store.block_get (transaction, current));
		assert (block != nullptr);
		block->visit (*this);
	}
}

void cga::representative_visitor::send_block (cga::send_block const & block_a)
{
	current = block_a.previous ();
}

void cga::representative_visitor::receive_block (cga::receive_block const & block_a)
{
	current = block_a.previous ();
}

void cga::representative_visitor::open_block (cga::open_block const & block_a)
{
	result = block_a.hash ();
}

void cga::representative_visitor::change_block (cga::change_block const & block_a)
{
	result = block_a.hash ();
}

void cga::representative_visitor::state_block (cga::state_block const & block_a)
{
	result = block_a.hash ();
}
