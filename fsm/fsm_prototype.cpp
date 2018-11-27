#include "fsm_prototype.h"
#include  <string.h>
#include "errcode.h"

namespace kit {

#define ARR_LEN(arr) sizeof(arr)/sizeof(arr[0])

FsmProtoType StateMachine::proto_types_[kMaxFsmProtypeCount];
int StateMachine::num_proto_type_ = 0;

int StateMachine::Register(std::string kind, StaterInterface* states[], int state_count) { 
	if (kind.empty() || !states || (state_count <= 0)) {
		return kErrcodeInvalidArgs;
	}
	for (int i = 0; i < state_count; ++i) {
		auto st = states[i];
		if (!st) {
			return kErrcodeNullPtr;
		}
		if (st->GetState() <= 0) {
			return kErrcodeFsmInvalidState;
		}
	}
	for (int i = 0; i < num_proto_type_; ++i) {
		if (proto_types_[i].kind == kind) {
			return kErrcodeFsmDuplicatedKind;
		}
	}

	if (num_proto_type_ >= static_cast<int>(ARR_LEN(proto_types_))) {
		return kErrcodeMeetUplimit;
	}
	auto pt = &proto_types_[num_proto_type_];
	pt->kind = kind;
	pt->count = state_count;
	memcpy(pt->states, states, sizeof(states[0]) * state_count);
	++num_proto_type_;

	return kErrcodeOK;
}

int StateMachine::Init(std::string kind, void* owner, int start_state) {
	if (!owner) {
		return kErrcodeNullPtr;
	}
	FsmProtoType* p = nullptr;
	for (int i = 0; i < num_proto_type_; ++i) {
		if (proto_types_[i].kind == kind) {
			p = &proto_types_[i];
			break;
		}
	}
	if (!p) {
		return kErrcodeNotFound;
	}
	for (int i = 0; i < p->count; ++i) {
		if (p->states[i]->GetState() == start_state) {
			st_ = p->states[i];
			break;
		}
	}
	if (!st_) {
		return kErrcodeNotFound;
	}
	curr_proto_type_ = p;
	owner_ = owner;
	st_->OnEnter(owner);
	return kErrcodeOK;
}

void StateMachine::HandleMessage(const char* msg_type, const void* message, size_t data_len) {
	int ret_state = st_->HandleMessage(owner_, msg_type, message, data_len);
	if (ret_state == 0 || ret_state == st_->GetState()) {
		//state not change
		return;
	}
	kit::StaterInterface* new_st = nullptr;
	for (int i = 0; i < curr_proto_type_->count; ++i) {
		if (curr_proto_type_->states[i]->GetState() == ret_state) {
			new_st = curr_proto_type_->states[i];
			break;
		}
	}
	if (!new_st) {
		//TODO Log
		return;
	}
	st_->OnLeave(owner_);
	st_ = new_st;
	st_->OnEnter(owner_);
}


} // namespace kit
