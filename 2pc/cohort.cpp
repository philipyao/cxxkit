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
    seqno_ = Cohort::GenerateSeqno();
    state_ = kStateCohortStart;
    vote_result_ = kVoteReusultInvalid;
}

void Cohort::RecvVoteRequest(uint64_t from_seqno, void* data, size_t data_len) {
    if (state_ == kStateCohortStart) {
        from_seqno_ = from_seqno;
        auto result = OnBusinessVoteRequest(data, data_len);
        vote_result_ = result;
        state_ = kStateCohortVote;
        SendVoteResult(from_seqno, result);
        //todo 启用定时器
    } else if (state_ == kStateCohortVote) {
        //对方重发了vote request
        if (from_seqno_ != from_seqno) {
            //todo
        }
        //直接将 vote_result_ 回给对方
         SendVoteResult(from_seqno, vote_result_);
    } else {
        //
        printf("Err: cohort %d receive vote request from %d when in state %d\n", seqno_, from_seqno, state_);
    }
}

void Cohort::RecvCommit(uint64_t from_seqno, int cmd, void* data, size_t data_len) {
    if (state_ == kStateCohortStart) {
        printf("Err: cohort %d receive commit request from %d when in state %d\n", seqno_, from_seqno, state_);
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
        //没有等到 commit 通知
    } else {

    }
}

uint64_t Cohort::GenerateSeqno() {
    static uint64_t base_seq = 0;
    ++base_seq;
    base_seq = (base_seq == 0) ? 1 : base_seq;
    return base_seq;
}

} //namespace kit