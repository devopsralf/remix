// Copyright (c) 2014-2016, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "pending_transaction.h"
#include "wallet.h"
#include "common_defines.h"

#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_basic_impl.h"
#include "cryptonote_core/cryptonote_format_utils.h"

#include <memory>
#include <vector>
#include <sstream>
#include <boost/format.hpp>

using namespace std;

namespace Bitmonero {

PendingTransaction::~PendingTransaction() {}


PendingTransactionImpl::PendingTransactionImpl(WalletImpl &wallet)
    : m_wallet(wallet)
{

}

PendingTransactionImpl::~PendingTransactionImpl()
{

}

int PendingTransactionImpl::status() const
{
    return m_status;
}

string PendingTransactionImpl::errorString() const
{
    return m_errorString;
}

bool PendingTransactionImpl::commit()
{

    LOG_PRINT_L0("m_pending_tx size: " << m_pending_tx.size());
    assert(m_pending_tx.size() == 1);
    try {
        while (!m_pending_tx.empty()) {
            auto & ptx = m_pending_tx.back();
            m_wallet.m_wallet->commit_tx(ptx);
            // success_msg_writer(true) << tr("Money successfully sent, transaction ") << get_transaction_hash(ptx.tx);
            // if no exception, remove element from vector
            m_pending_tx.pop_back();
        } // TODO: extract method;
    } catch (const tools::error::daemon_busy&) {
        // TODO: make it translatable with "tr"?
        m_errorString = tr("daemon is busy. Please try again later.");
        m_status = Status_Error;
    } catch (const tools::error::no_connection_to_daemon&) {
        m_errorString = tr("no connection to daemon. Please make sure daemon is running.");
        m_status = Status_Error;
    } catch (const tools::error::tx_rejected& e) {
        std::ostringstream writer(m_errorString);
        writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
        m_status = Status_Error;
    } catch (std::exception &e) {
        m_errorString = string(tr("Unknown exception: ")) + e.what();
        m_status = Status_Error;
    } catch (...) {
        m_errorString = tr("Unhandled exception");
        LOG_ERROR(m_errorString);
        m_status = Status_Error;
    }

    return m_status == Status_Ok;
}

uint64_t PendingTransactionImpl::amount() const
{
    uint64_t result = 0;
    for (const auto &ptx : m_pending_tx)   {
        for (const auto &dest : ptx.dests) {
            result += dest.amount;
        }
    }
    return result;
}

uint64_t PendingTransactionImpl::dust() const
{
    uint64_t result = 0;
    for (const auto & ptx : m_pending_tx) {
        result += ptx.dust;
    }
    return result;
}

uint64_t PendingTransactionImpl::fee() const
{
    uint64_t result = 0;
    for (const auto ptx : m_pending_tx) {
        result += ptx.fee;
    }
    return result;
}

}

