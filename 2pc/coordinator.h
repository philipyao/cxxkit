#ifndef __2PC_COORDINATOR_H__
#define __2PC_COORDINATOR_H__

#include <stddef.h>
#include <stdint.h>

namespace kit {

class Coordinator {
public:
	Coordinator();

	//No copying allowed
	Coordinator(const Coordinator& other) = delete;
	Coordinator& operator=(const Coordinator& other) = delete;

    virtual ~Coordinator() {}

    virtual void SendPrepare() = 0;
    virtual void SendCommit() = 0;
    virtual void OnFinished() = 0;

    void Start();
    void OnRecvPrepareAck(int result);
    void OnRecvCommitAck();
    void OnTimeout(void* data, size_t data_len);
    uint64_t GetSeqno() const { return seqno_; }

private:
    static uint64_t GenerateSeqno();
    
    int state_;
    uint64_t seqno_;
};

} // namespace kit


#endif //__2PC_COORDINATOR_H__