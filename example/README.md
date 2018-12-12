

## use_fsm

使用基础状态机框架，管理较复杂的逻辑流程





## use_2pc

使用两阶段提交的框架，来实现基础事务机制。本使用示例：

* 使用了 pthread 线程来模拟协调者 coordinator 和 参与者 cohort

* 协调者 Coordinator 和 参与者 cohort 都需要实现发包(SendXXX)、管理超时(SetTimer, DelTimer)等接口，因为这是具体业务实现相关的，框架不负责实现

* OnBusinessXXX 系列接口是两阶段提交各阶段需要实现的接口，可以在这里执行具体的业务逻辑。本示例中仅仅打印了日志

* 通过 message 队列 + 条件变量，模拟了协调者和参与者之间的通信

  以下为收包逻辑

  ```c++
  pthread_mutex_lock(&lock);
  while ((incoming_msg = MsgDequeue(&upq)) == nullptr) {
      pthread_cond_wait(&up, &lock);
  }
  pthread_mutex_unlock(&lock);
  ```

  以下为发包逻辑

  ```c++
  pthread_mutex_lock(&lock);
  MsgEnqueue(&upq, msg);
  pthread_mutex_unlock(&lock);
  pthread_cond_signal(&up);
  ```

* 利用线程 + sleep来模拟了简单的定时器。删除定时器调用 pthread_cancel 取消超时线程即可

* 在发包接口中，有概率放弃发包来模拟丢包超时的情况
