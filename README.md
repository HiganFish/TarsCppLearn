## 协程机制

tars中提供了协程机制，自然需要好好了解一番。

一个线程对应多个协程，但是时时刻刻也只能有一个协程在运行。所以重点是一个线程如何在多个协程之间切换。

```c++
typedef void*   fcontext_t;

struct transfer_t {
    fcontext_t  fctx;
    void    *   data;
};

extern "C" transfer_t jump_fcontext( fcontext_t const to, void * vp);
extern "C" fcontext_t make_fcontext( void * sp, std::size_t size, void (* fn)( transfer_t) );
```

transfer_t结构体 fctx指向的是 上下文，data则指向的是jump_fcontext中的vp这个自定义参数

上下文可以理解为是CPU寄存器的保存，通过恢复和保存CPU寄存器的状态实现协程之间的切换。



make_fcontext用于创建协程，需要提供协程栈地址和入口函数tn。make_fcontext会在协程栈的起始位置以某种格式创建保存有上下文的结构体，保存了函数的入口地址tn和函数的返回地址（协程的返回地址一般为exit 即退出）。上下文结构体是什么样不重要，只需要编码的make_fcontext和解码的jump_fcontext知道即可，上下文结构体只是作为CPU寄存器的中转保存位置。



jump_fcontext用于协程切换，vp是用户自定义参数，会被保存到transfer_t的data中，函数首先保存当前CPU的状态，然后恢复to_context中保存的状态，这样就实现了协程的切换，切换的时候会把保存的协程状态存储在transfer_t的fctx中，然后整个transfer_t结构体会作为参数传递给fn，用于fn后续切换回原本的协程。



tars中创建协程之后没有立即执行，而是将其挂在了就绪协程链表中，等待统一的时机去批量处理就绪的协程，就绪的协程处理完毕后会把自己设置为空闲状态，挂在空闲协程链表上，等待创建新协程的时候复用。



协程的调度类TC_CoroutineScheduler每个线程都有自己的调度类。这样就能防止冲突



## 事件处理机制 - Handle机制

tars中提供了Handle基类，其中事件处理的函数定义为了虚函数。可以根据自己的实际需要继承如ServantHandle，用于处理网络请求线程。



一个BindAdapter中存在多个ServantHandle，每个ServantHandle在`TC_EpollServer::initHandle`将会

- NET_THREAD_MERGE模式下 被分配一个NetThread线程
- NET_THREAD_QUEUE模式下 ？





## 事件处理模型

总的来看muduo中的一些思想，这里同样是适用的。细节方面区别还是会有的。

`NET_THREAD_MERGE_HANDLES_THREAD` 这个模式与muduo的最为相似。



```
//独立网路线程 + 独立handle线程: 网络线程负责收发包, 通过队列唤醒handle线程中处理
NET_THREAD_QUEUE_HANDLES_THREAD   = 0,

//独立网路线程组 + 独立handle线程: 网络线程负责收发包, 通过队列唤醒handle线程中处理, handle线程中启动协程处理
NET_THREAD_QUEUE_HANDLES_CO       = 1,

//合并网路线程 + handle线程(线程个数以处理线程配置为准, 网络线程配置无效): 连接分配到不同线程中处理(如果是UDP, 则网络线程竞争接收包), 这种模式下延时最小, 相当于每个包的收发以及业务处理都在一个线程中
NET_THREAD_MERGE_HANDLES_THREAD        = 2,
   
//合并网路线程 + handle线程(线程个数以处理线程配置为准, 网络线程配置无效): 连接分配到不同线程中处理(如果是UDP, 则网络线程竞争接收包), 每个包会启动协程来处理
NET_THREAD_MERGE_HANDLES_CO            = 3
```

根据这里选项的区别存在 **两个重要区分点**



### NET_THREAD_MERGE

#### THREAD

`TC_EpollServer::waitForShutdown` 函数负责epoll的初始化和服务器的启动等操作

```
_epoller = new TC_Epoller();

_epoller->create(10240);
_epoller->setName("epollserver-epoller");

initHandle();
createEpoll();
startHandle();
```

`create` 会创建epollfd，初始化好epoll_event的数组，同时默认情况下会添加一个notify用于从epoll_wait及时返回去处理事件队列中其他内容。



`initHandle` 则会根据事件处理模型的类型做对应的初始化。当前模式下将会给每个BindAdapter的ServantHandle创建一个NetThread。所有创建的线程都会被set到BindAdapter中（因为一个BindAdapter通常对应多个ServantHandle）。



同时设定线程的初始化函数和handle函数。线程和协程的区别是设定的handle函数不同，一个是handleOnceCoroutine，另一个是handleOnceThread。**区分点1**



`createEpoll`中为每个NetThread 根据最大连接数量 初始化ConnectionList。



`startHandle`中线程模式下会直接start NetThread，协程模式下调用exec函数 **区分点2**

- start  直接使用了std::thread创建了线程 实际入口点为各个实现的run函数。



至此主线程根据BindAdapter的ServantHandle数量创建了对应数量的线程startHandle函数运行后，所有子线程都开始运行各自的实际run函数。







#### Coroutine

### NET_THREAD_QUEUE

#### Thread

#### Coroutine















[点我查看中文版](README.zh.md)

This project is the source code of the Tars RPC framework C++ language.

Directory |Features
------------------|----------------
[servant](https://github.com/TarsCloud/TarsCpp/tree/master/servant)      |Source code implementation of C++ language framework rpc
[tools](https://github.com/TarsCloud/TarsCpp/tree/master/tools)        |Source code implementation of C++ language framework IDL tool
[util](https://github.com/TarsCloud/TarsCpp/tree/master/util)         |Source code implementation of C++ language framework basic tool library
[examples](https://github.com/TarsCloud/TarsCpp/tree/master/examples)     |Sample code for the C++ language framework, including: quick start examples, introduction to promise programming, examples of pressure test programs
[unittest](https://github.com/TarsCloud/tars-unittest/tree/master)      |Unittest of tarscpp rpc framework base on GoogleTest test framework. You can download it as a git submodule using 'git submodule init unittest;git submodule update' command.
[test_deprecated](https://github.com/TarsCloud/TarsCpp/tree/master/test)         |Test procedures for various parts of the C++ language framework, deprecated in current.
[docs](https://github.com/TarsCloud/TarsCpp/tree/master/docs)         |Document description
[docs-en](https://github.com/TarsCloud/TarsCpp/tree/master/docs-en)      |English document description

Dependent environment

Software |version requirements
------|--------
linux kernel:   |	2.6.18 and above
gcc:          	|   4.1.2 and above glibc-devel
bison tool:     |	2.5 and above
flex tool:      |	2.5 and above
cmake:       	|   3.2 and above
mysql:          |	4.1.17 and above

Compile and install
```
git clone https://github.com/TarsCloud/TarsCpp.git --recursive
cd TarsCpp
cmake .
make
make install
```

Detailed [reference](https://tarscloud.github.io/TarsDocs_en/)
