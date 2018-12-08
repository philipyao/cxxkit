#ifndef __2PC_COHORT_H__
#define __2PC_COHORT_H__

#include <stdint.h>
#include <cstddef>
#include "def.h"

namespace kit {

class Cohort {
public:
	Cohort();
	//No copying allowed
	Cohort(const Cohort& other) = delete;
	Cohort& operator=(const Cohort& other) = delete;

    virtual ~Cohort() {}

    //======= 以下接口供上层业务调用  ===========
    //1) 收到投票请求后调用
    bool RecvVoteRequest(uint64_t from_seqno, void* data, size_t data_len);
    //2） 收到提交指令后调用
    void RecvCommit(uint64_t from_seqno, int cmd, void* data, size_t data_len);
    //3) 超时触发后调用
    void Timeout(void* data, size_t data_len);

    //======= 以下回调接口需要上层业务实现  ===========
    //1) 收到投票请求后，业务处理，返回是否提交
    virtual VoteResult OnBusinessVoteRequest(void* data, size_t data_len) = 0;
    //2) 发送投票结果给coordinator，业务自己实现
    virtual void SendVoteResult(int from_seqno, int vote_result) = 0;
    //3) 投票后开启超时等待
    virtual void SetTimer() = 0;
    //4) 收到提交后的业务处理
    virtual void OnBusinessCommit(void* data, size_t data_len) = 0;
    //5) 收到取消后的业务处理
    virtual void OnBusinessAbort(void* data, size_t data_len) = 0;
    //6) 发送提交回应给coordinator，业务自己实现
    virtual void SendAck(int from_seqno) = 0;


private:
    int state_;
    uint64_t from_seqno_;
    VoteResult vote_result_;
};

} // namespace kit

#endif //__2PC_COHORT_H__