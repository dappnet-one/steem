#pragma once

#include <dpn/chain/dpn_fwd.hpp>
#include <dpn/chain/dpn_object_types.hpp>
#include <dpn/protocol/smt_operations.hpp>

#ifdef DPN_ENABLE_SMT

namespace dpn { namespace chain {

using protocol::curve_id;

enum class smt_phase : uint8_t
{
   account_elevated,
   setup_completed,
   contribution_begin_time_completed,
   contribution_end_time_completed,
   launch_failed,                      /// launch window closed with either not enough contributions or some cap not revealed
   launch_success                      /// enough contributions were declared and caps revealed before launch windows closed
};

/**Note that the object represents both liquid and vesting variant of SMT.
 * The same object is returned by indices when searched by liquid/vesting symbol/nai.
 */
class smt_token_object : public object< smt_token_object_type, smt_token_object >
{
   DPN_STD_ALLOCATOR_CONSTRUCTOR( smt_token_object );

public:

   struct smt_market_maker_state
   {
      asset    dpn_balance;
      asset    token_balance;
      uint32_t reserve_ratio = 0;
   };

public:

   template< typename Constructor, typename Allocator >
   smt_token_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   price    one_vesting_to_one_liquid() const
   {
      int64_t one_smt = std::pow(10, liquid_symbol.decimals());
      return price ( asset( one_smt, liquid_symbol.get_paired_symbol() ), asset( one_smt, liquid_symbol ) );
      // ^ On the assumption that liquid and vesting SMT have the same precision. See issue 2212
   }

   price    get_vesting_share_price() const
   {
      if ( total_vesting_fund_smt == 0 || total_vesting_shares == 0 )
         return one_vesting_to_one_liquid();
         // ^ In original method of globa_property_object it was one liquid to one vesting which seems to be a bug.

      return price( asset( total_vesting_shares, liquid_symbol.get_paired_symbol() ), asset( total_vesting_fund_smt, liquid_symbol ) );
   }

   price    get_reward_vesting_share_price() const
   {
      share_type reward_vesting_shares = total_vesting_shares + pending_rewarded_vesting_shares;
      share_type reward_vesting_smt = total_vesting_fund_smt + pending_rewarded_vesting_smt;

      if( reward_vesting_shares == 0 || reward_vesting_smt == 0 )
          return one_vesting_to_one_liquid();
      // ^ Additional check not found in original get_reward_vesting_share_price. See issue 2212

      return price( asset( reward_vesting_shares, liquid_symbol.get_paired_symbol() ), asset( reward_vesting_smt, liquid_symbol ) );
   }

   // id_type is actually oid<smt_token_object>
   id_type              id;

   /**The object represents both liquid and vesting variant of SMT
    * To get vesting symbol, call liquid_symbol.get_paired_symbol()
    */
   asset_symbol_type    liquid_symbol;
   account_name_type    control_account;
   smt_phase            phase = smt_phase::account_elevated;

   share_type           current_supply = 0;
   share_type           total_vesting_fund_smt = 0;
   share_type           total_vesting_shares = 0;
   share_type           pending_rewarded_vesting_shares = 0;
   share_type           pending_rewarded_vesting_smt = 0;

   smt_market_maker_state  market_maker;

   /// set_setup_parameters
   bool                 allow_voting = true;

   /// set_runtime_parameters
   uint32_t             cashout_window_seconds = DPN_CASHOUT_WINDOW_SECONDS;
   uint32_t             reverse_auction_window_seconds = DPN_REVERSE_AUCTION_WINDOW_SECONDS_HF20;

   uint32_t             vote_regeneration_period_seconds = DPN_VOTING_MANA_REGENERATION_SECONDS;
   uint32_t             votes_per_regeneration_period = SMT_DEFAULT_VOTES_PER_REGEN_PERIOD;

   uint128_t            content_constant = DPN_CONTENT_CONSTANT_HF0;
   uint16_t             percent_curation_rewards = SMT_DEFAULT_PERCENT_CURATION_REWARDS;
   protocol::curve_id   author_reward_curve = curve_id::linear;
   protocol::curve_id   curation_reward_curve = curve_id::square_root;

   bool                 allow_downvotes = true;

   ///parameters for 'smt_setup_operation'
   int64_t                       max_supply = 0;
};

class smt_ico_object : public object< smt_ico_object_type, smt_ico_object >
{
   DPN_STD_ALLOCATOR_CONSTRUCTOR( smt_ico_object );

public:
   template< typename Constructor, typename Allocator >
   smt_ico_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   id_type id;
   asset_symbol_type             symbol;
   dpn::protocol::
   smt_capped_generation_policy  capped_generation_policy;
   time_point_sec                contribution_begin_time;
   time_point_sec                contribution_end_time;
   time_point_sec                launch_time;
   share_type                    dpn_units_soft_cap = -1;
   share_type                    dpn_units_hard_cap = -1;
   asset                         contributed = asset( 0, DPN_SYMBOL );
};

class smt_token_emissions_object : public object< smt_token_emissions_object_type, smt_token_emissions_object >
{
   DPN_STD_ALLOCATOR_CONSTRUCTOR( smt_token_emissions_object );

public:
   template< typename Constructor, typename Allocator >
   smt_token_emissions_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   id_type                               id;
   asset_symbol_type                     symbol;
   time_point_sec                        schedule_time = DPN_GENESIS_TIME;
   dpn::protocol::smt_emissions_unit   emissions_unit;
   uint32_t                              interval_seconds = 0;
   uint32_t                              interval_count = 0;
   time_point_sec                        lep_time = DPN_GENESIS_TIME;
   time_point_sec                        rep_time = DPN_GENESIS_TIME;
   asset                                 lep_abs_amount = asset();
   asset                                 rep_abs_amount = asset();
   uint32_t                              lep_rel_amount_numerator = 0;
   uint32_t                              rep_rel_amount_numerator = 0;
   uint8_t                               rel_amount_denom_bits = 0;
};

class smt_contribution_object : public object< smt_contribution_object_type, smt_contribution_object >
{
   DPN_STD_ALLOCATOR_CONSTRUCTOR( smt_contribution_object );

public:
   template< typename Constructor, typename Allocator >
   smt_contribution_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   id_type                               id;
   asset_symbol_type                     symbol;
   account_name_type                     contributor;
   uint32_t                              contribution_id;
   asset                                 contribution;
};

struct by_symbol_contributor;
struct by_contributor;
struct by_symbol_id;

typedef multi_index_container <
   smt_contribution_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_contribution_object, smt_contribution_object_id_type, &smt_contribution_object::id > >,
      ordered_unique< tag< by_symbol_contributor >,
         composite_key< smt_contribution_object,
            member< smt_contribution_object, asset_symbol_type, &smt_contribution_object::symbol >,
            member< smt_contribution_object, account_name_type, &smt_contribution_object::contributor >,
            member< smt_contribution_object, uint32_t, &smt_contribution_object::contribution_id >
         >
      >,
      ordered_unique< tag< by_symbol_id >,
         composite_key< smt_contribution_object,
            member< smt_contribution_object, asset_symbol_type, &smt_contribution_object::symbol >,
            member< smt_contribution_object, smt_contribution_object_id_type, &smt_contribution_object::id >
         >
      >
#ifndef IS_LOW_MEM
      ,
      ordered_unique< tag< by_contributor >,
         composite_key< smt_contribution_object,
            member< smt_contribution_object, account_name_type, &smt_contribution_object::contributor >,
            member< smt_contribution_object, asset_symbol_type, &smt_contribution_object::symbol >,
            member< smt_contribution_object, uint32_t, &smt_contribution_object::contribution_id >
         >
      >
#endif
   >,
   allocator< smt_contribution_object >
> smt_contribution_index;

struct by_symbol;
struct by_control_account;

typedef multi_index_container <
   smt_token_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_token_object, smt_token_id_type, &smt_token_object::id > >,
      ordered_unique< tag< by_symbol >,
         member< smt_token_object, asset_symbol_type, &smt_token_object::liquid_symbol > >,
      ordered_unique< tag< by_control_account >,
         composite_key< smt_token_object,
            member< smt_token_object, account_name_type, &smt_token_object::control_account >,
            member< smt_token_object, asset_symbol_type, &smt_token_object::liquid_symbol >
         >
      >
   >,
   allocator< smt_token_object >
> smt_token_index;

typedef multi_index_container <
   smt_ico_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_ico_object, smt_ico_object_id_type, &smt_ico_object::id > >,
      ordered_unique< tag< by_symbol >,
         member< smt_ico_object, asset_symbol_type, &smt_ico_object::symbol > >
   >,
   allocator< smt_ico_object >
> smt_ico_index;

struct by_symbol_time;

typedef multi_index_container <
   smt_token_emissions_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< smt_token_emissions_object, smt_token_emissions_object_id_type, &smt_token_emissions_object::id > >,
      ordered_unique< tag< by_symbol_time >,
         composite_key< smt_token_emissions_object,
            member< smt_token_emissions_object, asset_symbol_type, &smt_token_emissions_object::symbol >,
            member< smt_token_emissions_object, time_point_sec, &smt_token_emissions_object::schedule_time >
         >
      >
   >,
   allocator< smt_token_emissions_object >
> smt_token_emissions_index;

} } // namespace dpn::chain

FC_REFLECT_ENUM( dpn::chain::smt_phase,
                  (account_elevated)
                  (setup_completed)
                  (contribution_begin_time_completed)
                  (contribution_end_time_completed)
                  (launch_failed)
                  (launch_success)
)

FC_REFLECT( dpn::chain::smt_token_object::smt_market_maker_state,
   (dpn_balance)
   (token_balance)
   (reserve_ratio)
)

FC_REFLECT( dpn::chain::smt_token_object,
   (id)
   (liquid_symbol)
   (control_account)
   (phase)
   (current_supply)
   (total_vesting_fund_smt)
   (total_vesting_shares)
   (pending_rewarded_vesting_shares)
   (pending_rewarded_vesting_smt)
   (allow_downvotes)
   (market_maker)
   (allow_voting)
   (cashout_window_seconds)
   (reverse_auction_window_seconds)
   (vote_regeneration_period_seconds)
   (votes_per_regeneration_period)
   (content_constant)
   (percent_curation_rewards)
   (author_reward_curve)
   (curation_reward_curve)
   (max_supply)
)

FC_REFLECT( dpn::chain::smt_ico_object,
   (id)
   (symbol)
   (capped_generation_policy)
   (contribution_begin_time)
   (contribution_end_time)
   (launch_time)
   (dpn_units_soft_cap)
   (dpn_units_hard_cap)
   (contributed)
)

FC_REFLECT( dpn::chain::smt_token_emissions_object,
   (id)
   (symbol)
   (schedule_time)
   (emissions_unit)
   (interval_seconds)
   (interval_count)
   (lep_time)
   (rep_time)
   (lep_abs_amount)
   (rep_abs_amount)
   (lep_rel_amount_numerator)
   (rep_rel_amount_numerator)
   (rel_amount_denom_bits)
)

FC_REFLECT( dpn::chain::smt_contribution_object,
   (id)
   (symbol)
   (contributor)
   (contribution_id)
   (contribution)
)

CHAINBASE_SET_INDEX_TYPE( dpn::chain::smt_token_object, dpn::chain::smt_token_index )
CHAINBASE_SET_INDEX_TYPE( dpn::chain::smt_ico_object, dpn::chain::smt_ico_index )
CHAINBASE_SET_INDEX_TYPE( dpn::chain::smt_token_emissions_object, dpn::chain::smt_token_emissions_index )
CHAINBASE_SET_INDEX_TYPE( dpn::chain::smt_contribution_object, dpn::chain::smt_contribution_index )

#endif
