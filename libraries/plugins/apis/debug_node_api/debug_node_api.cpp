#include <dpn/plugins/debug_node_api/debug_node_api_plugin.hpp>
#include <dpn/plugins/debug_node_api/debug_node_api.hpp>

#include <fc/filesystem.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <dpn/chain/block_log.hpp>
#include <dpn/chain/account_object.hpp>
#include <dpn/chain/database.hpp>
#include <dpn/chain/witness_objects.hpp>

#include <dpn/utilities/key_conversion.hpp>

namespace dpn { namespace plugins { namespace debug_node {

namespace detail {

class debug_node_api_impl
{
   public:
      debug_node_api_impl() :
         _db( appbase::app().get_plugin< chain::chain_plugin >().db() ),
         _debug_node( appbase::app().get_plugin< debug_node_plugin >() ) {}

      DECLARE_API_IMPL(
         (debug_push_blocks)
         (debug_generate_blocks)
         (debug_generate_blocks_until)
         (debug_pop_block)
         (debug_get_witness_schedule)
         (debug_get_hardfork_property_object)
         (debug_set_hardfork)
         (debug_has_hardfork)
         (debug_get_json_schema)
      )

      chain::database& _db;
      debug_node::debug_node_plugin& _debug_node;
};

DEFINE_API_IMPL( debug_node_api_impl, debug_push_blocks )
{
   auto& src_filename = args.src_filename;
   auto count = args.count;
   auto skip_validate_invariants = args.skip_validate_invariants;

   if( count == 0 )
      return { 0 };

   fc::path src_path = fc::path( src_filename );
   fc::path index_path = fc::path( src_filename + ".index" );
   if( fc::exists(src_path) && fc::exists(index_path) &&
      !fc::is_directory(src_path) && !fc::is_directory(index_path)
     )
   {
      ilog( "Loading ${n} from block_log ${fn}", ("n", count)("fn", src_filename) );
      idump( (src_filename)(count)(skip_validate_invariants) );
      chain::block_log log;
      log.open( src_path );
      uint32_t first_block = _db.head_block_num()+1;
      uint32_t skip_flags = chain::database::skip_nothing;
      if( skip_validate_invariants )
         skip_flags = skip_flags | chain::database::skip_validate_invariants;
      for( uint32_t i=0; i<count; i++ )
      {
         //fc::optional< chain::signed_block > block = log.read_block( log.get_block_pos( first_block + i ) );
         uint64_t block_pos = log.get_block_pos( first_block + i );
         if( block_pos == chain::block_log::npos )
         {
            wlog( "Block database ${fn} only contained ${i} of ${n} requested blocks", ("i", i)("n", count)("fn", src_filename) );
            return { i };
         }

         decltype( log.read_block(0) ) result;

         try
         {
            result = log.read_block( block_pos );
         }
         catch( const fc::exception& e )
         {
            elog( "Could not read block ${i} of ${n}", ("i", i)("n", count) );
            continue;
         }

         try
         {
            _db.push_block( result.first, skip_flags );
         }
         catch( const fc::exception& e )
         {
            elog( "Got exception pushing block ${bn} : ${bid} (${i} of ${n})", ("bn", result.first.block_num())("bid", result.first.id())("i", i)("n", count) );
            elog( "Exception backtrace: ${bt}", ("bt", e.to_detail_string()) );
         }
      }
      ilog( "Completed loading block_database successfully" );
      return { count };
   }
   return { 0 };
}

DEFINE_API_IMPL( debug_node_api_impl, debug_generate_blocks )
{
   debug_generate_blocks_return ret;
   _debug_node.debug_generate_blocks( ret, args );
   return ret;
}

DEFINE_API_IMPL( debug_node_api_impl, debug_generate_blocks_until )
{
   return { _debug_node.debug_generate_blocks_until( args.debug_key, args.head_block_time, args.generate_sparsely, chain::database::skip_nothing ) };
}

DEFINE_API_IMPL( debug_node_api_impl, debug_pop_block )
{
   return { _db.fetch_block_by_number( _db.head_block_num() ) };
}

DEFINE_API_IMPL( debug_node_api_impl, debug_get_witness_schedule )
{
   return _db.get( chain::witness_schedule_id_type() );
}

DEFINE_API_IMPL( debug_node_api_impl, debug_get_hardfork_property_object )
{
   return _db.get( chain::hardfork_property_id_type() );
}

DEFINE_API_IMPL( debug_node_api_impl, debug_set_hardfork )
{
   if( args.hardfork_id > DPN_NUM_HARDFORKS )
      return {};

   _debug_node.debug_update( [=]( chain::database& db )
   {
      db.set_hardfork( args.hardfork_id, false );
   });

   return {};
}

DEFINE_API_IMPL( debug_node_api_impl, debug_has_hardfork )
{
   return { _db.get( chain::hardfork_property_id_type() ).last_hardfork >= args.hardfork_id };
}

DEFINE_API_IMPL( debug_node_api_impl, debug_get_json_schema )
{
   return { _db.get_json_schema() };
}

} // detail

debug_node_api::debug_node_api(): my( new detail::debug_node_api_impl() )
{
   JSON_RPC_REGISTER_API( DPN_DEBUG_NODE_API_PLUGIN_NAME );
}

debug_node_api::~debug_node_api() {}

DEFINE_LOCKLESS_APIS( debug_node_api,
   (debug_push_blocks)
   (debug_generate_blocks)
   (debug_generate_blocks_until)
   (debug_pop_block)
   (debug_get_witness_schedule)
   (debug_get_hardfork_property_object)
   (debug_set_hardfork)
   (debug_has_hardfork)
   (debug_get_json_schema)
)

} } } // dpn::plugins::debug_node
