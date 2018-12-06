#ifndef __2PC_COHORT_H__
#define __2PC_COHORT_H__

#include <stdint.h>
#include <cstddef>

namespace kit {

class Cohort {
public:
	Cohort();
	//No copying allowed
	Cohort(const Cohort& other) = delete;
	Cohort& operator=(const Cohort& other) = delete;

    virtual ~Cohort() {}

    virtual void OnRecvPrepare(uint64_t seqno) = 0;
    virtual void OnRecvCommit(uint64_t seqno) = 0;
    virtual void SendPrepareAck() = 0;
    virtual void SendCommitAck() = 0;

    void OnTimeout(void* data, size_t data_len);

    //上层业务回调
    virtual void OnBusinessPrepare(void* data, size_t data_len) = 0;
    virtual void OnBusinessCommit(int cmd) = 0;

private:
    static uint64_t GenerateSeqno();
    uint64_t seqno_;
};

} // namespace kit

#endif //__2PC_COHORT_H__