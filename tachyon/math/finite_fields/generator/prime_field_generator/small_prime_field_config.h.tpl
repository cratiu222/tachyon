// clang-format off
#include <stdint.h>

#include "absl/base/call_once.h"

#include "tachyon/export.h"
#include "tachyon/build/build_config.h"
#include "tachyon/math/base/big_int.h"

namespace %{namespace} {

class TACHYON_EXPORT %{class}Config {
 public:
  constexpr static const char* kName = "%{namespace}::%{class}";

  constexpr static bool kUseMontgomery = %{use_montgomery};

  constexpr static size_t kModulusBits = %{modulus_bits};
  constexpr static uint32_t kModulus = %{modulus};
  constexpr static uint32_t kModulusMinusOneDivTwo = %{modulus_minus_one_div_two};
  constexpr static uint32_t kModulusPlusOneDivFour = %{modulus_plus_one_div_four};
  constexpr static uint32_t kTrace = %{trace};
  constexpr static uint32_t kTraceMinusOneDivTwo = %{trace_minus_one_div_two};
  constexpr static bool kModulusModFourIsThree = %{modulus_mod_four_is_three};
  constexpr static bool kModulusModSixIsOne = %{modulus_mod_six_is_one};
  constexpr static bool kModulusHasSpareBit = %{modulus_has_spare_bit};
  constexpr static uint32_t kInverse32 = %{inverse32};

  constexpr static uint32_t kOne = %{one};
  constexpr static uint32_t kMinusOne = %{minus_one};
  constexpr static uint32_t kTwoInv = %{two_inv};

  constexpr static bool kHasTwoAdicRootOfUnity = %{has_two_adic_root_of_unity};

  constexpr static bool kHasLargeSubgroupRootOfUnity = %{has_large_subgroup_root_of_unity};

%{if kHasTwoAdicRootOfUnity}
  static uint32_t kSubgroupGenerator;
  constexpr static uint32_t kTwoAdicity = %{two_adicity};
  static uint32_t kTwoAdicRootOfUnity;
%{endif kHasTwoAdicRootOfUnity}

  static void Init() {
    static absl::once_flag once;
    absl::call_once(once, []() {
%{if kHasTwoAdicRootOfUnity}
      kSubgroupGenerator = %{subgroup_generator};
      kTwoAdicRootOfUnity = %{two_adic_root_of_unity};
%{endif kHasTwoAdicRootOfUnity}
    });
  }

  constexpr static uint32_t AddMod(uint32_t a, uint32_t b) {
    // NOTE(chokobole): This assumes that the 2m - 2 < 2³², where m is modulus.
    return Reduce(a + b);
  }

  constexpr static uint32_t SubMod(uint32_t a, uint32_t b) {
    // NOTE(chokobole): This assumes that the 2m - 2 < 2³², where m is modulus.
    return Reduce(a + kModulus - b);
  }

  constexpr static uint32_t Reduce(uint32_t v) {
    %{reduce32}
  }

  constexpr static uint32_t Reduce(uint64_t v) {
    %{reduce64}
  }

  constexpr static uint32_t ToMontgomery(uint32_t v) {
    return (uint64_t{v} << 32) % kModulus;
  }

  constexpr static uint32_t ToMontgomery(uint64_t v) {
    return static_cast<uint32_t>(((tachyon::math::BigInt<2>(v) << 32) % tachyon::math::BigInt<2>(kModulus))[0]);
  }

  constexpr static uint32_t FromMontgomery(uint64_t v) {
    constexpr uint64_t kMask = (uint64_t{1} << 32) - 1;
    uint64_t t = (v * kInverse32) & kMask;
    uint64_t u = t * kModulus;
    uint32_t ret = (v - u) >> 32;
    if (v < u) {
      return ret + kModulus;
    } else {
      return ret;
    }
  }
};

}  // namespace %{namespace}
// clang-format on
