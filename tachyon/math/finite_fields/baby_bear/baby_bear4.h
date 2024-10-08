#ifndef TACHYON_MATH_FINITE_FIELDS_BABY_BEAR_BABY_BEAR4_H_
#define TACHYON_MATH_FINITE_FIELDS_BABY_BEAR_BABY_BEAR4_H_

#include "tachyon/math/finite_fields/baby_bear/internal/baby_bear4.h"
#include "tachyon/math/finite_fields/baby_bear/internal/packed_baby_bear4.h"

namespace tachyon::math {

template <>
struct ExtendedPackedFieldTraits<BabyBear4> {
  constexpr static bool kIsExtendedPackedField = false;
  using ExtendedPackedField = PackedBabyBear4;
};

}  // namespace tachyon::math

#endif  // TACHYON_MATH_FINITE_FIELDS_BABY_BEAR_BABY_BEAR4_H_
