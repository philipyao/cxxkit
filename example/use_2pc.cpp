#include "2pc/coordinator.h"
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <cinttypes>

class TestCoordinator : public kit::Coordinator {
public:
    TestCoordinator() : kit::Coordinator() {}
    ~TestCoordinator() {}

    void SendPrepare() {
        //set timeout
        uint64_t seqno = GetSeqno();
        //send prepare message to cohort
        printf("coordinator %" PRIu64 " send <PREPARE> message to cohort\n", seqno);
    }
    void SendCommit() {
        //set timeout
        uint64_t seqno = GetSeqno();
        //send commit message to cohort
        printf("coordinator %" PRIu64 " send <COMMIT> message to cohort\n", seqno);
    }
    void OnFinished() {
        uint64_t seqno = GetSeqno();
        printf("coordinator %" PRIu64 " 2pc transaction finished: callback\n", seqno);
    }
};

int main() {
    std::srand(std::time(nullptr));

    kit::Coordinator* bp = new TestCoordinator();
    bp->Start();

    if (std::rand() % 2) {
        int result = 1;
        bp->OnRecvPrepareAck(result);
    } else {
        bp->OnTimeout(NULL, 0);
    }

    if (std::rand() % 2) {
        bp->OnRecvCommitAck();
    } else {
        bp->OnTimeout(NULL, 0);
    }
    return 0;
}