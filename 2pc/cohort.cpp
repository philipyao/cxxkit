#include "cohort.h"
#include "def.h"
#include <stdio.h>

namespace kit {

enum StateCohort {
    kStateCohortStart       = 1,
    kStateCohortVote,
    kStateCohortCommit,
};

Cohort::Cohort() {
    state_ = kStateCohortStart;
    vote_result_ = kVoteReusultInvalid;
}

bool Cohort::RecvVoteRequest(uint64_t from_seqno, void* data, size_t data_len) {
    if (state_ == kStateCohortStart) {
        from_seqno_ = from_seqno;
        auto result = OnBusinessVoteRequest(data, data_len);
        vote_result_ = result;
        state_ = kStateCohortVote;
        SendVoteResult(from_seqno, result);
        //启用定时器
        SetTimer();
    } else if (state_ == kStateCohortVote) {
        //对方重发了vote request
        if (from_seqno_ != from_seqno) {
            //todo
        }
        //直接将 vote_result_ 回给对方
         SendVoteResult(from_seqno, vote_result_);
    } else {
        //
        printf("Err: cohort %p receive vote request from %d when in state %d\n", this, from_seqno, state_);
    }
    return false;
}

void Cohort::RecvCommit(uint64_t from_seqno, int cmd, void* data, size_t data_len) {
    if (state_ == kStateCohortStart) {
        printf("Err: cohort %p receive commit request from %d when in state %d\n", this, from_seqno, state_);
        return;
    } else if (state_ == kStateCohortVote) {
        if (cmd == kCmdGlobalCommit) {
            OnBusinessCommit(data, data_len);
        } else {
            OnBusinessAbort(data, data_len);
        }
        state_ = kStateCohortCommit;
        SendAck(from_seqno);
    } else {
        //对方可能没有收到commit回复，重发了commit
        SendAck(from_seqno);
    }
}

void Cohort::Timeout(void* data, size_t data_len) {
    if (state_ == kStateCohortVote) {
        //没有等到 commit 通知, 直接放弃 (TODO 优化)
        OnBusinessAbort(data, data_len);
        state_ = kStateCohortCommit;
        SendAck(from_seqno_);
    }
}

} //namespace kit