#include "fsm/fsm_prototype.h"
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

namespace {

enum MyStateType {
	kStateFoo		= 1,
	kStateBar		= 2,
	kStateHello		= 3,
	kStateWorld		= 4,
};

const std::string sm_name("test");

int RandomState() {
	return std::rand() % 5;
}

} // namespace


class StaterFoo : public kit::StaterInterface {
public:
	int GetState() const { return kStateFoo; }

	//Do something when enter this state
	void OnEnter(void* owner) {
		printf("%d: StateMachine enter: owner<%p>, state<%d>\n", __LINE__, owner, GetState());
	}

	//Handle message
	int HandleMessage(void* owner, const char* msg_type, const void* message, size_t data_len) {
		printf("StateMachine handle message: owner<%p>, state<%d>, msg_type<%s>, data_len<%zd>\n", 
				owner, GetState(), msg_type, data_len);
		return RandomState();		
	}

	//Do something when leave this state
	void OnLeave(void* owner) {
		printf("StateMachine leave: owner<%p>, state<%d>\n", owner, GetState());
	}
};

class StaterBar : public kit::StaterInterface {
public:
	int GetState() const { return kStateBar; }

	//Do something when enter this state
	void OnEnter(void* owner) {
		printf("%d: StateMachine enter: owner<%p>, state<%d>\n", __LINE__, owner, GetState());
	}

	//Handle message
	int HandleMessage(void* owner, const char* msg_type, const void* message, size_t data_len) {
		printf("StateMachine handle message: owner<%p>, state<%d>, msg_type<%s>, data_len<%zd>\n", 
				owner, GetState(), msg_type, data_len);
		return RandomState();		
	}

	//Do something when leave this state
	void OnLeave(void* owner) {
		printf("StateMachine leave: owner<%p>, state<%d>\n", owner, GetState());
	}
};


class StaterHello : public kit::StaterInterface {
public:
	int GetState() const { return kStateHello; }

	//Do something when enter this state
	void OnEnter(void* owner) {
		printf("StateMachine enter: owner<%p>, state<%d>\n", owner, GetState());
	}

	//Handle message
	int HandleMessage(void* owner, const char* msg_type, const void* message, size_t data_len) {
		printf("StateMachine handle message: owner<%p>, state<%d>, msg_type<%s>, data_len<%zd>\n", 
				owner, GetState(), msg_type, data_len);
		return RandomState();
	}

	//Do something when leave this state
	void OnLeave(void* owner) {
		printf("StateMachine leave: owner<%p>, state<%d>\n", owner, GetState());
	}
};

class StaterWorld : public kit::StaterInterface {
public:
	int GetState() const { return kStateWorld; }

	//Do something when enter this state
	void OnEnter(void* owner) {
		printf("StateMachine enter: owner<%p>, state<%d>\n", owner, GetState());
	}

	//Handle message
	int HandleMessage(void* owner, const char* msg_type, const void* message, size_t data_len) {
		printf("StateMachine handle message: owner<%p>, state<%d>, msg_type<%s>, data_len<%zd>\n", 
				owner, GetState(), msg_type, data_len);
		return RandomState();		
	}

	//Do something when leave this state
	void OnLeave(void* owner) {
		printf("StateMachine leave: owner<%p>, state<%d>\n", owner, GetState());
	}
};

class MyOwner {
public:
	int Init() {
		auto sm = new kit::StateMachine();
		int ret = sm->Init(sm_name, this, kStateFoo);
		if (ret != 0) {
			delete sm;
			sm = nullptr;
			return ret;
		}
		sm_ = sm;
		return 0;
	}
	void Handle() {
		sm_->HandleMessage("message_type", nullptr, 10);
	}

private:
	kit::StateMachine* sm_;
};

int main() {
	std::srand(std::time(nullptr));

	kit::StaterInterface* states[] = {
		new StaterFoo(), 
		new StaterBar(), 
		new StaterHello(),
		new StaterWorld(),
	};	
	int ret = kit::StateMachine::Register(sm_name, states, sizeof(states)/sizeof(states[0]));
	if (ret != 0) {
		printf("register StateMachine <%s> error: %d\n", sm_name.c_str(), ret);
		return 0;
	}
	printf("register StateMachine <%s> success\n", sm_name.c_str());

	MyOwner* mo = new MyOwner();
	ret = mo->Init();
	if (ret != 0) {
		printf("create sm: %p error %d\n", mo, ret);
		return 0;
	}
	printf("create sm: %p success\n", mo);

	while (true) {
		mo->Handle();
		usleep(2000000);
	}

	delete mo;
	mo = nullptr;

	return 0;
}


