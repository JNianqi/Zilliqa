/*
 * Copyright (c) 2018 Zilliqa
 * This source code is being disclosed to you solely for the purpose of your
 * participation in testing Zilliqa. You may view, compile and run the code for
 * that purpose and pursuant to the protocols and algorithms that are programmed
 * into, and intended by, the code. You may not do anything else with the code
 * without express permission from Zilliqa Research Pte. Ltd., including
 * modifying or publishing the code (or any part of it), and developing or
 * forming another public or private blockchain network. This source code is
 * provided 'as is' and no warranties are given as to title or non-infringement,
 * merchantability or fitness for purpose and, to the extent permitted by law,
 * all liability for your use of the code is disclaimed. Some programs in this
 * code are governed by the GNU General Public License v3.0 (available at
 * https://www.gnu.org/licenses/gpl-3.0.en.html) ('GPLv3'). The programs that
 * are governed by GPLv3.0 are those programs that are located in the folders
 * src/depends and tests/depends and which include a reference to GPLv3 in their
 * program files.
 */

#include "DataSender.h"

#include "libCrypto/Sha2.h"
#include "libNetwork/P2PComm.h"
#include "libUtils/DataConversion.h"
#include "libUtils/Logger.h"

using namespace std;

void SendDataToLookupNodesDefault(const VectorOfLookupNode& lookups,
                                  const vector<unsigned char>& message) {
  if (LOOKUP_NODE_MODE) {
    LOG_GENERAL(WARNING,
                "DataSender::SendDataToLookupNodesDefault not "
                "expected to be called from LookUp node.");
  }
  LOG_MARKER();

  // TODO: provide interface in P2PComm instead of repopulating the lookup into
  // vector of Peer
  vector<Peer> allLookupNodes;

  for (const auto& node : lookups) {
    LOG_GENERAL(INFO, "Sending msg to lookup node "
                          << node.second.GetPrintableIPAddress() << ":"
                          << node.second.m_listenPortHost);

    allLookupNodes.emplace_back(node.second);
  }

  P2PComm::GetInstance().SendBroadcastMessage(allLookupNodes, message);
}

void SendDataToShardNodesDefault(const vector<unsigned char>& message,
                                 const DequeOfShard& shards,
                                 const unsigned int& my_shards_lo,
                                 const unsigned int& my_shards_hi) {
  if (LOOKUP_NODE_MODE) {
    LOG_GENERAL(WARNING,
                "DataSender::SendDataToShardNodesDefault not expected to "
                "be called from LookUp node.");
    return;
  }
  // Too few target shards - avoid asking all DS clusters to send
  LOG_MARKER();

  auto p = shards.begin();
  advance(p, my_shards_lo);

  for (unsigned int i = my_shards_lo; i <= my_shards_hi; i++) {
    if (BROADCAST_TREEBASED_CLUSTER_MODE) {
      // Choose N other Shard nodes to be recipient of block
      std::vector<Peer> shardBlockReceivers;

      SHA2<HASH_TYPE::HASH_VARIANT_256> sha256;
      sha256.Update(message);
      vector<unsigned char> this_msg_hash = sha256.Finalize();

      LOG_GENERAL(
          INFO,
          "Sending message with hash: ["
              << DataConversion::Uint8VecToHexStr(this_msg_hash).substr(0, 6)
              << "] to NUM_FORWARDED_BLOCK_RECEIVERS_PER_SHARD:"
              << NUM_FORWARDED_BLOCK_RECEIVERS_PER_SHARD << " shard peers");

      unsigned int numOfBlockReceivers = std::min(
          NUM_FORWARDED_BLOCK_RECEIVERS_PER_SHARD, (uint32_t)p->size());

      for (unsigned int i = 0; i < numOfBlockReceivers; i++) {
        const auto& kv = p->at(i);
        shardBlockReceivers.emplace_back(std::get<SHARD_NODE_PEER>(kv));

        LOG_GENERAL(
            INFO,
            " PubKey: " << DataConversion::SerializableToHexStr(
                               std::get<SHARD_NODE_PUBKEY>(kv))
                        << " IP: "
                        << std::get<SHARD_NODE_PEER>(kv).GetPrintableIPAddress()
                        << " Port: "
                        << std::get<SHARD_NODE_PEER>(kv).m_listenPortHost);
      }

      P2PComm::GetInstance().SendBroadcastMessage(shardBlockReceivers, message);
    } else {
      vector<Peer> shard_peers;

      for (const auto& kv : *p) {
        shard_peers.emplace_back(std::get<SHARD_NODE_PEER>(kv));
        LOG_GENERAL(
            INFO,
            " PubKey: " << DataConversion::SerializableToHexStr(
                               std::get<SHARD_NODE_PUBKEY>(kv))
                        << " IP: "
                        << std::get<SHARD_NODE_PEER>(kv).GetPrintableIPAddress()
                        << " Port: "
                        << std::get<SHARD_NODE_PEER>(kv).m_listenPortHost);
      }

      P2PComm::GetInstance().SendBroadcastMessage(shard_peers, message);
    }

    p++;
  }
}

SendDataToLookupFunc SendDataToLookupFuncDefault =
    [](const VectorOfLookupNode& lookups,
       const std::vector<unsigned char>& message) mutable -> void {
  SendDataToLookupNodesDefault(lookups, message);
};

SendDataToShardFunc SendDataToShardFuncDefault =
    [](const std::vector<unsigned char>& message, const DequeOfShard& shards,
       const unsigned int& my_shards_lo,
       const unsigned int& my_shards_hi) mutable -> void {
  SendDataToShardNodesDefault(message, shards, my_shards_lo, my_shards_hi);
};

DataSender& DataSender::GetInstance() {
  static DataSender datasender;
  return datasender;
}

void DataSender::DetermineShardToSendDataTo(
    unsigned int& my_cluster_num, unsigned int& my_shards_lo,
    unsigned int& my_shards_hi, const DequeOfShard& shards,
    const deque<pair<PubKey, Peer>>& tmpCommittee, const uint16_t& indexB2) {
  // Multicast block to my assigned shard's nodes - send BLOCK message
  // Message = [block]

  // Multicast assignments:
  // 1. Divide committee into clusters of size MULTICAST_CLUSTER_SIZE
  // 2. Each cluster talks to all shard members in each shard
  //    cluster 0 => Shard 0
  //    cluster 1 => Shard 1
  //    ...
  //    cluster 0 => Shard (num of clusters)
  //    cluster 1 => Shard (num of clusters + 1)
  if (LOOKUP_NODE_MODE) {
    LOG_GENERAL(WARNING,
                "DataSender::DetermineShardToSendDataTo not "
                "expected to be called from LookUp node.");
    return;
  }

  LOG_MARKER();

  unsigned int num_clusters = tmpCommittee.size() / MULTICAST_CLUSTER_SIZE;
  if ((tmpCommittee.size() % MULTICAST_CLUSTER_SIZE) > 0) {
    num_clusters++;
  }
  LOG_GENERAL(INFO, "DEBUG num of clusters " << num_clusters)
  unsigned int shard_groups_count = shards.size() / num_clusters;
  if ((shards.size() % num_clusters) > 0) {
    shard_groups_count++;
  }
  LOG_GENERAL(INFO, "DEBUG num of shard group count " << shard_groups_count)

  my_cluster_num = indexB2 / MULTICAST_CLUSTER_SIZE;
  my_shards_lo = my_cluster_num * shard_groups_count;
  my_shards_hi = my_shards_lo + shard_groups_count - 1;

  if (my_shards_hi >= shards.size()) {
    my_shards_hi = shards.size() - 1;
  }
}

bool DataSender::SendDataToOthers(
    const BlockBase& blockwcosig,
    const deque<pair<PubKey, Peer>>& sendercommittee,
    const DequeOfShard& shards, const VectorOfLookupNode& lookups,
    const BlockHash& hashForRandom,
    const ComposeMessageForSenderFunc& composeMessageForSenderFunc,
    const SendDataToLookupFunc& sendDataToLookupFunc,
    const SendDataToShardFunc& sendDataToShardFunc) {
  if (LOOKUP_NODE_MODE) {
    LOG_GENERAL(WARNING,
                "DataSender::SendDataToOthers not expected "
                "to be called from LookUp node.");
    return true;
  }

  LOG_MARKER();

  if (blockwcosig.GetB2().size() != sendercommittee.size()) {
    LOG_GENERAL(WARNING, "B2 size and committee size is not identical!");
    return false;
  }

  deque<pair<PubKey, Peer>> tmpCommittee;
  for (unsigned int i = 0; i < blockwcosig.GetB2().size(); i++) {
    if (blockwcosig.GetB2().at(i)) {
      tmpCommittee.push_back(sendercommittee.at(i));
    }
  }

  bool inB2 = false;
  uint16_t indexB2;
  for (const auto& entry : tmpCommittee) {
    if (entry.second == Peer()) {
      inB2 = true;
      break;
    }
    indexB2++;
  }

  if (inB2) {
    vector<unsigned char> message;
    if (!(composeMessageForSenderFunc &&
          composeMessageForSenderFunc(message))) {
      LOG_GENERAL(
          WARNING,
          "composeMessageForSenderFunc undefined or cannot compose message");
      return false;
    }

    uint16_t randomDigits =
        DataConversion::charArrTo16Bits(hashForRandom.asBytes());
    uint16_t nodeToSendToLookUpLo =
        randomDigits % (tmpCommittee.size() - TX_SHARING_CLUSTER_SIZE);
    uint16_t nodeToSendToLookUpHi =
        nodeToSendToLookUpLo + TX_SHARING_CLUSTER_SIZE;

    if (indexB2 >= nodeToSendToLookUpLo && indexB2 < nodeToSendToLookUpHi) {
      LOG_GENERAL(INFO,
                  "Part of the committeement (assigned) that will send the "
                  "Data to the lookup nodes");
      if (sendDataToLookupFunc) {
        sendDataToLookupFunc(lookups, message);
      }
    }

    unsigned int my_cluster_num;
    unsigned int my_shards_lo;
    unsigned int my_shards_hi;

    DetermineShardToSendDataTo(my_cluster_num, my_shards_lo, my_shards_hi,
                               shards, tmpCommittee, indexB2);

    if ((my_cluster_num + 1) <= shards.size()) {
      if (sendDataToShardFunc) {
        sendDataToShardFunc(message, shards, my_shards_lo, my_shards_hi);
      }
    }
  }

  return true;
}