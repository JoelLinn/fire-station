#pragma once

#include "Config.h"

#ifdef HAVE_STD_FORMAT
#include <format>
namespace fmt = std;
#else
#include <fmt/core.h>
#endif
