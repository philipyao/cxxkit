#include "coordinator.h"
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

namespace kit {

enum StateType {
    kStateTypeStart         = 1,
    kStateTypePrepared,
    kStateTypeCommitted,
};

Coordinator::Coordinator() {
    state_ = kStateTypeStart;
    seqno_ = Coordinator::GenerateSeqno();
    timer_ = 0;
    commit_cmd_ = 0;
}

void Coordinator::Start() {
    printf("coordinator[%p] start, seqno %" PRIu64 "\n", this, seqno_);
    //发送投票请求
    SendVoteRequest(seqno_);
    state_ = kStateTypePrepared;
    //设置投票超时
    timer_ = SetTimer(&seqno_, sizeof(seqno_));
    return;
}

void Coordinator::RecvVoteReply(VoteResult result) {
    printf("Coordinator[%p] RecvVoteReply: result %d, state %d\n", this, result, state_);

    if (state_ == kStateTypePrepared) {
        //删除定时器
        TryDelTimer();

        //收到所有的reply后，决定cmd是什么
        if (result == kVoteResultCommit) {
            commit_cmd_ = kCmdGlobalCommit;
        } else {
            commit_cmd_ = kCmdGlobalAbort;
        }
        
        SendCommitCmd(commit_cmd_);
    }
}

void Coordinator::RecvAck() {
    printf("Coordinator[%p] RecvAck, state %d\n", this, state_);
    
    if (state_ == kStateTypeCommitted) {
        //删除定时器
        TryDelTimer();
        SendCommitCmd(commit_cmd_);
    }

    //TODO 释放资源

    //结束2PC事务, 回调上层
    OnFinished();

    return;
}

void Coordinator::Timeout(void* data, size_t data_len) {
    timer_ = 0;
    printf("coordinator[%p] Timeout, state %d\n", this, state_);
    if (state_ == kStateTypePrepared) {
        printf("coordinator[%p] start to abort\n", this);
        commit_cmd_ = kVoteResultAbort;
        SendCommitCmd(commit_cmd_);
        state_ = kStateTypeCommitted;
    } else if (state_ == kStateTypeCommitted) {
        //没有收到ack，再发送
        SendCommitCmd(commit_cmd_);
    }
    return; 
}

uint64_t Coordinator::GenerateSeqno() {
    static uint64_t base_seq = 0;
    ++base_seq;
    return base_seq;
}


/// private part
void Coordinator::SendCommitCmd(int cmd) {
    assert(timer_ == 0);
    printf("Coordinator[%p] SendCommitCmd: %d\n", this, cmd);
    //发出提交指令
    SendCommit(seqno_, cmd);
    //设置提交超时
    timer_ = SetTimer(nullptr, 0);
}

void Coordinator::TryDelTimer() {
    if (timer_) {
        DelTimer(timer_);
        timer_ = 0;
    } 
}

} //namespace kit
