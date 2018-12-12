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
    kMsgTypeInvalid,
    kMsgTypeVoteRequest,
    kMsgTypeVoteRequestAck,
    kMsgTypeCommit,
    kMsgTypeCommitAck,
};
struct Msg {
    struct Msg* prev;
    struct Msg* next;
    int typ;
    char data[512];
    Msg() : prev(nullptr), next(nullptr), typ(kMsgTypeInvalid) {}
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

struct MsgQ {
    Msg* head;
    Msg* tail;
};

#define GENSTR(str) #str

MsgQ upq;
MsgQ downq;

static void Init();
static void MsgEnqueue(MsgQ* queue, Msg* msg);
static Msg* MsgDequeue(MsgQ* queue);
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
        printf("coordinator[%p] SendVoteRequest: seqno %" PRIu64 "\n", this, seqno);

        pthread_mutex_lock(&lock);
        printf("coordinator pthread_cond_signal to cohort\n");
        MsgEnqueue(&upq, msg);
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&up);
    }
    int SetTimer(void* data, size_t data_len) {
        ++coordiator_tmrid;
        printf("coordinator[%p] SetTimer ok: TimeoutCoordinator-%d\n", this, coordiator_tmrid);
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
            printf("coordinator[%p] DelTimer ok: timer %d\n", this, search->first);
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
        MsgEnqueue(&upq, msg);
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
        printf("cohort pthread_cond_signal to coordinator\n");
        MsgEnqueue(&downq, msg);
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&down);
    }
    int SetTimer(void* data, size_t data_len) {
        ++tmrid;
        printf("cohort[%p] SetTimer ok: TimeoutCohort-%d\n", this, tmrid);
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
            printf("cohort[%p] DelTimer ok: timer %d\n", this, search->first);
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
        msg->typ = kMsgTypeCommitAck;

        pthread_mutex_lock(&lock);
        printf("cohort pthread_cond_signal to cohort\n");
        MsgEnqueue(&downq, msg);
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&down);
    }  
};

void* RunCoordinator(void* args) {
    printf("RunCoordinator\n");
    kit::Coordinator* bp = new TestCoordinator();
    bp->Start();

    Msg* incoming_msg = nullptr;
    while (1) {
        pthread_mutex_lock(&lock);
        while ((incoming_msg = MsgDequeue(&downq)) == nullptr) {
            pthread_cond_wait(&down, &lock);
        }
        pthread_mutex_unlock(&lock);

        MsgDataVoteRequestAck* reply_data;
        switch (incoming_msg->typ) {
        case kMsgTypeVoteRequestAck:
            printf("===== coordinator handle message: msg<%s>\n", GENSTR(kMsgTypeVoteRequestAck));
            reply_data = (MsgDataVoteRequestAck*)(incoming_msg->data);
            if (reply_data->seqno != bp->GetSeqno()) {
                printf("coordinator mismatch seqno %" PRIu64 " %" PRIu64 "\n", 
                       reply_data->seqno, bp->GetSeqno());
            } else {
                bp->RecvVoteReply(kit::VoteResult(reply_data->result));
            }
            break;
        case kMsgTypeCommitAck:
            printf("===== coordinator handle message: msg<%s>\n", GENSTR(kMsgTypeCommitAck));
            bp->RecvAck();
            break;
        default:
            printf("unrecognized message type %d\n", incoming_msg->typ);
            break;    
        }

        printf("===== coordinator process message done.\n");

        delete incoming_msg;
        incoming_msg = nullptr;
    }
    

    return nullptr;
}

void* TimeoutCoordinator(void* args) {
    CohortTimerData* pdata = (CohortTimerData*)args;
    kit::Coordinator* obj = (TestCoordinator*)(pdata->obj);
    sleep(5);
    printf("coordinator[%p] TimeoutCoordinator thread timeout\n", obj);
    
    obj->Timeout(pdata->data, pdata->data_len);
    return nullptr;
}

void *RunCohort(void* args) {
    printf("RunCohort\n");

    kit::Cohort* bp = nullptr;
    uint64_t curr_seqno = 0;
    
    Msg* incoming_msg = nullptr;
    while (1) {
        pthread_mutex_lock(&lock);
        while ((incoming_msg = MsgDequeue(&upq)) == nullptr) {
            pthread_cond_wait(&up, &lock);
        }
        pthread_mutex_unlock(&lock);   

        MsgDataVoteRequest* req_data;
        MsgDataCommit* cmt_data;
        switch (incoming_msg->typ) {
        case kMsgTypeVoteRequest:
            printf("***** cohort handle message: msg<%s>\n", GENSTR(kMsgTypeVoteRequest));
            req_data = (MsgDataVoteRequest*)(incoming_msg->data);
            if (req_data->from_seqno != curr_seqno) {
                bp = new TestCohort();
                printf("create new cohort[%p], from_seqno %" PRIu64 "\n", bp, req_data->from_seqno);
                bp->RecvVoteRequest(req_data->from_seqno, nullptr, 0);
            }
            break;
        case kMsgTypeCommit:
            printf("***** cohort handle message: msg<%s>\n", GENSTR(kMsgTypeCommit));
            cmt_data = (MsgDataCommit*)(incoming_msg->data);
            assert(bp != nullptr);
            bp->RecvCommit(cmt_data->from_seqno, cmt_data->cmd, nullptr, 0);
            break;
        default:
            printf("unrecognized message type %d\n", incoming_msg->typ);
            break;    
        }

        printf("***** cohort process message done.\n");

        delete incoming_msg;
        incoming_msg = nullptr;
    }
    
    return nullptr;
}

void* TimeoutCohort(void* args) {
    CohortTimerData* pdata = (CohortTimerData*)args;
    kit::Cohort* obj = (TestCohort*)(pdata->obj);
    sleep(5);
    printf("cohort[%p] TimeoutCohort thread timeout\n", obj);
    
    obj->Timeout(pdata->data, pdata->data_len);
    return nullptr;
}

void Init() {
    //初始化上下行消息队列 (借助首尾哨兵)
    upq.head = new Msg();
    upq.tail = new Msg();
    upq.head->next = upq.tail;
    upq.tail->prev = upq.head;

    downq.head = new Msg();
    downq.tail = new Msg();
    downq.head->next = downq.tail;
    downq.tail->prev = downq.head;
}

void MsgEnqueue(MsgQ* queue, Msg* msg) {
    //消息入队到队尾
    queue->tail->prev->next = msg;
    msg->next = queue->tail;
    msg->prev = queue->tail->prev;
    queue->tail->prev = msg;
}
Msg* MsgDequeue(MsgQ* queue) {
    //从队头取出消息
    if (queue->head->next == queue->tail) {
        return nullptr;
    }
    Msg* msg = queue->head->next;
    queue->head->next = msg->next;
    msg->next->prev = queue->head;
    msg->next = nullptr;
    msg->prev = nullptr;
    return msg;
}

int main() {
    std::srand(std::time(nullptr));
    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&up, nullptr);
    pthread_cond_init(&down, nullptr);
    Init();

    pthread_t th_cohort;
    pthread_t th_coordinator;
    void *retval;

    pthread_create(&th_cohort, nullptr, RunCohort, nullptr);
    pthread_create(&th_coordinator, nullptr, RunCoordinator, nullptr);

    pthread_join(th_cohort, &retval);
    pthread_join(th_coordinator, &retval);

    return 0;
}