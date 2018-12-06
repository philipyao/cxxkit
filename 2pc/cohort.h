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



    virtual void SendCommitAck() = 0;

    void RecvVoteRequest(uint64_t from_seqno, void* data, size_t data_len);
    void RecvCommit(uint64_t from_seqno, int cmd, void* data, size_t data_len);
    void Timeout(void* data, size_t data_len);

    //上层业务回调
    virtual void SendVoteResult(int from_seqno, int vote_result) = 0;
    virtual void SendAck(int from_seqno) = 0;
    virtual VoteResult OnBusinessVoteRequest(void* data, size_t data_len) = 0;
    virtual void OnBusinessCommit(void* data, size_t data_len) = 0;
    virtual void OnBusinessAbort(void* data, size_t data_len) = 0;

private:
    static uint64_t GenerateSeqno();
    uint64_t seqno_;
    int state_;
    uint64_t from_seqno_;
    VoteResult vote_result_;
};

} // namespace kit

#endif //__2PC_COHORT_H__