#pragma once

#include <boost/outcome.hpp>
#include <boost/outcome/success_failure.hpp>

#ifdef WIN32
#define BEAMMP_WARN_UNUSED
#else
#define BEAMMP_WARN_UNUSED __attribute__((warn_unused_result))
#endif

// attribute because [[nodiscard]] isn't possible on using decls - if you wanna write a standards proposal,
// this is one to write.
template<typename T, typename EC>
using Result BEAMMP_WARN_UNUSED = boost::outcome_v2::result<T, EC>;

namespace outcome = boost::outcome_v2;
