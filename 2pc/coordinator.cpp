#include "coordinator.h"

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
    //发送投票请求
    SendVoteRequest(seqno_);
    state_ = kStateTypePrepared;
    //设置投票超时
    timer_ = SetTimer();
    return;
}

void Coordinator::RecvVoteReply(VoteResult result) {
    //删除定时器
    TryDelTimer();

    if (result == kVoteResultCommit) {      
    } else {
    }

    //收到所有的reply后，决定cmd是什么
    commit_cmd_ = 1;
    SendCommitCmd(commit_cmd_);
}

void Coordinator::RecvAck() {
    //删除定时器
    TryDelTimer();

    if (state_ == kStateTypeCommitted) {
        SendCommitCmd(commit_cmd_);
        return;
    }

    //TODO 释放资源

    //结束2PC事务, 回调上层
    OnFinished();
}

void Coordinator::Timeout(void* data, size_t data_len) {
    if (state_ == kStateTypePrepared) {
        commit_cmd_ = 0; //abort
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
    //发出提交指令
    SendCommit(seqno_, cmd);
    //设置提交超时
    timer_ = SetTimer();
}

void Coordinator::TryDelTimer() {
    if (timer_) {
        DelTimer(timer_);
        timer_ = 0;
    } 
}

} //namespace kit
