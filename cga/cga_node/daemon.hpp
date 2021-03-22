#include <cga/lib/errors.hpp>
#include <cga/node/node.hpp>
#include <cga/node/rpc.hpp>

namespace cga_daemon
{
class daemon
{
public:
	void run (boost::filesystem::path const &, cga::node_flags const & flags);
};
class daemon_config
{
public:
	daemon_config ();
	cga::error deserialize_json (bool &, cga::jsonconfig &);
	cga::error serialize_json (cga::jsonconfig &);
	/** 
	 * Returns true if an upgrade occurred
	 * @param version The version to upgrade to.
	 * @param config Configuration to upgrade.
	 */
	bool upgrade_json (unsigned version, cga::jsonconfig & config);
	bool rpc_enable;
	cga::rpc_config rpc;
	cga::node_config node;
	bool opencl_enable;
	cga::opencl_config opencl;
	int json_version () const
	{
		return 2;
	}
};
}
