#ifndef __2PC_DEF_H__
#define __2PC_DEF_H__

namespace kit {

enum VoteResult {
    kVoteReusultInvalid,
    kVoteResultCommit,
    kVoteResultAbort,
}; 

enum CmdType {
    kCmdGlobalCommit,
    kCmdGlobalAbort,
};

}

#endif //__2PC_DEF_H__