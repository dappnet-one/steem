#include <dpn/plugins/condenser_api/condenser_api_plugin.hpp>
#include <dpn/plugins/condenser_api/condenser_api.hpp>
#include <dpn/plugins/chain/chain_plugin.hpp>

namespace dpn { namespace plugins { namespace condenser_api {

condenser_api_plugin::condenser_api_plugin() {}
condenser_api_plugin::~condenser_api_plugin() {}

void condenser_api_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cli.add_options()
      ("disable-get-block", "Disable get_block API call" )
      ;
}

void condenser_api_plugin::plugin_initialize( const variables_map& options )
{
   api = std::make_shared< condenser_api >();
}

void condenser_api_plugin::plugin_startup()
{
   api->api_startup();
}

void condenser_api_plugin::plugin_shutdown() {}

} } } // dpn::plugins::condenser_api
