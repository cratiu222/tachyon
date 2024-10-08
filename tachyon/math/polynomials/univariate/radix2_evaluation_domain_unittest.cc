#include "tachyon/math/polynomials/univariate/radix2_evaluation_domain.h"

#include <memory>

#include "gtest/gtest.h"

#include "tachyon/math/finite_fields/baby_bear/baby_bear.h"
#include "tachyon/math/finite_fields/koala_bear/koala_bear.h"
#include "tachyon/math/finite_fields/test/finite_field_test.h"
#include "tachyon/math/matrix/matrix_types.h"
#include "tachyon/math/polynomials/univariate/naive_batch_fft.h"

namespace tachyon::math {

namespace {

template <typename F>
class Radix2EvaluationDomainTest
    : public FiniteFieldTest<typename PackedFieldTraits<F>::PackedField> {};

}  // namespace

using PrimeFieldTypes = testing::Types<BabyBear, KoalaBear>;
TYPED_TEST_SUITE(Radix2EvaluationDomainTest, PrimeFieldTypes);

TYPED_TEST(Radix2EvaluationDomainTest, FFTBatchDeath) {
  using F = TypeParam;
  std::unique_ptr<Radix2EvaluationDomain<F>> domain =
      Radix2EvaluationDomain<F>::Create(1);
  RowMajorMatrix<F> matrix = RowMajorMatrix<F>::Random(1, 3);
  EXPECT_DEATH(domain->FFTBatch(matrix), "");
}

TYPED_TEST(Radix2EvaluationDomainTest, FFTBatch) {
  using F = TypeParam;
  for (uint32_t log_r = 1; log_r < 5; ++log_r) {
    RowMajorMatrix<F> expected =
        RowMajorMatrix<F>::Random(size_t{1} << log_r, 3);
    RowMajorMatrix<F> result = expected;
    NaiveBatchFFT<F> naive;
    naive.FFTBatch(expected);
    std::unique_ptr<Radix2EvaluationDomain<F>> domain =
        Radix2EvaluationDomain<F>::Create(size_t{1} << log_r);
    domain->FFTBatch(result);
    EXPECT_EQ(expected, result);
  }
}

TYPED_TEST(Radix2EvaluationDomainTest, CosetLDEBatchDeath) {
  using F = TypeParam;
  std::unique_ptr<Radix2EvaluationDomain<F>> domain =
      Radix2EvaluationDomain<F>::Create(1);
  RowMajorMatrix<F> matrix = RowMajorMatrix<F>::Random(1, 3);
  F shift = F::FromMontgomery(F::Config::kSubgroupGenerator);
  RowMajorMatrix<F> result(2, 3);
  EXPECT_DEATH(domain->CosetLDEBatch(std::move(matrix), 1, shift, result), "");
}

TYPED_TEST(Radix2EvaluationDomainTest, CosetLDEBatch) {
  using F = TypeParam;
  for (uint32_t log_r = 1; log_r < 5; ++log_r) {
    RowMajorMatrix<F> input = RowMajorMatrix<F>::Random(size_t{1} << log_r, 3);
    NaiveBatchFFT<F> naive;
    F shift = F::FromMontgomery(F::Config::kSubgroupGenerator);
    RowMajorMatrix<F> expected(size_t{1} << (log_r + 1), 3);
    naive.CosetLDEBatch(input, 1, shift, expected);
    std::unique_ptr<Radix2EvaluationDomain<F>> domain =
        Radix2EvaluationDomain<F>::Create(size_t{1} << log_r);
    RowMajorMatrix<F> result(size_t{1} << (log_r + 1), 3);
    domain->CosetLDEBatch(std::move(input), 1, shift, result);
    EXPECT_EQ(expected, result);
  }
}

}  // namespace tachyon::math
