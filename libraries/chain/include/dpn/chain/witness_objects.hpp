#pragma once
#include <dpn/chain/dpn_fwd.hpp>

#include <dpn/protocol/authority.hpp>
#include <dpn/protocol/dpn_operations.hpp>

#include <dpn/chain/util/rd_dynamics.hpp>

#include <dpn/chain/dpn_object_types.hpp>

namespace dpn { namespace chain {

   using dpn::protocol::digest_type;
   using dpn::protocol::public_key_type;
   using dpn::protocol::version;
   using dpn::protocol::hardfork_version;
   using dpn::protocol::price;
   using dpn::protocol::asset;
   using dpn::protocol::asset_symbol_type;
   using dpn::chain::util::rd_dynamics_params;

   /**
    * Witnesses must vote on how to set certain chain properties to ensure a smooth
    * and well functioning network.  Any time @owner is in the active set of witnesses these
    * properties will be used to control the blockchain configuration.
    */
   struct chain_properties
   {
      /**
       *  This fee, paid in DPN, is converted into VESTING SHARES for the new account. Accounts
       *  without vesting shares cannot earn usage rations and therefore are powerless. This minimum
       *  fee requires all accounts to have some kind of commitment to the network that includes the
       *  ability to vote and make transactions.
       */
      asset             account_creation_fee =
         asset( DPN_MIN_ACCOUNT_CREATION_FEE, DPN_SYMBOL );

      /**
       *  This witnesses vote for the maximum_block_size which is used by the network
       *  to tune rate limiting and capacity
       */
      uint32_t          maximum_block_size = DPN_MIN_BLOCK_SIZE_LIMIT * 2;
      uint16_t          dbd_interest_rate  = DPN_DEFAULT_DBD_INTEREST_RATE;
      /**
       * How many free accounts should be created per elected witness block.
       * Scaled so that DPN_ACCOUNT_SUBSIDY_PRECISION represents one account.
       */
      int32_t           account_subsidy_budget = DPN_DEFAULT_ACCOUNT_SUBSIDY_BUDGET;

      /**
       * What fraction of the "stockpiled" free accounts "expire" per elected witness block.
       * Scaled so that 1 << DPN_RD_DECAY_DENOM_SHIFT represents 100% of accounts
       * expiring.
       */
      uint32_t          account_subsidy_decay = DPN_DEFAULT_ACCOUNT_SUBSIDY_DECAY;
   };

   /**
    *  All witnesses with at least 1% net positive approval and
    *  at least 2 weeks old are able to participate in block
    *  production.
    */
   class witness_object : public object< witness_object_type, witness_object >
   {
      DPN_STD_ALLOCATOR_CONSTRUCTOR( witness_object )

      public:
         enum witness_schedule_type
         {
            elected,
            timeshare,
            miner,
            none
         };

         template< typename Constructor, typename Allocator >
         witness_object( Constructor&& c, allocator< Allocator > a )
            :url( a )
         {
            c( *this );
         }

         id_type           id;

         /** the account that has authority over this witness */
         account_name_type owner;
         time_point_sec    created;
         shared_string     url;
         uint32_t          total_missed = 0;
         uint64_t          last_aslot = 0;
         uint64_t          last_confirmed_block_num = 0;

         /**
          * Some witnesses have the job because they did a proof of work,
          * this field indicates where they were in the POW order. After
          * each round, the witness with the lowest pow_worker value greater
          * than 0 is removed.
          */
         uint64_t          pow_worker = 0;

         /**
          *  This is the key used to sign blocks on behalf of this witness
          */
         public_key_type   signing_key;

         chain_properties  props;
         price             dbd_exchange_rate;
         time_point_sec    last_dbd_exchange_update;


         /**
          *  The total votes for this witness. This determines how the witness is ranked for
          *  scheduling.  The top N witnesses by votes are scheduled every round, every one
          *  else takes turns being scheduled proportional to their votes.
          */
         share_type        votes;
         witness_schedule_type schedule = none; /// How the witness was scheduled the last time it was scheduled

         /**
          * These fields are used for the witness scheduling algorithm which uses
          * virtual time to ensure that all witnesses are given proportional time
          * for producing blocks.
          *
          * @ref votes is used to determine speed. The @ref virtual_scheduled_time is
          * the expected time at which this witness should complete a virtual lap which
          * is defined as the position equal to 1000 times MAXVOTES.
          *
          * virtual_scheduled_time = virtual_last_update + (1000*MAXVOTES - virtual_position) / votes
          *
          * Every time the number of votes changes the virtual_position and virtual_scheduled_time must
          * update.  There is a global current virtual_scheduled_time which gets updated every time
          * a witness is scheduled.  To update the virtual_position the following math is performed.
          *
          * virtual_position       = virtual_position + votes * (virtual_current_time - virtual_last_update)
          * virtual_last_update    = virtual_current_time
          * votes                  += delta_vote
          * virtual_scheduled_time = virtual_last_update + (1000*MAXVOTES - virtual_position) / votes
          *
          * @defgroup virtual_time Virtual Time Scheduling
          */
         ///@{
         fc::uint128       virtual_last_update;
         fc::uint128       virtual_position;
         fc::uint128       virtual_scheduled_time = fc::uint128::max_value();
         ///@}

         digest_type       last_work;

         /**
          * This field represents the Dpn blockchain version the witness is running.
          */
         version           running_version;

         hardfork_version  hardfork_version_vote;
         time_point_sec    hardfork_time_vote = DPN_GENESIS_TIME;

         int64_t           available_witness_account_subsidies = 0;
   };


   class witness_vote_object : public object< witness_vote_object_type, witness_vote_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         witness_vote_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         witness_vote_object(){}

         id_type           id;

         account_name_type witness;
         account_name_type account;
   };

   class witness_schedule_object : public object< witness_schedule_object_type, witness_schedule_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         witness_schedule_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         witness_schedule_object(){}

         id_type                                                           id;

         fc::uint128                                                       current_virtual_time;
         uint32_t                                                          next_shuffle_block_num = 1;
         fc::array< account_name_type, DPN_MAX_WITNESSES >             current_shuffled_witnesses;
         uint8_t                                                           num_scheduled_witnesses = 1;
         uint8_t                                                           elected_weight = 1;
         uint8_t                                                           timeshare_weight = 5;
         uint8_t                                                           miner_weight = 1;
         uint32_t                                                          witness_pay_normalization_factor = 25;
         chain_properties                                                  median_props;
         version                                                           majority_version;

         uint8_t max_voted_witnesses            = DPN_MAX_VOTED_WITNESSES_HF0;
         uint8_t max_miner_witnesses            = DPN_MAX_MINER_WITNESSES_HF0;
         uint8_t max_runner_witnesses           = DPN_MAX_RUNNER_WITNESSES_HF0;
         uint8_t hardfork_required_witnesses    = DPN_HARDFORK_REQUIRED_WITNESSES;

         // Derived fields that are stored for easy caching and reading of values.
         rd_dynamics_params account_subsidy_rd;
         rd_dynamics_params account_subsidy_witness_rd;
         int64_t            min_witness_account_subsidy_decay = 0;
   };



   struct by_vote_name;
   struct by_pow;
   struct by_work;
   struct by_schedule_time;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      witness_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< witness_object, witness_id_type, &witness_object::id > >,
         ordered_unique< tag< by_work >,
            composite_key< witness_object,
               member< witness_object, digest_type, &witness_object::last_work >,
               member< witness_object, witness_id_type, &witness_object::id >
            >
         >,
         ordered_unique< tag< by_name >, member< witness_object, account_name_type, &witness_object::owner > >,
         ordered_unique< tag< by_pow >,
            composite_key< witness_object,
               member< witness_object, uint64_t, &witness_object::pow_worker >,
               member< witness_object, witness_id_type, &witness_object::id >
            >
         >,
         ordered_unique< tag< by_vote_name >,
            composite_key< witness_object,
               member< witness_object, share_type, &witness_object::votes >,
               member< witness_object, account_name_type, &witness_object::owner >
            >,
            composite_key_compare< std::greater< share_type >, std::less< account_name_type > >
         >,
         ordered_unique< tag< by_schedule_time >,
            composite_key< witness_object,
               member< witness_object, fc::uint128, &witness_object::virtual_scheduled_time >,
               member< witness_object, witness_id_type, &witness_object::id >
            >
         >
      >,
      allocator< witness_object >
   > witness_index;

   struct by_account_witness;
   struct by_witness_account;
   typedef multi_index_container<
      witness_vote_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< witness_vote_object, witness_vote_id_type, &witness_vote_object::id > >,
         ordered_unique< tag< by_account_witness >,
            composite_key< witness_vote_object,
               member< witness_vote_object, account_name_type, &witness_vote_object::account >,
               member< witness_vote_object, account_name_type, &witness_vote_object::witness >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
         >,
         ordered_unique< tag< by_witness_account >,
            composite_key< witness_vote_object,
               member< witness_vote_object, account_name_type, &witness_vote_object::witness >,
               member< witness_vote_object, account_name_type, &witness_vote_object::account >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
         >
      >, // indexed_by
      allocator< witness_vote_object >
   > witness_vote_index;

   typedef multi_index_container<
      witness_schedule_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< witness_schedule_object, witness_schedule_id_type, &witness_schedule_object::id > >
      >,
      allocator< witness_schedule_object >
   > witness_schedule_index;

} }

#ifdef ENABLE_MIRA
namespace mira {

template<> struct is_static_length< dpn::chain::witness_vote_object > : public boost::true_type {};
template<> struct is_static_length< dpn::chain::witness_schedule_object > : public boost::true_type {};

} // mira
#endif

FC_REFLECT_ENUM( dpn::chain::witness_object::witness_schedule_type, (elected)(timeshare)(miner)(none) )

FC_REFLECT( dpn::chain::chain_properties,
             (account_creation_fee)
             (maximum_block_size)
             (dbd_interest_rate)
             (account_subsidy_budget)
             (account_subsidy_decay)
          )

FC_REFLECT( dpn::chain::witness_object,
             (id)
             (owner)
             (created)
             (url)(votes)(schedule)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)
             (props)
             (dbd_exchange_rate)(last_dbd_exchange_update)
             (last_work)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
             (available_witness_account_subsidies)
          )
CHAINBASE_SET_INDEX_TYPE( dpn::chain::witness_object, dpn::chain::witness_index )

FC_REFLECT( dpn::chain::witness_vote_object, (id)(witness)(account) )
CHAINBASE_SET_INDEX_TYPE( dpn::chain::witness_vote_object, dpn::chain::witness_vote_index )

FC_REFLECT( dpn::chain::witness_schedule_object,
             (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_witnesses)(num_scheduled_witnesses)
             (elected_weight)(timeshare_weight)(miner_weight)(witness_pay_normalization_factor)
             (median_props)(majority_version)
             (max_voted_witnesses)
             (max_miner_witnesses)
             (max_runner_witnesses)
             (hardfork_required_witnesses)
             (account_subsidy_rd)
             (account_subsidy_witness_rd)
             (min_witness_account_subsidy_decay)
          )
CHAINBASE_SET_INDEX_TYPE( dpn::chain::witness_schedule_object, dpn::chain::witness_schedule_index )
