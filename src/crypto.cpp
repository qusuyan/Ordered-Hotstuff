/**
 * Copyright 2018 VMware
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hotstuff/crypto.h"

#include "hotstuff/entity.h"

namespace hotstuff {

secp256k1_context_t secp256k1_default_sign_ctx = new Secp256k1Context(true);
secp256k1_context_t secp256k1_default_verify_ctx = new Secp256k1Context(false);

QuorumCertDummy::QuorumCertDummy(const ReplicaConfig &config,
                                 const uint256_t &obj_hash)
    : QuorumCert(obj_hash, config.nreplicas) {}

QuorumCertOrderDummy::QuorumCertOrderDummy(const ReplicaConfig &config,
                                           const uint256_t &obj_hash)
    : QuorumCertOrder(obj_hash, config.nreplicas) {}

const std::vector<uint32_t> QuorumCertOrderDummy::get_order(
    const ReplicaConfig &config, size_t cmd_count) const {
  std::vector<uint32_t> ret;
  if (cmd_count == 0)
    return ret;
    
  size_t edge_votes[cmd_count][cmd_count] = {};
  size_t vote_threshold = config.nreplicas - config.nmajority + 1;  // f + 1

  // create partial ordering graph
  for (const auto &proposed : proposed_order) {
    auto &ordering = proposed.second;
    for (auto it = ordering.begin(); it < ordering.end(); it++)
      for (auto it2 = it + 1; it2 < ordering.end(); it2++)
        edge_votes[*it][*it2]++;
  }

  bool edges[cmd_count][cmd_count] = {};
  for (size_t i = 0; i < cmd_count; i++) {
    for (size_t j = 0; j < cmd_count; j++) {
      if (edge_votes[i][j] >= vote_threshold &&
          (edge_votes[i][j] > edge_votes[j][i] ||
           (edge_votes[i][j] == edge_votes[j][i] && i < j)))
        edges[i][j] = true;
    }
  }

  // find a topological ordering in the edge_votes graph
  bool enqueued[cmd_count] = {true};
  bool dequeued[cmd_count] = {};
  
  uint32_t min_indegree = UINT32_MAX;
  uint32_t elem = 0;
  std::list<uint32_t> to_visit = {};
  for (size_t i = 0; i < cmd_count; i++) {
    uint32_t indegree = 0;
    for (size_t j = 0; j < cmd_count; j++)
      if (edges[j][i]) indegree++;

    if (indegree < min_indegree) {
      min_indegree = indegree;
      elem = i;
    }
  }

  to_visit.push_back(elem);
  enqueued[elem] = true;

  while (to_visit.size() > 0) {
    // find a node with in-degree 0, potentially problemmatic if cycle exists
    min_indegree = UINT32_MAX;
    auto tmp = to_visit.begin();

    for (auto it = to_visit.begin(); it != to_visit.end(); it++) {
      uint32_t indegree = 0;
      for (size_t i = 0; i < cmd_count; i++)
        if (!dequeued[i] && edges[i][*it]) indegree++;
      if (indegree < min_indegree) {
        min_indegree = indegree;
        tmp = it;
      }
    }

    elem = *tmp;
    to_visit.erase(tmp);
    for (size_t i = 0; i < cmd_count; i++) {
      if (!enqueued[i] && edges[elem][i]) {
        enqueued[i] = true;
        to_visit.push_back(i);
      }
    }
    ret.push_back(elem);
    dequeued[elem] = true;
  }

  return ret;
}

QuorumCertSecp256k1::QuorumCertSecp256k1(const ReplicaConfig &config,
                                         const uint256_t &obj_hash)
    : QuorumCert(obj_hash, config.nreplicas) {}

bool QuorumCertSecp256k1::verify(const ReplicaConfig &config) {
  if (sigs.size() < config.nmajority) return false;
  for (size_t i = 0; i < rids.size(); i++)
    if (rids.get(i)) {
      HOTSTUFF_LOG_DEBUG("checking cert(%d), obj_hash=%s", i,
                         get_hex10(obj_hash).c_str());
      if (!sigs[i].verify(
              obj_hash,
              static_cast<const PubKeySecp256k1 &>(config.get_pubkey(i)),
              secp256k1_default_verify_ctx))
        return false;
    }
  return true;
}

promise_t QuorumCertSecp256k1::verify(const ReplicaConfig &config,
                                      VeriPool &vpool) {
  if (sigs.size() < config.nmajority)
    return promise_t([](promise_t &pm) { pm.resolve(false); });
  std::vector<promise_t> vpm;
  for (size_t i = 0; i < rids.size(); i++)
    if (rids.get(i)) {
      HOTSTUFF_LOG_DEBUG("checking cert(%d), obj_hash=%s", i,
                         get_hex10(obj_hash).c_str());
      vpm.push_back(vpool.verify(new Secp256k1VeriTask(
          obj_hash, static_cast<const PubKeySecp256k1 &>(config.get_pubkey(i)),
          sigs[i])));
    }
  return promise::all(vpm).then([](const promise::values_t &values) {
    for (const auto &v : values)
      if (!promise::any_cast<bool>(v)) return false;
    return true;
  });
}

}  // namespace hotstuff
