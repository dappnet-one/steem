#include <dpn/protocol/validation.hpp>
#include <dpn/protocol/dpn_required_actions.hpp>

namespace dpn { namespace protocol {

#ifdef IS_TEST_NET
void example_required_action::validate()const
{
   validate_account_name( account );
}

bool operator==( const example_required_action& lhs, const example_required_action& rhs )
{
   return lhs.account == rhs.account;
}
#endif

} } //dpn::protocol
