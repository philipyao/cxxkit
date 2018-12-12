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
int coordiator_tmrid = 0;
std::unordered_map<int, pthread_t> tmrs;
std::unordered_map<int, pthread_t> coordiator_tmrs;

enum MsgType {
    kMsgTypeVoteRequest,
    kMsgTypeVoteRequestAck,
    kMsgTypeCommit,
    kMsgTypeCommitAck,
};
struct Msg {
    struct Msg* next;
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

Msg* up_msgs = nullptr;
Msg* down_msgs = nullptr;

static void* RunCoordinator(void* args);
static void* TimeoutCoordinator(void* args);
static void *RunCohort(void* args);
static void *TimeoutCohort(void* args);

class TestCoordinator : public kit::Coordinator {
public:
    TestCoordinator() : kit::Coordinator() {}
    ~TestCoordinator() {}

    void SendVoteRequest(uint64_t seqno) {
        Msg* msg = new Msg();
        msg->typ = kMsgTypeVoteRequest;
        MsgDataVoteRequest* req_data = (MsgDataVoteRequest*)(msg->data);
        req_data->from_seqno = seqno;
        printf("SendVoteRequest: seqno %" PRIu64 "\n", seqno);

        pthread_mutex_lock(&lock);
        printf("coordinator pthread_cond_signal to cohort\n");
        msg->next = up_msgs;
        up_msgs = msg;
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&up);
    }
    int SetTimer(void* data, size_t data_len) {
        ++coordiator_tmrid;
        printf("coordinator[%p] SetTimer ok: timer %d\n", this, coordiator_tmrid);
        pthread_t th_tmr;
        CohortTimerData tmr_data;
        tmr_data.obj = ptrdiff_t(this);
        tmr_data.data = data;
        tmr_data.data_len = data_len;
        pthread_create(&th_tmr, NULL, TimeoutCoordinator, &tmr_data);
        coordiator_tmrs[coordiator_tmrid] = th_tmr;
        return coordiator_tmrid;
    }
    void DelTimer(int tmr) {
        auto search = coordiator_tmrs.find(tmr);
        if (search != coordiator_tmrs.end()) {
            pthread_cancel(search->second);
            printf("coordinator[%p] DelTimer ok: timer %d %d\n", this, tmr, search->first);
        } else {
            printf("coordinator[%p] Error DelTimer: timer %d not found\n", this, tmr);
        }
    }
    void SendCommit(uint64_t seqno, int cmd) {
        Msg* msg = new Msg();
        msg->typ = kMsgTypeCommit;
        MsgDataCommit* req_data = (MsgDataCommit*)(msg->data);
        req_data->from_seqno = seqno;
        req_data->cmd = cmd;
        printf("SendCommit: seqno %" PRIu64 ", cmd %d\n", seqno, cmd);
        
        pthread_mutex_lock(&lock);
        printf("coordinator pthread_cond_signal to cohort\n");
        msg->next = up_msgs;
        up_msgs = msg;
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&up);
    }
    void OnFinished() {
        uint64_t seqno = GetSeqno();
        printf("coordinator %" PRIu64 " 2pc transaction finished: callback\n", seqno);
    }
};

class TestCohort : public kit::Cohort {
public:
    kit::VoteResult OnBusinessVoteRequest(void* data, size_t data_len) {
        printf("cohort[%p] OnBusinessVoteRequest: data %p, len %zu\n", this, data, data_len);
        //20% 几率 abort
        return (std::rand() % 5) ? kit::kVoteResultCommit : kit::kVoteResultAbort;
    }
    void SendVoteResult(uint64_t from_seqno, int vote_result) {
        Msg* msg = new Msg();
        msg->typ = kMsgTypeVoteRequestAck;
        MsgDataVoteRequestAck* ack = (MsgDataVoteRequestAck*)(msg->data);
        ack->seqno = from_seqno;
        ack->result = vote_result;
        printf("cohort[%p] SendVoteResult: from_seqno %" PRIu64 ", vote_result %d\n",
               this, from_seqno, vote_result);

        pthread_mutex_lock(&lock);
        printf("cohort pthread_cond_signal to cohort\n");
        msg->next = down_msgs;
        down_msgs = msg;
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&down);
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
        Msg* msg = new Msg();
        msg->typ = kMsgTypeVoteRequestAck;

        pthread_mutex_lock(&lock);
        printf("cohort pthread_cond_signal to cohort\n");
        msg->next = down_msgs;
        down_msgs = msg;
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&down);
    }  
};

void* RunCoordinator(void* args) {
    printf("RunCoordinator\n");

    pthread_mutex_lock(&lock);

    printf("coordinator get lock\n");
    kit::Coordinator* bp = new TestCoordinator();
    bp->Start();

    printf("coordinator release lock\n");
    pthread_mutex_unlock(&lock);

    while (1) {
        pthread_mutex_lock(&lock);
        printf("coordinator wait...\n");
        pthread_cond_wait(&down, &lock);

        printf("coordinator get lock\n");

        Msg incoming_msg;
        incoming_msg = down_message;
        printf("coordinator recv message from coordinator: msg<%d>\n", incoming_msg.typ);

        printf("coordinator release lock\n");
        pthread_mutex_unlock(&lock);

        MsgDataVoteRequestAck* reply_data;
        switch (incoming_msg.typ) {
        case kMsgTypeVoteRequestAck:
            reply_data = (MsgDataVoteRequestAck*)(incoming_msg.data);
            if (reply_data->seqno != bp->GetSeqno()) {
                printf("coordinator mismatch seqno %" PRIu64 " %" PRIu64 "\n", 
                       reply_data->seqno, bp->GetSeqno());
            } else {
                bp->RecvVoteReply(kit::VoteResult(reply_data->result));
            }
            break;
        case kMsgTypeCommit:
            bp->RecvAck();
            break;
        default:
            printf("unrecognized message type %d\n", incoming_msg.typ);
            break;    
        }

    }
    

    return nullptr;
}

void* TimeoutCoordinator(void* args) {
    CohortTimerData* pdata = (CohortTimerData*)args;
    kit::Coordinator* obj = (TestCoordinator*)(pdata->obj);
    printf("coordinator[%p] start run TimeoutCoordinator thread\n", obj);
    sleep(5);
    printf("coordinator[%p] TimeoutCoordinator thread timeout\n", obj);
    
    obj->Timeout(pdata->data, pdata->data_len);
    return nullptr;
}

void *RunCohort(void* args) {
    printf("RunCohort\n");

    kit::Cohort* bp = nullptr;
    uint64_t curr_seqno = 0;
    
    while (1) {
        pthread_mutex_lock(&lock);
        printf("cohort wait...\n");
        pthread_cond_wait(&up, &lock);

        printf("cohort get lock\n");
        Msg incoming_msg;
        incoming_msg = up_message;
        printf("cohort recv message from coordinator: msg<%d>\n", incoming_msg.typ);
        printf("cohort release lock\n");
        pthread_mutex_unlock(&lock);        

        MsgDataVoteRequest* req_data;
        MsgDataCommit* cmt_data;
        switch (incoming_msg.typ) {
        case kMsgTypeVoteRequest:
            req_data = (MsgDataVoteRequest*)(incoming_msg.data);
            if (req_data->from_seqno != curr_seqno) {
                printf("create new cohort, from_seqno %" PRIu64 "\n", req_data->from_seqno);
                bp = new TestCohort();
                bp->RecvVoteRequest(req_data->from_seqno, nullptr, 0);
            }
            break;
        case kMsgTypeCommit:
            cmt_data = (MsgDataCommit*)(incoming_msg.data);
            assert(bp != nullptr);
            bp->RecvCommit(cmt_data->from_seqno, cmt_data->cmd, nullptr, 0);
            break;
        default:
            printf("unrecognized message type %d\n", incoming_msg.typ);
            break;    
        }


    }
    
    return nullptr;
}

void* TimeoutCohort(void* args) {
    CohortTimerData* pdata = (CohortTimerData*)args;
    kit::Cohort* obj = (TestCohort*)(pdata->obj);
    printf("cohort[%p] start run TimeoutCohort thread\n", obj);
    sleep(5);
    printf("cohort[%p] TimeoutCohort thread timeout\n", obj);
    
    obj->Timeout(pdata->data, pdata->data_len);
    return nullptr;
}



int main() {
    std::srand(std::time(nullptr));
    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&up, nullptr);
    pthread_cond_init(&down, nullptr);

    pthread_t th_cohort;
    pthread_t th_coordinator;
    void *retval;

    pthread_create(&th_cohort, nullptr, RunCohort, nullptr);
    pthread_create(&th_coordinator, nullptr, RunCoordinator, nullptr);

    pthread_join(th_cohort, &retval);
    pthread_join(th_coordinator, &retval);

    return 0;
}