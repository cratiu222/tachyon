// Copyright (c) 2022 The Plonky3 Authors
// Use of this source code is governed by a MIT/Apache-2.0 style license that
// can be found in the LICENSE-MIT.plonky3 and the LICENCE-APACHE.plonky3
// file.

#ifndef TACHYON_CRYPTO_HASHES_SPONGE_POSEIDON2_POSEIDON2_HORIZEN_EXTERNAL_MATRIX_H_
#define TACHYON_CRYPTO_HASHES_SPONGE_POSEIDON2_POSEIDON2_HORIZEN_EXTERNAL_MATRIX_H_

#include "tachyon/crypto/hashes/sponge/poseidon2/poseidon2_external_matrix.h"

namespace tachyon::crypto {

template <typename F>
class Poseidon2HorizenExternalMatrix final
    : public Poseidon2ExternalMatrix<Poseidon2HorizenExternalMatrix<F>> {
 public:
  template <typename Derived>
  static void DoApply(Eigen::MatrixBase<Derived>& v) {
    F t0 = v(0, 0) + v(1, 0);
    F t1 = v(2, 0) + v(3, 0);
    F t2 = v(1, 0) + v(1, 0) + t1;
    F t3 = v(3, 0) + v(3, 0) + t0;
    v(3, 0) = t1.Double().Double() + t3;
    v(1, 0) = t0.Double().Double() + t2;
    v(0, 0) = t3 + v(1, 0);
    v(2, 0) = t2 + v(3, 0);
  }

  static math::Matrix<F> DoConstruct() {
    return math::Matrix<F>{
        {F(5), F(7), F(1), F(3)},
        {F(4), F(6), F(1), F(1)},
        {F(1), F(3), F(5), F(7)},
        {F(1), F(1), F(4), F(6)},
    };
  }
};

template <typename Field_>
struct Poseidon2ExternalMatrixTraits<Poseidon2HorizenExternalMatrix<Field_>> {
  using Field = Field_;
};

}  // namespace tachyon::crypto

#endif  // TACHYON_CRYPTO_HASHES_SPONGE_POSEIDON2_POSEIDON2_HORIZEN_EXTERNAL_MATRIX_H_
