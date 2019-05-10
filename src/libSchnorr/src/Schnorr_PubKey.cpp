/*
 * Copyright (C) 2019 Zilliqa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/err.h>

#include "Schnorr.h"
#include "SchnorrInternal.h"

using namespace std;

// ============================================================================
// Construction
// ============================================================================

bool PubKey::constructPreChecks() { return (m_P != nullptr); }

PubKey::PubKey()
    : m_P(EC_POINT_new(Schnorr::GetCurveGroup()), EC_POINT_clear_free) {
  if (!constructPreChecks()) {
    throw std::bad_alloc();
  }
}

PubKey::PubKey(const PrivKey& privkey)
    : m_P(EC_POINT_new(Schnorr::GetCurveGroup()), EC_POINT_clear_free) {
  if (!constructPreChecks()) {
    throw std::bad_alloc();
  }

  if (BN_is_zero(privkey.m_d.get()) ||
      (BN_cmp(privkey.m_d.get(), Schnorr::GetCurveOrder()) != -1)) {
    // Input private key is invalid
    return;
  }
  if (EC_POINT_mul(Schnorr::GetCurveGroup(), m_P.get(), privkey.m_d.get(), NULL,
                   NULL, NULL) == 0) {
    // Public key generation failed
    return;
  }
}

PubKey::PubKey(const bytes& src, unsigned int offset)
    : m_P(EC_POINT_new(Schnorr::GetCurveGroup()), EC_POINT_clear_free) {
  if (!constructPreChecks()) {
    throw std::bad_alloc();
  }

  if (!Deserialize(src, offset)) {
    //
  }
}

PubKey::PubKey(const PubKey& src)
    : m_P(EC_POINT_new(Schnorr::GetCurveGroup()), EC_POINT_clear_free) {
  if (!constructPreChecks()) {
    throw std::bad_alloc();
  }

  if (!EC_POINT_copy(m_P.get(), src.m_P.get())) {
    //
  }
}

PubKey::~PubKey() {}

// ============================================================================
// Serialization
// ============================================================================

bool PubKey::Serialize(bytes& dst, unsigned int offset) const {
  ECPOINTSerialize::SetNumber(dst, offset, PUB_KEY_SIZE, m_P);
  return true;
}

bool PubKey::Deserialize(const bytes& src, unsigned int offset) {
  shared_ptr<EC_POINT> result =
      ECPOINTSerialize::GetNumber(src, offset, PUB_KEY_SIZE);

  if (result == nullptr) {
    return false;
  }

  if (!EC_POINT_copy(m_P.get(), result.get())) {
    return false;
  }

  return true;
}

// ============================================================================
// Assignment and Comparison
// ============================================================================

PubKey& PubKey::operator=(const PubKey& src) {
  if (!EC_POINT_copy(m_P.get(), src.m_P.get())) {
    //
  }
  return *this;
}

bool PubKey::comparePreChecks(const PubKey& r, shared_ptr<BIGNUM>& lhs_bnvalue,
                              shared_ptr<BIGNUM>& rhs_bnvalue) const {
  unique_ptr<BN_CTX, void (*)(BN_CTX*)> ctx(BN_CTX_new(), BN_CTX_free);
  if (ctx == nullptr) {
    throw std::bad_alloc();
  }

  lhs_bnvalue.reset(
      EC_POINT_point2bn(Schnorr::GetCurveGroup(), m_P.get(),
                        POINT_CONVERSION_COMPRESSED, NULL, ctx.get()),
      BN_clear_free);
  rhs_bnvalue.reset(
      EC_POINT_point2bn(Schnorr::GetCurveGroup(), r.m_P.get(),
                        POINT_CONVERSION_COMPRESSED, NULL, ctx.get()),
      BN_clear_free);

  if ((lhs_bnvalue == nullptr) || (rhs_bnvalue == nullptr)) {
    throw std::bad_alloc();
  }

  return true;
}

bool PubKey::operator<(const PubKey& r) const {
  shared_ptr<BIGNUM> lhs_bnvalue, rhs_bnvalue;
  return comparePreChecks(r, lhs_bnvalue, rhs_bnvalue) &&
         BN_cmp(lhs_bnvalue.get(), rhs_bnvalue.get()) == -1;
}

bool PubKey::operator>(const PubKey& r) const { return r < *this; }

bool PubKey::operator==(const PubKey& r) const {
  shared_ptr<BIGNUM> lhs_bnvalue, rhs_bnvalue;
  return comparePreChecks(r, lhs_bnvalue, rhs_bnvalue) &&
         BN_cmp(lhs_bnvalue.get(), rhs_bnvalue.get()) == 0;
}

PubKey::operator std::string() const {
  std::string output;
  if (!SerializableCryptoToHexStr(*this, output)) {
    return "";
  }
  return "0x" + output;
}

size_t hash<PubKey>::operator()(PubKey const& pubKey) const noexcept {
  std::size_t seed = 0;
  std::string pubKeyStr;
  if (!SerializableCryptoToHexStr(pubKey, pubKeyStr)) {
    return seed;
  }
  boost::hash_combine(seed, pubKeyStr);
  return seed;
}

std::ostream& operator<<(std::ostream& os, const PubKey& p) {
  std::string output;
  if (!SerializableCryptoToHexStr(p, output)) {
    os << "";
  } else {
    os << "0x" << output;
  }
  return os;
}