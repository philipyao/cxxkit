#include "cohort.h"
#include "def.h"
#include <stdio.h>
#include <cinttypes>

namespace kit {

enum StateCohort {
    kStateCohortStart       = 1,
    kStateCohortVote,
    kStateCohortCommit,
};

static const char* GenStateStr(int st) {
    switch (st) {
    case kStateCohortStart:
        return GENSTR(kStateCohortStart);
    case kStateCohortVote:
        return GENSTR(kStateCohortVote);
    case kStateCohortCommit:
        return GENSTR(kStateCohortCommit);
    default:
        return "unknown";             
    }
}

Cohort::Cohort() {
    state_ = kStateCohortStart;
    from_seqno_ = 0;
    vote_result_ = kVoteReusultInvalid;
    timer_ = 0;
}

void Cohort::RecvVoteRequest(uint64_t from_seqno, void* data, size_t data_len) {
    printf("cohort[%p] RecvVoteRequest: from_seqno %" PRIu64 ", state %s\n", 
            this, from_seqno, GenStateStr(state_));
    if (state_ == kStateCohortStart) {
        from_seqno_ = from_seqno;
        auto result = OnBusinessVoteRequest(data, data_len);
        vote_result_ = result;
        state_ = kStateCohortVote;
        SendVoteResult(from_seqno, result);
        //启用定时器
        timer_ = SetTimer(&from_seqno, sizeof(from_seqno));
    } else if (state_ == kStateCohortVote) {
        printf("recv retransmit\n");
        //对方重发了vote request
        if (from_seqno_ != from_seqno) {
            //todo
        }
        //直接将 vote_result_ 回给对方
         SendVoteResult(from_seqno, vote_result_);
    } else {
        //
        printf("Err: cohort %p receive vote request from %" PRIu64 " when in state %s\n", 
               this, from_seqno, GenStateStr(state_));
    }
    return ;
}

void Cohort::RecvCommit(uint64_t from_seqno, int cmd, void* data, size_t data_len) {
    printf("cohort[%p] RecvCommit: from_seqno %" PRIu64 ", state %s\n", 
            this, from_seqno, GenStateStr(state_));
    if (state_ == kStateCohortStart) {
        printf("Err: cohort %p receive commit request from %" PRIu64 " when in state %s\n", 
               this, from_seqno, GenStateStr(state_));
        return;
    } else if (state_ == kStateCohortVote) {
        //删除超时定时器
        if (timer_ != 0) {
            DelTimer(timer_);
            timer_ = 0;
        }
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
    printf("cohort[%p] Timeout: data %p, len %zd, state %s\n", 
            this, data, data_len, GenStateStr(state_));
    if (state_ == kStateCohortVote) {
        printf("just abort\n");
        //没有等到 commit 通知, 直接放弃 (TODO 优化)
        OnBusinessAbort(data, data_len);
        state_ = kStateCohortCommit;
        SendAck(from_seqno_);
    }
}

} //namespace kit