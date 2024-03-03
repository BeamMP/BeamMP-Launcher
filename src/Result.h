#pragma once

#include <boost/outcome.hpp>
#include <boost/outcome/success_failure.hpp>

// attribute because [[nodiscard]] isn't possible on using decls - if you wanna write a standards proposal,
// this is one to write.
template<typename T, typename EC>
using Result __attribute__((warn_unused_result)) = boost::outcome_v2::result<T, EC>;

namespace outcome = boost::outcome_v2;
