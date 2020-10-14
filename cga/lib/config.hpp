#pragma once

#include <chrono>
#include <cstddef>

namespace cga
{
/**
 * Network variants with different genesis blocks and network parameters
 * @warning Enum values are used for comparison; do not change.
 */
enum class cga_networks
{
	// Low work parameters, publicly known genesis key, test IP ports
	cga_test_network = 0,
	
	// Normal work parameters, secret beta genesis key, beta IP ports
	cga_beta_network = 17,
	
	// Normal work parameters, secret live key, live IP ports
	cga_live_network = 3,
	
};
cga::cga_networks constexpr cga_network = cga_networks::ACTIVE_NETWORK;
bool constexpr is_live_network = cga_network == cga_networks::cga_live_network;
bool constexpr is_beta_network = cga_network == cga_networks::cga_beta_network;
bool constexpr is_test_network = cga_network == cga_networks::cga_test_network;

std::chrono::milliseconds const transaction_timeout = std::chrono::milliseconds (1000);
}
