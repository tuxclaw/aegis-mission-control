#pragma once

#include <tl/expected.hpp>

#include "core/aegis_error.h"

namespace aegis {

template <typename T>
using Result = tl::expected<T, AegisError>;

}  // namespace aegis
