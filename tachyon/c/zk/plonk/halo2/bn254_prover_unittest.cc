#include "tachyon/c/zk/plonk/halo2/bn254_prover.h"

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
#include "tachyon/c/math/polynomials/univariate/bn254_univariate_dense_polynomial_type_traits.h"
#include "tachyon/c/math/polynomials/univariate/bn254_univariate_evaluation_domain_type_traits.h"
#include "tachyon/c/math/polynomials/univariate/bn254_univariate_evaluations_type_traits.h"
#include "tachyon/c/zk/base/bn254_blinder_type_traits.h"
#include "tachyon/c/zk/plonk/halo2/bn254_ps.h"
#include "tachyon/c/zk/plonk/halo2/bn254_transcript.h"
#include "tachyon/c/zk/plonk/halo2/kzg_family_prover_impl.h"
#include "tachyon/c/zk/plonk/halo2/test/bn254_halo2_params_data.h"
#include "tachyon/crypto/random/cha_cha20/cha_cha20_rng.h"
#include "tachyon/crypto/random/rng_type.h"
#include "tachyon/crypto/random/xor_shift/xor_shift_rng.h"
#include "tachyon/math/elliptic_curves/bn/bn254/bn254.h"
#include "tachyon/zk/plonk/halo2/blake2b_transcript.h"
#include "tachyon/zk/plonk/halo2/pcs_type.h"
#include "tachyon/zk/plonk/halo2/poseidon_transcript.h"
#include "tachyon/zk/plonk/halo2/sha256_transcript.h"
#include "tachyon/zk/plonk/halo2/snark_verifier_poseidon_transcript.h"
#include "tachyon/zk/plonk/halo2/transcript_type.h"
#include "tachyon/zk/plonk/halo2/vendor.h"

namespace tachyon::zk::plonk::halo2::bn254 {

namespace {

using GWCPCS = c::zk::plonk::halo2::bn254::GWCPCS;
using SHPlonkPCS = c::zk::plonk::halo2::bn254::SHPlonkPCS;
using PSEGWC = c::zk::plonk::halo2::bn254::PSEGWC;
using PSESHPlonk = c::zk::plonk::halo2::bn254::PSESHPlonk;
using ScrollGWC = c::zk::plonk::halo2::bn254::ScrollGWC;
using ScrollSHPlonk = c::zk::plonk::halo2::bn254::ScrollSHPlonk;

template <typename PS>
using ProverImpl = c::zk::plonk::halo2::KZGFamilyProverImpl<PS>;

struct Param {
  uint8_t vendor;
  uint8_t pcs_type;
  uint8_t transcript_type;

  Param(uint8_t vendor, uint8_t pcs_type, uint8_t transcript_type)
      : vendor(vendor), pcs_type(pcs_type), transcript_type(transcript_type) {}
};

class ProverTest : public testing::TestWithParam<Param> {
 public:
  void SetUp() override {
    Param param = GetParam();

    k_ = 5;
    s_ = math::bn254::Fr(2);
    const tachyon_bn254_fr& c_s = c::base::c_cast(s_);
    prover_ = tachyon_halo2_bn254_prover_create_from_unsafe_setup(
        param.vendor, param.pcs_type, param.transcript_type, k_, &c_s);
  }

  void TearDown() override { tachyon_halo2_bn254_prover_destroy(prover_); }

 protected:
  tachyon_halo2_bn254_prover* prover_ = nullptr;
  uint32_t k_;
  math::bn254::Fr s_;
};

}  // namespace

INSTANTIATE_TEST_SUITE_P(
    ProverTest, ProverTest,
    testing::Values(
        Param(TACHYON_HALO2_PSE_VENDOR, TACHYON_HALO2_GWC_PCS,
              TACHYON_HALO2_BLAKE2B_TRANSCRIPT),
        Param(TACHYON_HALO2_PSE_VENDOR, TACHYON_HALO2_GWC_PCS,
              TACHYON_HALO2_POSEIDON_TRANSCRIPT),
        Param(TACHYON_HALO2_PSE_VENDOR, TACHYON_HALO2_GWC_PCS,
              TACHYON_HALO2_SHA256_TRANSCRIPT),
        Param(TACHYON_HALO2_PSE_VENDOR, TACHYON_HALO2_GWC_PCS,
              TACHYON_HALO2_SNARK_VERIFIER_POSEIDON_TRANSCRIPT),
        Param(TACHYON_HALO2_SCROLL_VENDOR, TACHYON_HALO2_GWC_PCS,
              TACHYON_HALO2_BLAKE2B_TRANSCRIPT),
        Param(TACHYON_HALO2_SCROLL_VENDOR, TACHYON_HALO2_GWC_PCS,
              TACHYON_HALO2_POSEIDON_TRANSCRIPT),
        Param(TACHYON_HALO2_SCROLL_VENDOR, TACHYON_HALO2_GWC_PCS,
              TACHYON_HALO2_SHA256_TRANSCRIPT),
        Param(TACHYON_HALO2_SCROLL_VENDOR, TACHYON_HALO2_GWC_PCS,
              TACHYON_HALO2_SNARK_VERIFIER_POSEIDON_TRANSCRIPT),
        Param(TACHYON_HALO2_PSE_VENDOR, TACHYON_HALO2_SHPLONK_PCS,
              TACHYON_HALO2_BLAKE2B_TRANSCRIPT),
        Param(TACHYON_HALO2_PSE_VENDOR, TACHYON_HALO2_SHPLONK_PCS,
              TACHYON_HALO2_POSEIDON_TRANSCRIPT),
        Param(TACHYON_HALO2_PSE_VENDOR, TACHYON_HALO2_SHPLONK_PCS,
              TACHYON_HALO2_SHA256_TRANSCRIPT),
        Param(TACHYON_HALO2_PSE_VENDOR, TACHYON_HALO2_SHPLONK_PCS,
              TACHYON_HALO2_SNARK_VERIFIER_POSEIDON_TRANSCRIPT),
        Param(TACHYON_HALO2_SCROLL_VENDOR, TACHYON_HALO2_SHPLONK_PCS,
              TACHYON_HALO2_BLAKE2B_TRANSCRIPT),
        Param(TACHYON_HALO2_SCROLL_VENDOR, TACHYON_HALO2_SHPLONK_PCS,
              TACHYON_HALO2_POSEIDON_TRANSCRIPT),
        Param(TACHYON_HALO2_SCROLL_VENDOR, TACHYON_HALO2_SHPLONK_PCS,
              TACHYON_HALO2_SHA256_TRANSCRIPT),
        Param(TACHYON_HALO2_SCROLL_VENDOR, TACHYON_HALO2_SHPLONK_PCS,
              TACHYON_HALO2_SNARK_VERIFIER_POSEIDON_TRANSCRIPT)));

TEST_P(ProverTest, Constructor) {
  Param param = GetParam();

  tachyon_halo2_bn254_prover* prover_from_halo2_parmas =
      tachyon_halo2_bn254_prover_create_from_params(
          param.vendor, param.pcs_type, param.transcript_type, k_,
          reinterpret_cast<const uint8_t*>(
              c::zk::plonk::halo2::bn254::kExpectedHalo2Params),
          std::size(c::zk::plonk::halo2::bn254::kExpectedHalo2Params));

  const tachyon_bn254_g2_affine* expected_s_g2 =
      tachyon_halo2_bn254_prover_get_s_g2(prover_);
  const tachyon_bn254_g2_affine* s_g2 =
      tachyon_halo2_bn254_prover_get_s_g2(prover_from_halo2_parmas);
  ASSERT_TRUE(tachyon_bn254_g2_affine_eq(expected_s_g2, s_g2));
  tachyon_halo2_bn254_prover_destroy(prover_from_halo2_parmas);
}

TEST_P(ProverTest, Getters) {
  Param param = GetParam();
  if (param.vendor != TACHYON_HALO2_PSE_VENDOR ||
      param.transcript_type != TACHYON_HALO2_BLAKE2B_TRANSCRIPT) {
    GTEST_SKIP() << "Getters test is not related to vendor and transcript_type";
  }

  EXPECT_EQ(tachyon_halo2_bn254_prover_get_k(prover_), k_);
  EXPECT_EQ(tachyon_halo2_bn254_prover_get_n(prover_), size_t{1} << k_);
  tachyon_bn254_blinder* blinder =
      tachyon_halo2_bn254_prover_get_blinder(prover_);
  const tachyon_bn254_univariate_evaluation_domain* domain =
      tachyon_halo2_bn254_prover_get_domain(prover_);
  switch (static_cast<PCSType>(prover_->pcs_type)) {
    case PCSType::kGWC: {
      EXPECT_EQ(blinder,
                &c::base::c_cast(
                    reinterpret_cast<zk::ProverBase<GWCPCS>*>(prover_->extra)
                        ->blinder()));
      EXPECT_EQ(
          domain,
          c::base::c_cast(
              reinterpret_cast<zk::Entity<GWCPCS>*>(prover_->extra)->domain()));
      break;
    }
    case PCSType::kSHPlonk: {
      EXPECT_EQ(blinder,
                &c::base::c_cast(reinterpret_cast<zk::ProverBase<SHPlonkPCS>*>(
                                     prover_->extra)
                                     ->blinder()));
      EXPECT_EQ(domain,
                c::base::c_cast(
                    reinterpret_cast<zk::Entity<SHPlonkPCS>*>(prover_->extra)
                        ->domain()));
      break;
    }
  }

  // NOTE(dongchangYoo): |expected_s_g2| can be generated by doubling
  // g2-generator since |s| equals to 2.
  tachyon_bn254_g2_affine expected_gen = tachyon_bn254_g2_affine_generator();
  tachyon_bn254_g2_jacobian expected_s_g2_jacob =
      tachyon_bn254_g2_affine_dbl(&expected_gen);
  const tachyon_bn254_g2_affine* s_g2_affine =
      tachyon_halo2_bn254_prover_get_s_g2(prover_);
  EXPECT_EQ(c::base::native_cast(*s_g2_affine),
            c::base::native_cast(expected_s_g2_jacob).ToAffine());
}

TEST_P(ProverTest, Commit) {
  Param param = GetParam();
  if (param.transcript_type != TACHYON_HALO2_BLAKE2B_TRANSCRIPT) {
    GTEST_SKIP() << "Commit test is not related to transcript type";
  }

  using Poly =
      math::UnivariateDensePolynomial<math::bn254::Fr, c::math::kMaxDegree>;
  Poly poly = Poly::Random(5);

  tachyon_bn254_g1_projective* point =
      tachyon_halo2_bn254_prover_commit(prover_, c::base::c_cast(&poly));

  math::bn254::G1ProjectivePoint expected;
  switch (static_cast<Vendor>(prover_->vendor)) {
    case Vendor::kPSE: {
      switch (static_cast<PCSType>(prover_->pcs_type)) {
        case PCSType::kGWC: {
          expected = reinterpret_cast<ProverImpl<PSEGWC>*>(prover_->extra)
                         ->Commit(poly)
                         .ToProjective();
          break;
        }
        case PCSType::kSHPlonk: {
          expected = reinterpret_cast<ProverImpl<PSESHPlonk>*>(prover_->extra)
                         ->Commit(poly)
                         .ToProjective();
          break;
        }
      }
      break;
    }
    case Vendor::kScroll: {
      switch (static_cast<PCSType>(prover_->pcs_type)) {
        case PCSType::kGWC: {
          expected = reinterpret_cast<ProverImpl<ScrollGWC>*>(prover_->extra)
                         ->Commit(poly)
                         .ToProjective();
          break;
        }
        case PCSType::kSHPlonk: {
          expected =
              reinterpret_cast<ProverImpl<ScrollSHPlonk>*>(prover_->extra)
                  ->Commit(poly)
                  .ToProjective();
          break;
        }
      }
      break;
    }
  }

  EXPECT_EQ(c::base::native_cast(*point), expected);
}

TEST_P(ProverTest, CommitLagrange) {
  Param param = GetParam();
  if (param.transcript_type != TACHYON_HALO2_BLAKE2B_TRANSCRIPT) {
    GTEST_SKIP() << "CommitLagrange test is not related to transcript type";
  }

  using Evals =
      math::UnivariateEvaluations<math::bn254::Fr, c::math::kMaxDegree>;
  Evals evals = Evals::Random(5);

  tachyon_bn254_g1_projective* point =
      tachyon_halo2_bn254_prover_commit_lagrange(prover_,
                                                 c::base::c_cast(&evals));

  math::bn254::G1ProjectivePoint expected;
  switch (static_cast<Vendor>(prover_->vendor)) {
    case Vendor::kPSE: {
      switch (static_cast<PCSType>(prover_->pcs_type)) {
        case PCSType::kGWC: {
          expected = reinterpret_cast<ProverImpl<PSEGWC>*>(prover_->extra)
                         ->Commit(evals)
                         .ToProjective();
          break;
        }
        case PCSType::kSHPlonk: {
          expected = reinterpret_cast<ProverImpl<PSESHPlonk>*>(prover_->extra)
                         ->Commit(evals)
                         .ToProjective();
          break;
        }
      }
      break;
    }
    case Vendor::kScroll: {
      switch (static_cast<PCSType>(prover_->pcs_type)) {
        case PCSType::kGWC: {
          expected = reinterpret_cast<ProverImpl<ScrollGWC>*>(prover_->extra)
                         ->Commit(evals)
                         .ToProjective();
          break;
        }
        case PCSType::kSHPlonk: {
          expected =
              reinterpret_cast<ProverImpl<ScrollSHPlonk>*>(prover_->extra)
                  ->Commit(evals)
                  .ToProjective();
          break;
        }
      }
      break;
    }
  }

  EXPECT_EQ(c::base::native_cast(*point), expected);
}

TEST_P(ProverTest, BatchCommit) {
  Param param = GetParam();
  if (param.transcript_type != TACHYON_HALO2_BLAKE2B_TRANSCRIPT) {
    GTEST_SKIP() << "BatchCommit test is not related to transcript type";
  }

  using Poly =
      math::UnivariateDensePolynomial<math::bn254::Fr, c::math::kMaxDegree>;
  std::vector<Poly> polys =
      base::CreateVector(4, []() { return Poly::Random(5); });

  tachyon_halo2_bn254_prover_batch_start(prover_, polys.size());
  for (size_t i = 0; i < polys.size(); ++i) {
    tachyon_halo2_bn254_prover_batch_commit(prover_, c::base::c_cast(&polys[i]),
                                            i);
  }

  std::vector<math::bn254::G1AffinePoint> points(polys.size());
  tachyon_halo2_bn254_prover_batch_end(
      prover_,
      const_cast<tachyon_bn254_g1_affine*>(c::base::c_cast(points.data())),
      points.size());

  for (size_t i = 0; i < points.size(); ++i) {
    const Poly& poly = polys[i];
    math::bn254::G1AffinePoint expected;
    switch (static_cast<Vendor>(prover_->vendor)) {
      case Vendor::kPSE: {
        switch (static_cast<PCSType>(prover_->pcs_type)) {
          case PCSType::kGWC: {
            expected = reinterpret_cast<ProverImpl<PSEGWC>*>(prover_->extra)
                           ->Commit(poly);
            break;
          }
          case PCSType::kSHPlonk: {
            expected = reinterpret_cast<ProverImpl<PSESHPlonk>*>(prover_->extra)
                           ->Commit(poly);
            break;
          }
        }
        break;
      }
      case Vendor::kScroll: {
        switch (static_cast<PCSType>(prover_->pcs_type)) {
          case PCSType::kGWC: {
            expected = reinterpret_cast<ProverImpl<ScrollGWC>*>(prover_->extra)
                           ->Commit(poly);
            break;
          }
          case PCSType::kSHPlonk: {
            expected =
                reinterpret_cast<ProverImpl<ScrollSHPlonk>*>(prover_->extra)
                    ->Commit(poly);
            break;
          }
        }
        break;
      }
    }
    EXPECT_EQ(points[i], expected);
  }
}

TEST_P(ProverTest, BatchCommitLagrange) {
  Param param = GetParam();
  if (param.transcript_type != TACHYON_HALO2_BLAKE2B_TRANSCRIPT) {
    GTEST_SKIP()
        << "BatchCommitLagrange test is not related to transcript type";
  }

  using Evals =
      math::UnivariateEvaluations<math::bn254::Fr, c::math::kMaxDegree>;
  std::vector<Evals> evals_vec =
      base::CreateVector(4, []() { return Evals::Random(5); });

  tachyon_halo2_bn254_prover_batch_start(prover_, evals_vec.size());
  for (size_t i = 0; i < evals_vec.size(); ++i) {
    tachyon_halo2_bn254_prover_batch_commit_lagrange(
        prover_, c::base::c_cast(&evals_vec[i]), i);
  }

  std::vector<math::bn254::G1AffinePoint> points(evals_vec.size());
  tachyon_halo2_bn254_prover_batch_end(
      prover_,
      const_cast<tachyon_bn254_g1_affine*>(c::base::c_cast(points.data())),
      points.size());

  for (size_t i = 0; i < points.size(); ++i) {
    const Evals& evals = evals_vec[i];
    math::bn254::G1AffinePoint expected;
    switch (static_cast<Vendor>(prover_->vendor)) {
      case Vendor::kPSE: {
        switch (static_cast<PCSType>(prover_->pcs_type)) {
          case PCSType::kGWC: {
            expected = reinterpret_cast<ProverImpl<PSEGWC>*>(prover_->extra)
                           ->Commit(evals);
            break;
          }
          case PCSType::kSHPlonk: {
            expected = reinterpret_cast<ProverImpl<PSESHPlonk>*>(prover_->extra)
                           ->Commit(evals);
            break;
          }
        }
        break;
      }
      case Vendor::kScroll: {
        switch (static_cast<PCSType>(prover_->pcs_type)) {
          case PCSType::kGWC: {
            expected = reinterpret_cast<ProverImpl<ScrollGWC>*>(prover_->extra)
                           ->Commit(evals);
            break;
          }
          case PCSType::kSHPlonk: {
            expected =
                reinterpret_cast<ProverImpl<ScrollSHPlonk>*>(prover_->extra)
                    ->Commit(evals);
            break;
          }
        }
        break;
      }
    }
    EXPECT_EQ(points[i], expected);
  }
}

template <typename RNG>
void SetRngTestImpl(tachyon_halo2_bn254_prover* prover, uint8_t rng_type) {
  std::vector<uint8_t> seed = base::CreateVector(
      RNG::kSeedSize, []() { return base::Uniform(base::Range<uint8_t>()); });
  tachyon_rng* rng =
      tachyon_rng_create_from_seed(rng_type, seed.data(), seed.size());
  uint8_t state[RNG::kStateSize];
  size_t state_len;
  tachyon_rng_get_state(rng, state, &state_len);
  tachyon_halo2_bn254_prover_set_rng_state(prover, rng_type, state, state_len);

  auto cpp_rng = std::make_unique<RNG>();
  ASSERT_TRUE(cpp_rng->SetSeed(seed));
  auto cpp_generator =
      std::make_unique<RandomFieldGenerator<math::bn254::Fr>>(cpp_rng.get());

  math::bn254::Fr expected;
  switch (static_cast<PCSType>(prover->pcs_type)) {
    case PCSType::kGWC: {
      expected = reinterpret_cast<zk::ProverBase<GWCPCS>*>(prover->extra)
                     ->blinder()
                     .Generate();
      break;
    }
    case PCSType::kSHPlonk: {
      expected = reinterpret_cast<zk::ProverBase<SHPlonkPCS>*>(prover->extra)
                     ->blinder()
                     .Generate();
      break;
    }
  }

  EXPECT_EQ(cpp_generator->Generate(), expected);

  tachyon_rng_destroy(rng);
}

TEST_P(ProverTest, SetRng) {
  Param param = GetParam();
  if (param.vendor != TACHYON_HALO2_PSE_VENDOR ||
      param.transcript_type != TACHYON_HALO2_BLAKE2B_TRANSCRIPT) {
    GTEST_SKIP() << "SetRng test is not related to vendor and transcript_type";
  }

  SetRngTestImpl<crypto::XORShiftRNG>(prover_, TACHYON_RNG_XOR_SHIFT);
  SetRngTestImpl<crypto::ChaCha20RNG>(prover_, TACHYON_RNG_CHA_CHA20);
}

TEST_P(ProverTest, SetTranscript) {
  Param param = GetParam();
  if (param.vendor != TACHYON_HALO2_PSE_VENDOR) {
    GTEST_SKIP() << "SetRng test is not related to vendor";
  }

  uint8_t transcript_type = param.transcript_type;

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
      // NOTE(chokobole): In case of Poseidon transcript,
      // |tachyon_halo2_bn254_transcript_writer_update()| touches an internal
      // member |absorbing_|, so |state_len| has to be updated after this
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
    case TranscriptType::kSnarkVerifierPoseidon: {
      SnarkVerifierPoseidonWriter<math::bn254::G1AffinePoint>* sv_poseidon =
          reinterpret_cast<
              SnarkVerifierPoseidonWriter<math::bn254::G1AffinePoint>*>(
              transcript->extra);
      digest_len = sv_poseidon->GetDigestLen();
      // NOTE(chokobole): In case of SnarkVerifierPoseidon transcript,
      // |tachyon_halo2_bn254_transcript_writer_update()| touches an internal
      // member |buf_|, so |state_len| has to be updated after this
      break;
    }
  }

  std::vector<uint8_t> data = base::CreateVector(
      digest_len, []() { return base::Uniform(base::Range<uint8_t>()); });
  tachyon_halo2_bn254_transcript_writer_update(transcript, data.data(),
                                               data.size());

  switch (static_cast<TranscriptType>(transcript_type)) {
    case TranscriptType::kPoseidon: {
      PoseidonWriter<math::bn254::G1AffinePoint>* poseidon =
          reinterpret_cast<PoseidonWriter<math::bn254::G1AffinePoint>*>(
              transcript->extra);
      state_len = poseidon->GetStateLen();
      break;
    }
    case TranscriptType::kSnarkVerifierPoseidon: {
      SnarkVerifierPoseidonWriter<math::bn254::G1AffinePoint>* sv_poseidon =
          reinterpret_cast<
              SnarkVerifierPoseidonWriter<math::bn254::G1AffinePoint>*>(
              transcript->extra);
      state_len = sv_poseidon->GetStateLen();
      break;
    }
    case TranscriptType::kBlake2b:
    case TranscriptType::kSha256:
      break;
  }

  std::vector<uint8_t> state(state_len);
  tachyon_halo2_bn254_transcript_writer_get_state(transcript, state.data(),
                                                  &state_len);
  tachyon_halo2_bn254_prover_set_transcript_state(prover_, state.data(),
                                                  state_len);

  math::bn254::Fr expected;
  switch (static_cast<PCSType>(prover_->pcs_type)) {
    case PCSType::kGWC: {
      expected = reinterpret_cast<zk::Entity<GWCPCS>*>(prover_->extra)
                     ->transcript()
                     ->SqueezeChallenge();
      break;
    }
    case PCSType::kSHPlonk: {
      expected = reinterpret_cast<zk::Entity<SHPlonkPCS>*>(prover_->extra)
                     ->transcript()
                     ->SqueezeChallenge();
      break;
    }
  }

  EXPECT_EQ(
      reinterpret_cast<crypto::TranscriptWriter<math::bn254::G1AffinePoint>*>(
          transcript->extra)
          ->SqueezeChallenge(),
      expected);

  tachyon_halo2_bn254_transcript_writer_destroy(transcript);
}

}  // namespace tachyon::zk::plonk::halo2::bn254
