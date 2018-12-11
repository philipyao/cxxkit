#include "2pc/coordinator.h"
#include "2pc/cohort.h"
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <cinttypes>
#include <unistd.h>
#include <pthread/pthread.h>
#include <unordered_map>

uint64_t g_seqno = 0;
pthread_mutex_t lock;
pthread_cond_t up;
pthread_cond_t down;
int tmrid = 0;
std::unordered_map<int, pthread_t> tmrs;

enum MsgType {
    kMsgTypeVoteRequest,
    kMsgTypeVoteRequestAck,
    kMsgTypeCommit,
    kMsgTypeCommitAck,
};
struct Msg {
    int typ;
    char data[512];
};
struct MsgDataVoteRequest {
    uint64_t from_seqno;
};
struct MsgDataVoteRequestAck {
    uint64_t seqno;
    int result;
};
struct MsgDataCommit {
    uint64_t from_seqno;
    int cmd;
};
struct kMsgTypeCommitAck {
    uint64_t seqno;
};

struct CohortTimerData {
    ptrdiff_t obj;
    void* data;
    size_t data_len;
};

Msg up_message;
Msg down_message;

class TestCoordinator : public kit::Coordinator {
public:
    TestCoordinator() : kit::Coordinator() {}
    ~TestCoordinator() {}

    void SendVoteRequest(uint64_t seqno) {

    }
    int SetTimer() {
        return 0;
    }
    void DelTimer(int tmr) {

    }
    void SendCommit(int seqno, int cmd) {

    }
    void OnFinished() {
        uint64_t seqno = GetSeqno();
        printf("coordinator %" PRIu64 " 2pc transaction finished: callback\n", seqno);
    }
};

class TestCohort : public kit::Cohort {
public:
    kit::VoteResult OnBusinessVoteRequest(void* data, size_t data_len) {
        printf("cohort[%p] OnBusinessVoteRequest: data %p, len %d\n", this, data, data_len);
        //20% 几率 abort
        return (std::rand() % 5) ? kit::kVoteResultCommit : kit::kVoteResultAbort;
    }
    void SendVoteResult(uint64_t from_seqno, int vote_result) {
        down_message.typ = kMsgTypeVoteRequestAck;
        *(int*)(down_message.data) = vote_result;
        printf("cohort[%p] SendVoteResult: from_seqno %" PRIu64 ", vote_result %d\n",
               this, from_seqno, vote_result);
    }
    int SetTimer(void* data, size_t data_len) {
        ++tmrid;
        printf("cohort[%p] SetTimer ok: timer %d\n", this, tmrid);
        pthread_t th_tmr;
        CohortTimerData tmr_data;
        tmr_data.obj = ptrdiff_t(this);
        tmr_data.data = data;
        tmr_data.data_len = data_len;
        pthread_create(&th_tmr, NULL, TimeoutCohort, &tmr_data);
        tmrs[tmrid] = th_tmr;
        return tmrid;
    }
    void DelTimer(int tmr) {
        auto search = tmrs.find(tmr);
        if (search != tmrs.end()) {
            pthread_cancel(search->second);
            printf("cohort[%p] DelTimer ok: timer %d %d\n", this, tmr, search->first);
        } else {
            printf("cohort[%p] Error DelTimer: timer %d not found\n", this, tmr);
        }
    }
    void OnBusinessCommit(void* data, size_t data_len) {
        printf("cohort[%p] OnBusinessCommit: data %p, len %zd\n", this, data, data_len);
    }
    void OnBusinessAbort(void* data, size_t data_len) {
        printf("cohort[%p] OnBusinessAbort: data %p, len %zd\n", this, data, data_len);
    }
    void SendAck(int from_seqno) {
        printf("cohort[%p] SendAck: from_seqno %d\n", this, from_seqno);
        down_message.typ = kMsgTypeVoteRequestAck;

        pthread_cond_signal(&down);
    }  
};


void* RunCoordinator(void* args) {
    kit::Coordinator* bp = new TestCoordinator();
    bp->Start();


    return nullptr;
}

void* TimeoutCoordinator(void* args) {
    sleep(5);
    return nullptr;
}

void *RunCohort(void* args) {
    printf("RunCohort\n");

    kit::Cohort* bp = nullptr;
    uint64_t curr_seqno = 0;
    while (1) {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&up, &lock);

        printf("cohort recv message from coordinator: %d\n", up_message.typ);

        switch (up_message.typ) {
        case kMsgTypeVoteRequest:
            MsgDataVoteRequest* req_data = (MsgDataVoteRequest*)(up_message.data);
            if (req_data->from_seqno != curr_seqno) {
                printf("create new cohort, from_seqno %" PRIu64 "\n", req_data->from_seqno);
                bp = new TestCohort();
                bp->RecvVoteRequest(req_data->from_seqno, nullptr, 0);
            }
            break;
        case kMsgTypeCommit:
            MsgDataCommit* cmt_data = (MsgDataCommit*)(up_message.data);
            assert(bp != nullptr);
            bp->RecvCommit(cmt_data->from_seqno, cmt_data->cmd, nullptr, 0);
            break;
        default:
            printf("unrecognized message type %d\n", up_message.typ);
            break;    
        }

        pthread_mutex_unlock(&lock);
    }
    return nullptr;
}

void* TimeoutCohort(void* args) {
    CohortTimerData* pdata = (CohortTimerData*)args;
    printf("cohort[%p] start run TimeoutCohort thread\n", pdata->obj);
    sleep(5);
    printf("cohort[%p] TimeoutCohort thread timeout\n", pdata->obj);
    kit::Cohort* obj = (kit::Cohort*)(pdata->obj);
    obj->Timeout(pdata->data, pdata->data_len);
    return nullptr;
}

int main() {
    std::srand(std::time(nullptr));
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&up, NULL);
    pthread_cond_init(&down, NULL);

    pthread_t th_cohort;
    pthread_t th_coordinator;
    void *retval;

    pthread_create(&th_cohort, NULL, RunCohort, nullptr);

    pthread_join(th_cohort, &retval);

    return 0;
}