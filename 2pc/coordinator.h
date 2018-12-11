#ifndef __2PC_COORDINATOR_H__
#define __2PC_COORDINATOR_H__

#include <stddef.h>
#include <stdint.h>
#include "def.h"

namespace kit {

class Coordinator {
public:
	Coordinator();

	//No copying allowed
	Coordinator(const Coordinator& other) = delete;
	Coordinator& operator=(const Coordinator& other) = delete;

    virtual ~Coordinator() {}

    //======= 以下接口供上层业务调用  ===========
    //1) 2pc开始
    void Start();
    //2) 收到vote的返回
    void RecvVoteReply(VoteResult result);
    //3) 收到commit的返回
    void RecvAck();
    //4) 超时后调用
    void Timeout(void* data, size_t data_len);

    //======= 以下回调接口需要上层业务实现  ===========
    virtual void SendVoteRequest(uint64_t seqno) = 0;
    virtual int SetTimer() = 0;
    virtual void DelTimer(int tmr) = 0;
    virtual void SendCommit(int seqno, int cmd) = 0;
    virtual void OnFinished() = 0;

    //获取seqno
    uint64_t GetSeqno() const { return seqno_; }

private:
    static uint64_t GenerateSeqno();
    void SendCommitCmd(int cmd);
    void TryDelTimer();

    int state_;
    uint64_t seqno_;
    int timer_;
    int commit_cmd_;
};

} // namespace kit


#endif //__2PC_COORDINATOR_H__