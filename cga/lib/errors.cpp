#include "cga/lib/errors.hpp"

std::string cga::error_common_messages::message (int ev) const
{
	switch (static_cast<cga::error_common> (ev))
	{
		case cga::error_common::generic:
			return "Unknown error";
		case cga::error_common::missing_account:
			return "Missing account";
		case cga::error_common::missing_balance:
			return "Missing balance";
		case cga::error_common::missing_link:
			return "Missing link, source or destination";
		case cga::error_common::missing_previous:
			return "Missing previous";
		case cga::error_common::missing_representative:
			return "Missing representative";
		case cga::error_common::missing_signature:
			return "Missing signature";
		case cga::error_common::missing_work:
			return "Missing work";
		case cga::error_common::exception:
			return "Exception thrown";
		case cga::error_common::account_exists:
			return "Account already exists";
		case cga::error_common::account_not_found:
			return "Account not found";
		case cga::error_common::account_not_found_wallet:
			return "Account not found in wallet";
		case cga::error_common::bad_account_number:
			return "Bad account number";
		case cga::error_common::bad_balance:
			return "Bad balance";
		case cga::error_common::bad_link:
			return "Bad link value";
		case cga::error_common::bad_previous:
			return "Bad previous hash";
		case cga::error_common::bad_representative_number:
			return "Bad representative";
		case cga::error_common::bad_source:
			return "Bad source";
		case cga::error_common::bad_signature:
			return "Bad signature";
		case cga::error_common::bad_private_key:
			return "Bad private key";
		case cga::error_common::bad_public_key:
			return "Bad public key";
		case cga::error_common::bad_seed:
			return "Bad seed";
		case cga::error_common::bad_threshold:
			return "Bad threshold number";
		case cga::error_common::bad_wallet_number:
			return "Bad wallet number";
		case cga::error_common::bad_work_format:
			return "Bad work";
		case cga::error_common::insufficient_balance:
			return "Insufficient balance";
		case cga::error_common::invalid_amount:
			return "Invalid amount number";
		case cga::error_common::invalid_amount_big:
			return "Amount too big";
		case cga::error_common::invalid_count:
			return "Invalid count";
		case cga::error_common::invalid_ip_address:
			return "Invalid IP address";
		case cga::error_common::invalid_port:
			return "Invalid port";
		case cga::error_common::invalid_index:
			return "Invalid index";
		case cga::error_common::invalid_type_conversion:
			return "Invalid type conversion";
		case cga::error_common::invalid_work:
			return "Invalid work";
		case cga::error_common::numeric_conversion:
			return "Numeric conversion error";
		case cga::error_common::wallet_lmdb_max_dbs:
			return "Failed to create wallet. Increase lmdb_max_dbs in node config";
		case cga::error_common::wallet_locked:
			return "Wallet is locked";
		case cga::error_common::wallet_not_found:
			return "Wallet not found";
	}

	return "Invalid error code";
}

std::string cga::error_blocks_messages::message (int ev) const
{
	switch (static_cast<cga::error_blocks> (ev))
	{
		case cga::error_blocks::generic:
			return "Unknown error";
		case cga::error_blocks::bad_hash_number:
			return "Bad hash number";
		case cga::error_blocks::invalid_block:
			return "Block is invalid";
		case cga::error_blocks::invalid_block_hash:
			return "Invalid block hash";
		case cga::error_blocks::invalid_type:
			return "Invalid block type";
		case cga::error_blocks::not_found:
			return "Block not found";
		case cga::error_blocks::work_low:
			return "Block work is less than threshold";
	}

	return "Invalid error code";
}

std::string cga::error_rpc_messages::message (int ev) const
{
	switch (static_cast<cga::error_rpc> (ev))
	{
		case cga::error_rpc::generic:
			return "Unknown error";
		case cga::error_rpc::bad_destination:
			return "Bad destination account";
		case cga::error_rpc::bad_key:
			return "Bad key";
		case cga::error_rpc::bad_link:
			return "Bad link number";
		case cga::error_rpc::bad_previous:
			return "Bad previous";
		case cga::error_rpc::bad_representative_number:
			return "Bad representative number";
		case cga::error_rpc::bad_source:
			return "Bad source";
		case cga::error_rpc::bad_timeout:
			return "Bad timeout number";
		case cga::error_rpc::block_create_balance_mismatch:
			return "Balance mismatch for previous block";
		case cga::error_rpc::block_create_key_required:
			return "Private key or local wallet and account required";
		case cga::error_rpc::block_create_public_key_mismatch:
			return "Incorrect key for given account";
		case cga::error_rpc::block_create_requirements_state:
			return "Previous, representative, final balance and link (source or destination) are required";
		case cga::error_rpc::block_create_requirements_open:
			return "Representative account and source hash required";
		case cga::error_rpc::block_create_requirements_receive:
			return "Previous hash and source hash required";
		case cga::error_rpc::block_create_requirements_change:
			return "Representative account and previous hash required";
		case cga::error_rpc::block_create_requirements_send:
			return "Destination account, previous hash, current balance and amount required";
		case cga::error_rpc::confirmation_not_found:
			return "Active confirmation not found";
		case cga::error_rpc::invalid_balance:
			return "Invalid balance number";
		case cga::error_rpc::invalid_destinations:
			return "Invalid destinations number";
		case cga::error_rpc::invalid_offset:
			return "Invalid offset";
		case cga::error_rpc::invalid_missing_type:
			return "Invalid or missing type argument";
		case cga::error_rpc::invalid_root:
			return "Invalid root hash";
		case cga::error_rpc::invalid_sources:
			return "Invalid sources number";
		case cga::error_rpc::invalid_subtype:
			return "Invalid block subtype";
		case cga::error_rpc::invalid_subtype_balance:
			return "Invalid block balance for given subtype";
		case cga::error_rpc::invalid_subtype_epoch_link:
			return "Invalid epoch link";
		case cga::error_rpc::invalid_subtype_previous:
			return "Invalid previous block for given subtype";
		case cga::error_rpc::invalid_timestamp:
			return "Invalid timestamp";
		case cga::error_rpc::payment_account_balance:
			return "Account has non-zero balance";
		case cga::error_rpc::payment_unable_create_account:
			return "Unable to create transaction account";
		case cga::error_rpc::rpc_control_disabled:
			return "RPC control is disabled";
		case cga::error_rpc::sign_hash_disabled:
			return "Signing by block hash is disabled";
		case cga::error_rpc::source_not_found:
			return "Source not found";
	}

	return "Invalid error code";
}

std::string cga::error_process_messages::message (int ev) const
{
	switch (static_cast<cga::error_process> (ev))
	{
		case cga::error_process::generic:
			return "Unknown error";
		case cga::error_process::bad_signature:
			return "Bad signature";
		case cga::error_process::old:
			return "Old block";
		case cga::error_process::negative_spend:
			return "Negative spend";
		case cga::error_process::fork:
			return "Fork";
		case cga::error_process::unreceivable:
			return "Unreceivable";
		case cga::error_process::gap_previous:
			return "Gap previous block";
		case cga::error_process::gap_source:
			return "Gap source block";
		case cga::error_process::opened_burn_account:
			return "Burning account";
		case cga::error_process::balance_mismatch:
			return "Balance and amount delta do not match";
		case cga::error_process::block_position:
			return "This block cannot follow the previous block";
		case cga::error_process::other:
			return "Error processing block";
	}

	return "Invalid error code";
}

std::string cga::error_config_messages::message (int ev) const
{
	switch (static_cast<cga::error_config> (ev))
	{
		case cga::error_config::generic:
			return "Unknown error";
		case cga::error_config::invalid_value:
			return "Invalid configuration value";
		case cga::error_config::missing_value:
			return "Missing value in configuration";
	}

	return "Invalid error code";
}
