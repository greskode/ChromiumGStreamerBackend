// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/ssl_status_serialization.h"

#include "base/logging.h"
#include "base/pickle.h"

namespace content {

std::string SerializeSecurityInfo(
    int cert_id,
    net::CertStatus cert_status,
    int security_bits,
    int ssl_connection_status,
    const SignedCertificateTimestampIDStatusList&
        signed_certificate_timestamp_ids) {
  base::Pickle pickle;
  pickle.WriteInt(cert_id);
  pickle.WriteUInt32(cert_status);
  pickle.WriteInt(security_bits);
  pickle.WriteInt(ssl_connection_status);
  pickle.WriteInt(signed_certificate_timestamp_ids.size());
  for (SignedCertificateTimestampIDStatusList::const_iterator iter =
           signed_certificate_timestamp_ids.begin();
       iter != signed_certificate_timestamp_ids.end(); ++iter) {
    pickle.WriteInt(iter->id);
    pickle.WriteUInt16(iter->status);
  }
  return std::string(static_cast<const char*>(pickle.data()), pickle.size());
}

bool DeserializeSecurityInfo(const std::string& state, SSLStatus* ssl_status) {
  *ssl_status = SSLStatus();

  if (state.empty()) {
    // No SSL used.
    return true;
  }

  base::Pickle pickle(state.data(), static_cast<int>(state.size()));
  base::PickleIterator iter(pickle);
  int num_scts_to_read;
  if (!iter.ReadInt(&ssl_status->cert_id) ||
      !iter.ReadUInt32(&ssl_status->cert_status) ||
      !iter.ReadInt(&ssl_status->security_bits) ||
      !iter.ReadInt(&ssl_status->connection_status) ||
      !iter.ReadInt(&num_scts_to_read)) {
    *ssl_status = SSLStatus();
    return false;
  }

  // Sanity check |security_bits|: the only allowed negative value is -1.
  if (ssl_status->security_bits < -1) {
    *ssl_status = SSLStatus();
    return false;
  }

  for (; num_scts_to_read > 0; --num_scts_to_read) {
    int id;
    uint16 status;
    if (!iter.ReadInt(&id) || !iter.ReadUInt16(&status)) {
      *ssl_status = SSLStatus();
      return false;
    }

    ssl_status->signed_certificate_timestamp_ids.push_back(
        SignedCertificateTimestampIDAndStatus(
            id, static_cast<net::ct::SCTVerifyStatus>(status)));
  }

  return true;
}

}  // namespace content
