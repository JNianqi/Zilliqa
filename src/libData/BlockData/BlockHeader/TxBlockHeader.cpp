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

#include "TxBlockHeader.h"
#include "libMessage/Messenger.h"
#include "libUtils/Logger.h"

using namespace std;
using namespace boost::multiprecision;

TxBlockHeader::TxBlockHeader() : m_blockNum((uint64_t)-1) {}

TxBlockHeader::TxBlockHeader(const vector<unsigned char>& src,
                             unsigned int offset) {
  if (!Deserialize(src, offset)) {
    LOG_GENERAL(WARNING, "We failed to init TxBlockHeader.");
  }
}

TxBlockHeader::TxBlockHeader(
    uint8_t type, uint32_t version, const uint256_t& gasLimit,
    const uint256_t& gasUsed, const uint256_t& rewards, const BlockHash& prevHash,
    const uint64_t& blockNum, const uint256_t& timestamp,
    const TxnHash& txRootHash, const StateHash& stateRootHash,
    const StateHash& deltaRootHash, const StateHash& stateDeltaHash,
    const TxnHash& tranReceiptRootHash, uint32_t numTxs,
    uint32_t numMicroBlockHashes, const PubKey& minerPubKey,
    const uint64_t& dsBlockNum, const BlockHash& dsBlockHeader)
    : m_type(type),
      m_version(version),
      m_gasLimit(gasLimit),
      m_gasUsed(gasUsed),
      m_rewards(rewards),
      m_prevHash(prevHash),
      m_blockNum(blockNum),
      m_timestamp(timestamp),
      m_hash{txRootHash, stateRootHash, deltaRootHash, stateDeltaHash,
             tranReceiptRootHash},
      m_numTxs(numTxs),
      m_numMicroBlockHashes(numMicroBlockHashes),
      m_minerPubKey(minerPubKey),
      m_dsBlockNum(dsBlockNum),
      m_dsBlockHeader(dsBlockHeader) {}

bool TxBlockHeader::Serialize(vector<unsigned char>& dst,
                              unsigned int offset) const {
  if (!Messenger::SetTxBlockHeader(dst, offset, *this)) {
    LOG_GENERAL(WARNING, "Messenger::SetTxBlockHeader failed.");
    return false;
  }

  return true;
}

bool TxBlockHeader::Deserialize(const vector<unsigned char>& src,
                                unsigned int offset) {
  if (!Messenger::GetTxBlockHeader(src, offset, *this)) {
    LOG_GENERAL(WARNING, "Messenger::GetTxBlockHeader failed.");
    return false;
  }

  return true;
}

const uint8_t& TxBlockHeader::GetType() const { return m_type; }

const uint32_t& TxBlockHeader::GetVersion() const { return m_version; }

const uint256_t& TxBlockHeader::GetGasLimit() const { return m_gasLimit; }

const uint256_t& TxBlockHeader::GetGasUsed() const { return m_gasUsed; }

const uint256_t& TxBlockHeader::GetRewards() const { return m_rewards; }

const BlockHash& TxBlockHeader::GetPrevHash() const { return m_prevHash; }

const uint64_t& TxBlockHeader::GetBlockNum() const { return m_blockNum; }

const uint256_t& TxBlockHeader::GetTimestamp() const { return m_timestamp; }

const TxnHash& TxBlockHeader::GetTxRootHash() const {
  return m_hash.m_txRootHash;
}

const StateHash& TxBlockHeader::GetStateRootHash() const {
  return m_hash.m_stateRootHash;
}

const StateHash& TxBlockHeader::GetDeltaRootHash() const {
  return m_hash.m_deltaRootHash;
}

const StateHash& TxBlockHeader::GetStateDeltaHash() const {
  return m_hash.m_stateDeltaHash;
}

const TxnHash& TxBlockHeader::GetTranReceiptRootHash() const {
  return m_hash.m_tranReceiptRootHash;
}

const uint32_t& TxBlockHeader::GetNumTxs() const { return m_numTxs; }

const uint32_t& TxBlockHeader::GetNumMicroBlockHashes() const {
  return m_numMicroBlockHashes;
}

const PubKey& TxBlockHeader::GetMinerPubKey() const { return m_minerPubKey; }

const uint64_t& TxBlockHeader::GetDSBlockNum() const { return m_dsBlockNum; }

const BlockHash& TxBlockHeader::GetDSBlockHeader() const {
  return m_dsBlockHeader;
}

bool TxBlockHeader::operator==(const TxBlockHeader& header) const {
  return std::tie(m_type, m_version, m_gasLimit, m_gasUsed, m_rewards, m_prevHash,
                  m_blockNum, m_timestamp, m_hash, m_numTxs,
                  m_numMicroBlockHashes, m_minerPubKey, m_dsBlockHeader) ==
         std::tie(header.m_type, header.m_version, header.m_gasLimit,
                  header.m_gasUsed, header.m_rewards, header.m_prevHash, header.m_blockNum,
                  header.m_timestamp, header.m_hash, header.m_numTxs,
                  header.m_numMicroBlockHashes, header.m_minerPubKey,
                  header.m_dsBlockHeader);
}

bool TxBlockHeader::operator<(const TxBlockHeader& header) const {
  return std::tie(header.m_type, header.m_version, header.m_gasLimit,
                  header.m_gasUsed, header.m_rewards, header.m_prevHash, header.m_blockNum,
                  header.m_timestamp, header.m_hash, header.m_numTxs,
                  header.m_numMicroBlockHashes, header.m_minerPubKey,
                  header.m_dsBlockHeader) >
         std::tie(m_type, m_version, m_gasLimit, m_gasUsed, m_rewards, m_prevHash,
                  m_blockNum, m_timestamp, m_hash, m_numTxs,
                  m_numMicroBlockHashes, m_minerPubKey, m_dsBlockHeader);
}

bool TxBlockHeader::operator>(const TxBlockHeader& header) const {
  return header < *this;
}
