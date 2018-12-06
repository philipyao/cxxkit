#include "cohort.h"

namespace kit {

Cohort::Cohort() {
    seqno_ = Cohort::GenerateSeqno();
}

void Cohort::OnRecvPrepare() {

}

void Cohort::OnRecvCommit() {
    //TODO 释放资源，删除定时器

    //回调上层，结束2PC事务
    OnFinished();
}

void Cohort::OnTimeout(void* data, size_t data_len) {

    return; 
}

uint64_t Cohort::GenerateSeqno() {
    static uint64_t base_seq = 0;
    ++base_seq;
    base_seq = (base_seq == 0) ? 1 : base_seq;
    return base_seq;
}

} //namespace kit