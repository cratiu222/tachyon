#include "benchmark/ec/ec_config.h"

#include "tachyon/base/ranges/algorithm.h"

namespace tachyon::benchmark {

bool ECConfig::Parse(int argc, char** argv) {
  parser_.AddFlag<base::Flag<std::vector<size_t>>>(&point_nums_)
      .set_short_name("-n")
      .set_required()
      .set_help("The number of points to test");

  if (!Config::Parse(argc, argv, {/*include_check_results=*/false})) {
    return false;
  }

  base::ranges::sort(point_nums_);  // NOLINT(build/include_what_you_use)
  return true;
}

}  // namespace tachyon::benchmark
