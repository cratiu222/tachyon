#ifndef BENCHMARK_MSM_MSM_CONFIG_H_
#define BENCHMARK_MSM_MSM_CONFIG_H_

#include <stdint.h>

#include <vector>

// clang-format off
#include "benchmark/config.h"
// clang-format on
#include "tachyon/math/elliptic_curves/msm/test/variable_base_msm_test_set.h"

namespace tachyon::benchmark {

class MSMConfig : public Config {
 public:
  enum class TestSet {
    kRandom,
    kNonUniform,
  };

  struct Options : public Config::Options {
    bool include_vendors = false;
  };

  MSMConfig();
  explicit MSMConfig(const Options& options);
  MSMConfig(const MSMConfig& other) = delete;
  MSMConfig& operator=(const MSMConfig& other) = delete;

  const std::vector<uint32_t>& exponents() const { return exponents_; }

  std::vector<size_t> GetPointNums() const;

  template <typename Point, typename Bucket>
  bool GenerateTestSet(size_t size,
                       math::VariableBaseMSMTestSet<Point, Bucket>* out) const {
    switch (test_set_) {
      case TestSet::kRandom:
        *out = math::VariableBaseMSMTestSet<Point, Bucket>::Random(
            size, math::VariableBaseMSMMethod::kNone);
        return true;
      case TestSet::kNonUniform:
        *out = math::VariableBaseMSMTestSet<Point, Bucket>::NonUniform(
            size, 1, math::VariableBaseMSMMethod::kNone);
        return true;
    }
    return false;
  }

 private:
  // Config methods
  void PostParse() override;
  bool Validate() const override;

  std::vector<uint32_t> exponents_;
  TestSet test_set_;
  bool include_vendors_;
};

}  // namespace tachyon::benchmark

#endif  // BENCHMARK_MSM_MSM_CONFIG_H_
