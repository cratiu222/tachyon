#include "tachyon/c/zk/plonk/halo2/bn254_gwc_prover.h"

#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "tachyon/c/crypto/random/rng.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/fq_type_traits.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/fr_type_traits.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/g1_point_traits.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/g1_point_type_traits.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/g2_point_traits.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/g2_point_type_traits.h"
#include "tachyon/c/math/polynomials/constants.h"
#include "tachyon/c/math/polynomials/univariate/bn254_univariate_dense_polynomial_type_traits.h"
#include "tachyon/c/math/polynomials/univariate/bn254_univariate_evaluation_domain_type_traits.h"
#include "tachyon/c/math/polynomials/univariate/bn254_univariate_evaluations_type_traits.h"
#include "tachyon/c/zk/base/bn254_blinder_type_traits.h"
#include "tachyon/c/zk/plonk/halo2/bn254_transcript.h"
#include "tachyon/c/zk/plonk/halo2/test/bn254_halo2_params_data.h"
#include "tachyon/math/elliptic_curves/bn/bn254/bn254.h"
#include "tachyon/zk/base/commitments/gwc_extension.h"
#include "tachyon/zk/lookup/halo2/scheme.h"
#include "tachyon/zk/plonk/halo2/blake2b_transcript.h"
#include "tachyon/zk/plonk/halo2/poseidon_transcript.h"
#include "tachyon/zk/plonk/halo2/prover.h"
#include "tachyon/zk/plonk/halo2/sha256_transcript.h"
#include "tachyon/zk/plonk/halo2/transcript_type.h"

namespace tachyon::zk::plonk::halo2::bn254 {

class GWCProverTest : public testing::TestWithParam<int> {
 public:
  using PCS = GWCExtension<math::bn254::BN254Curve, c::math::kMaxDegree,
                           c::math::kMaxDegree, math::bn254::G1AffinePoint>;
  using LS = tachyon::zk::lookup::halo2::Scheme<PCS::Poly, PCS::Evals,
                                                PCS::Commitment>;

  void SetUp() override {
    k_ = 5;
    s_ = math::bn254::Fr(2);
    const tachyon_bn254_fr& c_s = c::base::c_cast(s_);
    prover_ = tachyon_halo2_bn254_gwc_prover_create_from_unsafe_setup(
        GetParam(), k_, &c_s);
  }

  void TearDown() override { tachyon_halo2_bn254_gwc_prover_destroy(prover_); }

 protected:
  tachyon_halo2_bn254_gwc_prover* prover_ = nullptr;
  uint32_t k_;
  math::bn254::Fr s_;
};

INSTANTIATE_TEST_SUITE_P(GWCProverTest, GWCProverTest,
                         testing::Values(TACHYON_HALO2_BLAKE2B_TRANSCRIPT,
                                         TACHYON_HALO2_POSEIDON_TRANSCRIPT,
                                         TACHYON_HALO2_SHA256_TRANSCRIPT));

TEST_P(GWCProverTest, Constructor) {
  uint8_t transcript_type = GetParam();

  tachyon_halo2_bn254_gwc_prover* prover_from_halo2_parmas =
      tachyon_halo2_bn254_gwc_prover_create_from_params(
          transcript_type, k_,
          reinterpret_cast<const uint8_t*>(
              c::zk::plonk::halo2::bn254::kExpectedHalo2Params),
          std::size(c::zk::plonk::halo2::bn254::kExpectedHalo2Params));

  const tachyon_bn254_g2_affine* expected_s_g2 =
      tachyon_halo2_bn254_gwc_prover_get_s_g2(prover_);
  const tachyon_bn254_g2_affine* s_g2 =
      tachyon_halo2_bn254_gwc_prover_get_s_g2(prover_from_halo2_parmas);
  ASSERT_TRUE(tachyon_bn254_g2_affine_eq(expected_s_g2, s_g2));
  tachyon_halo2_bn254_gwc_prover_destroy(prover_from_halo2_parmas);
}

TEST_P(GWCProverTest, Getters) {
  EXPECT_EQ(tachyon_halo2_bn254_gwc_prover_get_k(prover_), k_);
  EXPECT_EQ(tachyon_halo2_bn254_gwc_prover_get_n(prover_), size_t{1} << k_);
  EXPECT_EQ(tachyon_halo2_bn254_gwc_prover_get_blinder(prover_),
            c::base::c_cast(
                &(reinterpret_cast<Prover<PCS, LS>*>(prover_)->blinder())));
  EXPECT_EQ(
      tachyon_halo2_bn254_gwc_prover_get_domain(prover_),
      c::base::c_cast(reinterpret_cast<Prover<PCS, LS>*>(prover_)->domain()));

  // NOTE(dongchangYoo): |expected_s_g2| can be generated by doubling
  // g2-generator since |s| equals to 2.
  tachyon_bn254_g2_affine expected_gen = tachyon_bn254_g2_affine_generator();
  tachyon_bn254_g2_jacobian expected_s_g2_jacob =
      tachyon_bn254_g2_affine_dbl(&expected_gen);
  const tachyon_bn254_g2_affine* s_g2_affine =
      tachyon_halo2_bn254_gwc_prover_get_s_g2(prover_);
  EXPECT_EQ(c::base::native_cast(*s_g2_affine),
            c::base::native_cast(expected_s_g2_jacob).ToAffine());
}

TEST_P(GWCProverTest, Commit) {
  PCS::Domain::DensePoly poly = PCS::Domain::DensePoly::Random(5);
  tachyon_bn254_g1_jacobian* point =
      tachyon_halo2_bn254_gwc_prover_commit(prover_, c::base::c_cast(&poly));
  EXPECT_EQ(
      (reinterpret_cast<Prover<PCS, LS>*>(prover_)->Commit(poly).ToJacobian()),
      c::base::native_cast(*point));
}

TEST_P(GWCProverTest, CommitLagrange) {
  PCS::Domain::Evals evals = PCS::Domain::Evals::Random(5);
  tachyon_bn254_g1_jacobian* point =
      tachyon_halo2_bn254_gwc_prover_commit_lagrange(prover_,
                                                     c::base::c_cast(&evals));
  EXPECT_EQ(
      (reinterpret_cast<Prover<PCS, LS>*>(prover_)->Commit(evals).ToJacobian()),
      c::base::native_cast(*point));
}

TEST_P(GWCProverTest, SetRng) {
  std::vector<uint8_t> seed = base::CreateVector(
      crypto::XORShiftRNG::kSeedSize,
      []() { return base::Uniform(base::Range<uint8_t>()); });
  tachyon_rng* rng = tachyon_rng_create_from_seed(TACHYON_RNG_XOR_SHIFT,
                                                  seed.data(), seed.size());
  uint8_t state[crypto::XORShiftRNG::kStateSize];
  size_t state_len;
  tachyon_rng_get_state(rng, state, &state_len);
  tachyon_halo2_bn254_gwc_prover_set_rng_state(prover_, state, state_len);

  auto cpp_rng = std::make_unique<crypto::XORShiftRNG>(
      crypto::XORShiftRNG::FromSeed(seed));
  auto cpp_generator =
      std::make_unique<RandomFieldGenerator<math::bn254::Fr>>(cpp_rng.get());

  EXPECT_EQ((reinterpret_cast<Prover<PCS, LS>*>(prover_)->blinder().Generate()),
            cpp_generator->Generate());

  tachyon_rng_destroy(rng);
}

TEST_P(GWCProverTest, SetTranscript) {
  uint8_t transcript_type = GetParam();

  tachyon_halo2_bn254_transcript_writer* transcript =
      tachyon_halo2_bn254_transcript_writer_create(transcript_type);

  size_t digest_len = 0;
  size_t state_len = 0;
  switch (static_cast<TranscriptType>(transcript_type)) {
    case TranscriptType::kBlake2b: {
      Blake2bWriter<math::bn254::G1AffinePoint>* blake2b =
          reinterpret_cast<Blake2bWriter<math::bn254::G1AffinePoint>*>(
              transcript->extra);
      digest_len = blake2b->GetDigestLen();
      state_len = blake2b->GetStateLen();
      break;
    }
    case TranscriptType::kPoseidon: {
      PoseidonWriter<math::bn254::G1AffinePoint>* poseidon =
          reinterpret_cast<PoseidonWriter<math::bn254::G1AffinePoint>*>(
              transcript->extra);
      digest_len = poseidon->GetDigestLen();
      state_len = poseidon->GetStateLen();
      break;
    }
    case TranscriptType::kSha256: {
      Sha256Writer<math::bn254::G1AffinePoint>* sha256 =
          reinterpret_cast<Sha256Writer<math::bn254::G1AffinePoint>*>(
              transcript->extra);
      digest_len = sha256->GetDigestLen();
      state_len = sha256->GetStateLen();
      break;
    }
  }

  std::vector<uint8_t> data = base::CreateVector(
      digest_len, []() { return base::Uniform(base::Range<uint8_t>()); });
  tachyon_halo2_bn254_transcript_writer_update(transcript, data.data(),
                                               data.size());

  if (static_cast<TranscriptType>(transcript_type) ==
      TranscriptType::kPoseidon) {
    PoseidonWriter<math::bn254::G1AffinePoint>* poseidon =
        reinterpret_cast<PoseidonWriter<math::bn254::G1AffinePoint>*>(
            transcript->extra);
    state_len = poseidon->GetStateLen();
  }

  std::vector<uint8_t> state(state_len);
  tachyon_halo2_bn254_transcript_writer_get_state(transcript, state.data(),
                                                  &state_len);
  tachyon_halo2_bn254_gwc_prover_set_transcript_state(prover_, state.data(),
                                                      state_len);

  EXPECT_EQ(
      (reinterpret_cast<Prover<PCS, LS>*>(prover_)
           ->transcript()
           ->SqueezeChallenge()),
      reinterpret_cast<crypto::TranscriptWriter<math::bn254::G1AffinePoint>*>(
          transcript->extra)
          ->SqueezeChallenge());

  tachyon_halo2_bn254_transcript_writer_destroy(transcript);
}

}  // namespace tachyon::zk::plonk::halo2::bn254
