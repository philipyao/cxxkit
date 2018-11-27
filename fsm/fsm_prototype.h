#ifndef __FSM_PROTO_TYPE_H__
#define __FSM_PROTO_TYPE_H__

#include <stddef.h>
#include <string>

namespace kit {

static const int kMaxStateCount	= 64;
static const int kMaxFsmProtypeCount = 128;

class StaterInterface {
public:
	StaterInterface() = default;
	//No copying allowed
	StaterInterface(const StaterInterface& other) = delete;
	StaterInterface& operator=(const StaterInterface& other) = delete;

	//Virual base
    virtual ~StaterInterface() {}

	//Get stater state
	virtual int GetState() const = 0;

	//Do something when enter this state
	virtual void OnEnter(void* owner) = 0;

	//Handle message
	virtual int HandleMessage(void* owner, const char* msg_type, 
							   const void* message, size_t data_len) = 0;

	//Do something when leave this state
	virtual void OnLeave(void* owner) = 0;
};

struct FsmProtoType {
	std::string kind;
	int count;
	StaterInterface* states[kMaxStateCount];
	FsmProtoType() : count(0) {}
};

class StateMachine {
public:
	StateMachine() : owner_(nullptr), st_(nullptr), curr_proto_type_(nullptr) {}
	~StateMachine() {}

	static int Register(std::string kind, StaterInterface* states[], int state_count);

	int Init(std::string kind, void* owner, int start_state);

	void HandleMessage(const char* msg_type, const void* message, size_t data_len);

private:
	void* owner_;
	StaterInterface* st_;
	FsmProtoType* curr_proto_type_;

	static FsmProtoType proto_types_[kMaxFsmProtypeCount];
	static int num_proto_type_;
};

} // namespace kit


#endif //__FSM_PROTO_TYPE_H__

