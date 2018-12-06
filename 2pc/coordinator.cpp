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
}

void Coordinator::Start() {
    SendPrepare();
    state_ = kStateTypePrepared;
    return;
}

void Coordinator::OnRecvPrepareAck(int result) {
    if (result > 0) {
        SendCommit();
    } else {
        // SendAbort();
    }
}

void Coordinator::OnRecvCommitAck() {
    //TODO 释放资源，删除定时器

    //回调上层，结束2PC事务
    OnFinished();
}

void Coordinator::OnTimeout(void* data, size_t data_len) {
    if (state_ == kStateTypePrepared) {
        // SendAbort();
    } else if (state_ == kStateTypeCommitted) {
        //超时重试
        //SendCommit();
        //SendAbort();
    }
    return; 
}

uint64_t Coordinator::GenerateSeqno() {
    static uint64_t base_seq = 0;
    ++base_seq;
    return base_seq;
}

} //namespace kit
