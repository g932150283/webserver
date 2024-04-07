# webserver

## 压测

 ab -n 1000000 -c 200 "http://172.30.72.21:8020/webserver"

```nginx 1个线程
Server Software:        nginx/1.18.0
Server Hostname:        172.30.72.21
Server Port:            80

Document Path:          /webserver
Document Length:        162 bytes

Concurrency Level:      200
Time taken for tests:   59.502 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      321000000 bytes
HTML transferred:       162000000 bytes
Requests per second:    16806.23 [#/sec] (mean)
Time per request:       11.900 [ms] (mean)
Time per request:       0.060 [ms] (mean, across all concurrent requests)
Transfer rate:          5268.36 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    5   1.5      5      29
Processing:     1    7   1.9      7      50
Waiting:        0    5   1.8      5      45
Total:          2   12   2.4     11      61

Percentage of the requests served within a certain time (ms)
  50%     11
  66%     12
  75%     12
  80%     13
  90%     14
  95%     16
  98%     18
  99%     21
 100%     61 (longest request)
```

```webserver 1个线程
Server Software:        webserver/1.0.0
Server Hostname:        172.30.72.21
Server Port:            8020

Document Path:          /webserver
Document Length:        140 bytes

Concurrency Level:      200
Time taken for tests:   30.773 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      256000000 bytes
HTML transferred:       140000000 bytes
Requests per second:    32496.27 [#/sec] (mean)
Time per request:       6.155 [ms] (mean)
Time per request:       0.031 [ms] (mean, across all concurrent requests)
Transfer rate:          8124.07 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    2   0.8      2      32
Processing:     1    4   1.4      4      35
Waiting:        0    3   1.2      3      34
Total:          2    6   1.7      6      38

Percentage of the requests served within a certain time (ms)
  50%      6
  66%      6
  75%      7
  80%      7
  90%      8
  95%      9
  98%     10
  99%     11
 100%     38 (longest request)
```

```
Server Software:        webserver/1.0.0
Server Hostname:        172.30.72.21
Server Port:            8020

Document Path:          /webserver
Document Length:        140 bytes

Concurrency Level:      200
Time taken for tests:   47.636 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      256000000 bytes
HTML transferred:       140000000 bytes
Requests per second:    20992.63 [#/sec] (mean)
Time per request:       9.527 [ms] (mean)
Time per request:       0.048 [ms] (mean, across all concurrent requests)
Transfer rate:          5248.16 [Kbytes/sec] received
```

## 项目路径

bin --  二进制

bulid -- 中间文件路径

cmake -- cmake函数文件夹

CMakeList.txt -- cmake定义文件

lib -- 库得输出路径

Makefile -- 

sylar -- 源代码路径

tests -- 测试代码

## 日志系统

服务器出问题、做统计

1)
    Log4J


    Logger(定义日志类别)
    -Formatter(日志格式)

    Appender(日志输出地方)（控制台、文件、专门日志收集系统）


    框架级log用唯一标识区分，业务级用另外名称定义
    - 可控


## 配置系统

Config --> Yaml

yamp-cpp

```cpp
YAML::Node node = YAML::LoadFile(filename)
node.IsMap()
for(auto it = node.begin();
        it != node.end(); it++){
            it->first, it->seconde
        }

node.IsSequence()
```

配置系统原则：约定优于配置

```cpp

template<T, FromStr, ToStr>
class ConfigVar;

template<F, T>
LexicalCast;

//容器片特化 vector, list, set, map, unordered_set, unordered_map
// map、unordered_map 需要支持 key = std::string
// Config::Lookup(key), key相同， 类型不同

/*
容器偏特化（template specialization for containers）是C++模板编程中的一个概念。
它指的是对特定类型的模板进行特殊处理或定制，以满足特定的需求。
代码中，存在两个类模板的片特化：

LexicalCast<std::string, std::vector<T>>：
这是对 LexicalCast 模板的特殊版本，用于将 YAML 格式的字符串转换为 std::vector<T> 类型的对象。

LexicalCast<std::vector<T>, std::string>：
这是另一个 LexicalCast 模板的特殊版本，用于将 std::vector<T> 类型的对象转换为 YAML 格式的字符串。

在这两种情况下，为了更好地处理特定的类型转换，对通用的 LexicalCast 模板进行了特殊化。
这使得可以为这两个特殊情况提供定制的实现，以满足程序的需求。
这种特殊化允许根据不同的类型执行不同的操作，提高了代码的灵活性和可读性。
*/

```

自定义类型，需要实现webserver::LexicalCast,偏特化
实现后，可以支持Config解析自定义类型
自定义类型可以和常规stl容器一起使用

配置的事件机制
当一个配置项发生修改的时候，可以反向通知对应的代码，回调

### 日志系统整合配置系统

```yaml
logs:
    - name: root
      level: (debug,,info, warn, error, fatal)
      formatter: "%d%T%p%T%t%m%n" 
      appender:
            - type: (StdoutLogAppender, FileLogAppender)
              level: (debug, ...)
              file: /logs/xxx.log

```

```cpp

webserver::Logger g_logger = webserver::LoggerMgr::GetInstance()->getLogger(name);
WEBSERVER_LOG_INFO(g_logger) << "xxxx log";

```

```cpp
// 保证log唯一性
static Logger::ptr g_log = WEBSERVER_LOG_NAME("system");
// m_root, m_system->m_root
// 当logger的appenders为空，使用root写logger
```
```cpp
// 定义LogDefine LogAppenderDefine，偏特化 LexicalCast
// 实现日志配置解析
```

```cpp

```

## 线程库封装

线程 C++11
互斥量 pthread线程库
既能用到新特性，又可以兼顾性能

pthread实现pthread_creatre
互斥量 mutex 
信号量 semaphore
C++11中的互斥量没有读写分离，高并发情况下，写少读多，将读写分离，提升性能

信号量用在pthread生成线程中，在线程构造函数中，确保在出线程构造函数之前，要启动的线程就一定启动。

与Log整合，Logger, Appender

Spinlock替换Mutex
写文件，周期性，reopen

与Config整合

```bash
ps uax | grep thread
```
这个命令的意思是列出所有进程 (ps uax)，然后使用 grep 过滤输出，只显示包含单词 "thread" 的行。

```bash
top -H -p <进程ID>
```
top 命令用于显示系统中运行的进程和系统资源的使用情况。-H 选项表示显示线程信息，-p 选项用于指定要监视的进程 ID。如果你想要监视特定进程的线程，你需要提供一个有效的进程 ID。例如：

![alt text](pic/thread01-1.png)

![alt text](pic/thread01-2.png)



## 协程库封装

将异步操作封装成同步

定义协程接口
ucontext_t
macro

协程比线程更轻量级
用户态的线程，可创建的数量是线程的成千上万倍
轻量级，切换速度快，操作权掌握在用户手中

```
Fiber::GetThis()
thread->main_fiber <--> sub_fiber
            ↑
            |
            ↓
        sub_fiber
```


协程调度模块scheduler
```   
          1 - N      1 - M       
scheduler  --> thread --> fiber

1. 线程池， 分配一组线程
2. 协程调度器，将协程指定到相应的线程上去执行

m_threads 线程池
<function<void()>, fiber, threadid> m_fibers  协程队列

schedule(func/fiber)

start()
stop()  协程调度器所有任务结束后退出
run()  核心 协程和线程

run() 
1.设置当前线程的scheduler
2.设置当前线程执行run方法的fiber
3.协程调度循环while(true)
    3.1 协程消息队列是否有 任务
    3.2 无任务执行，执行idle

```


### IO协程调度模块
在协程调度模块的基础上，封装了epoll。支持对IO事件的调度功能，可以为socket句柄添加读事件(EPOLLIN)和写事件(EPOLLOUT)，并且支持删除事件功能。IOManager主要通过FdContext结构体存储文件描述符fd, 注册的事件event，执行任务cb/fiber，其中fd和event用于epoll_wait，cb/fiber用于执行任务。
当有任务时，使用管道pipe来唤醒epoll_wait()先执行其他任务。

```
IOManager(epoll) --->Scheduler
            |
            |
            ↓
           idle(epoll_wait)

信号量
PutMessage(msg, ) + 信号量1, single()
message_queue
     |
     | ----- Thread
     | ----- Thread
           wait()-信号量1, RecvMessage(msg, )

异步IS，等待数据返回。 epoll_wait

epoll_create, epoll_ctl, epoll_wait
```

```
Timer -> addTimer() --> cancel()
获取当前的定时器触发离现在的时间差
返回当前需要触发的定时器
```


```
                [Fiber]                   [Timer]
                   ↑N                       ↑
                   |                        |
                   |1                       |
                [Thread]               [TimerManager]
                   ↑M                       ↑
                   |                        |
                   |1                       |
                [Scheduledr] <----  [IOManager(epoll)]

```

## HOOK

sleep
usleep

socket相关 : socket, connet, accept
io相关 : read, wirte, send, recv
fd相关 : fcntl, ioctl

## socket函数库

              [UnixAddress]
                    |
                ---------                      | - [IPv4Address]
                |Address|  --- [IPAddress] --- |
                ---------                      | - [IPv6Address]
                    |
                    |
                ---------
                 |Socket|
                ---------

connetc
accept
read/wirte/clses   


## 序列化bytearry

write(int, float, ...)
read(int, float, ...)
                
## http协议开发

主要封装HTTP请求（class HttpRequest）和HTTP响应报文（class HttpResponse）

使用状态机解析报文格式，保存到请求和响应报文对象中。

请求报文格式
```

GET / HTTP/1.1  #请求行
Host: www.baidu.com   #主机地址
Connection: keep-alive   #表示TCP未断开
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64;x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.149 Safari/537.36   #产生请求的浏览器类型
Sec-Fetch-Dest: document
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
Sec-Fetch-Site: none
Sec-Fetch-Mode: navigate
Sec-Fetch-User: ?1
Accept-Encoding: gzip, deflate, br
Accept-Language: zh-CN,zh;q=0.9
Cookie: ......    #用户安全凭证

```

应答报文格式


```
HTTP/1.1 200 OK
Content-Type: text/html; charset=UTF-8
Date: Mon, 05 Jun 2023 06:53:13 GMT
Link: <http://www.sylar.top/blog/index.php?rest_route=/>; rel="https://api.w.org/"
Server: nginx/1.12.2
Transfer-Encoding: chunked
X-Powered-By: PHP/7.0.33
Connection: close
Content-length: 45383
​
<!DOCTYPE html>
<html lang="zh-CN" class="no-js">
<head>
<meta charset="UTF-8">
```
HTTP/1.1 - API

HttpRequest
HttpResponse


GET / HTTP/1.1
host: www.baidu.com

HTTP/1.0 200 OK
Pragma: no-cache
Content-Type: text/html
Content-Length: 14988
Connection: close

url: http://www.baidu.com:80/page/xxx?id=10&v=20#fr
    协议 : http
    host : www.baidu.com
    port : 80
    path : xxx
    param : id=10&v=20
    fragment : fr

ragel mongrel2

## TcpServer封装
基于TcpServer实现了一个EchoSever

## Stream 针对文件/socket封装
read/write/readFixeSize/writeFixeSize

## HttpServer模块

HttpServer模块概述
封装HttpSession接收请求报文，发送响应报文，该类继承自SocketStream。
封装Servlet,虚拟接口，当Server命中某个uri时，执行对应的Servlet。
封装HttpServer服务器，继承自TcpServer。


HttpSession/HttpConnection
Server accept socket -> session
client connect socket -> connection



HttpServer : TcpServer

        Servlet <--- FunctionServlet
           |
           |
           ↓
     ServletDispatch


## HttpConnection模块

class HttpConnection继承自class SocketStream，发送请求报文，接收响应报文。
封装struct HttpResultHTTP响应结果
实现连接池class HttpConnectionPool，仅在长连接有效时使用。
封装class URI，使用状态机解析URI。

URI格式

```

foo://user@example.com:8042/over/there?name=ferret#nose
_/   ___________________/_________/ _________/ __/
 |              |              |            |        |
scheme      authority         path        query   fragment
 |   __________________________|__
/ \ /                             \
urn:example:animal:ferret:nose
​
authority   = [ userinfo "@" ] host [ ":" port ]


     foo://user@webserver.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment



```




## 总结

### 日志模块
Logger, LoggerAppender, Formatter, LogEvent
LoggerAppender，有几种实例，std、文件，后续优化文件输出，如一天的日志或者限制日志大小，可通过继承LogFileAppender实现
### 配置模块
ConfigBase, ConfigVar, Config
模板做序列化和反序列化
设计思想：约定优于配置
在声明一条配置时，已经设置好了里面的一些默认参数，绝大多数情况下使用系统默认参数已经足够，修改参数的场景比较少
一个服务器跑起来有100项配置项，其中90项可以用默认的来决定，只有10种左右需要人工去调，省去人为配置环节

另外，配置项除了支持基础类型int float string, 还支持集合类型set\mat,还支持自定义类型，需要自己实现tostring,fromstring
配置模块采用yaml格式配置，要能支持从yaml字符串转成对应的结构体就可以
yaml格式简洁清晰 xml起始结束标签 json一堆标签
修改tostring,fromstring的模板类


### 线程模块
Thread, RWMutex, Mutex, Semphore, Spinlock
主要封装pthread, 因为c++11很多东西不是很好用，只有互斥锁，没有读写分离锁，没有自旋锁

### 协程模块
Fiber
在两个执行栈之间切换，cpp在函数中执行另一个函数
协程是一个函数执行到一半的时候切入到另一个函数的某个位置

### 协程调度模块
Scheduler
协程自己只是一个原子的执行单元，像线程一样new一个线程，线程结束后可能返回到执行线程的地方去
多线程多协程在跑起来时，有一个协程调度器分配，现在应该轮到那个协程执行，执行到一半让出执行时间，yeild之后，我的schedul知道你让出执行时间了，
何时再唤醒此协程，就有协程调度器决定

协程调度器以人为方式，实现了类似于系统的线程调度器。相对线程来说，就是线程调度器。一个进程有十个线程，我怎么知道哪个线程应该执行，此时线程调度器，系统进行调度

协程是我们自己创造出来的一种抽象概念，利用协同协程的东西，封装成抽象概念，这个抽象概念需要我们自己来管理，所以需要有一个协程调度器来负责协程的生命周期，创建、销毁等等

### IO协程调度模块
IOManager(epoll), TimerManager
协程调度器只是一种协程调度器的封装，他只封装了我有协程过来，我怎么样让协程在线程之间切换，或者让协程之间的上下文状态切换正常
真正在服务器框架中，不是所有的协程都是可运行状态。 有的协程在做异步IO，此时协程状态不是马上执行 ，并不是把时间直接让给协程调度器，而是把协程和某一个事件关联，当那个事件触发的时候，才把要触发的协程，放到协程调度器中执行。

如果所有的协程都在协程调度器中直接调度的话，说明服务器框架一旦没有任务做的时候，他可能没有协程需要去执行了 
IO协程调度器，服务器不是每时每刻都很繁忙，有时就是说服务器刚启动的时候或者运行的时候没有连接过来，就是没事干，协程调度器没有任务做，应该在某个地方去wait一个事件，让出整个的执行时间，直到有连接来或者有数据来，才唤醒我要做的工作流程。让cpu时间比较平滑；也不会说因为sleep这种情况影响一些现有的事件没能触发

IO协程调度器基于scheduler和epoll来做，epoll来封装io事件和执行定时器功能
epoll_wait的时间戳相当于超时时间，利用这个时间戳实现一个简单的定时器（ms）
支持两种类型定时器和两种运行模式
按一次性触发的，30s之后执行一次任务，加一次性定时器
循环定时器，每30s执行一次
一定执行，时间一到立即执行
条件执行，条件在30s后消失，就没必要执行

### Hook模块
FdManager
做文件句柄相关操作
主要是让我们在做后续网络编程时，我想用同步的方法去写socket，但是我又希望有异步的性能
socket在收数据时可能远端永远不发数据，然后receive就一直阻塞。如果不用hook，按照正常的模式，也不用异步的话，他的receive会阻塞整个线程
一般情况下，程序的线程是有限的，当你阻塞了整个线程那就意味着，如果有其他连接过来，他就没地方执行了
所以通过hook的时候，当他去执行receive的时候，需要阻塞的时候，并不是阻塞整个线程，而是把他自己的协程阻塞了
这样线程就可以让给有需要执行任务的协程了，然后如果数据回来了之后，我们利用IOmanager的方法，你关注的那个读事件或者写事件回来了，我就唤醒你那个协程，
让你的协程进入协程调度器，让他执行起来，这样数据来了的时候，我们就可以从他阻塞的位置返回出来，写代码的时候根本不需要知道，你这个地方是异步还是同步，你只需要知道你是按同步的方式写，真正的执行起来，能发挥异步的性能

### Socket模块
Address, Socket
Address基类，哪个协议簇，哪种类型的address；继承下来分了三类ip  unix unknown
Socket封装了一系列的API，把socket常规的操作send receive这些操作全部封装好了，封装到socket里面，后续的操作直接操作socket就可以

### ByteArray模块
ByteArray
网络操作的过程中，发送的数组一般是二进制的，二进制的数据涉及到字节序的问题，涉及到压缩的问题或者有涉及到一些写入数据解析的问题
实现了一系列和基础数据类型有关的操作，比如写入基础数据类型int float string，针对int类型，写入分两种类型，一种是按固定长度，你的int4个字节，那我就4个字节；另外一种是压缩int的方式，比如我这个int虽然占4个字节，但是我这个int绝大多数情况下是100以内，100以内用一个char就能描述出来，一个char上限是256，我用一个位来描述我是否有下一个字节，去掉这个字节还有127；如果用int来声明对象，但绝大多数情况下是小于100，我们在传输过程中只需要传输一个字节就够了。

提供了写方法，还要提供相应的读方法，因为从远端接受的数据也是一个二进制，要解析里面的内容，比如说我们约定好你传的是int，用相应readint的方法去读出我想要的数据

### Stream模块
Stream， SocketStream
SocketStream，Stream针对文件类型的，socket是一种文件类型，后续专门声明一种文件的stream，没有屏蔽文件还是socket的区别，因为有的时候直接通过文件来数据写到socket，或者socket来数据写道文件，或者socket来数据写道socket。抽象成统一的stream时，只需要操作stream的api，从一个stream的read写到另一个stream的write，并不需要关系这个stream是socket还是文件，通过这种方式，让后续应用变简单。现有的库中stream相关的继承，stream-socketstream-(httpconnection\httpsession)
stream针对连接的socket，一方面accept产生的socket，一方面是主动连接请求的socket

### TcpServer模块
TcpServer
针对socket server类型进行封装的，我这个socket是做server，所以我要先bind-listen-accept, 专门用于产生连接的，去连接外部请求的，所以他封装了一些很通用的服务器架构，比如说通过一个地址绑定到一个地址，然后listen他，然后通过accept产生连接，产生链接之后怎么分配协程，或者协程调度器让他去执行我这个新产生的socket的一些后续操作，比如收包、发消息等等。

### HttpServer模块
HttpSergver
继承自TcpServer，区别是链接上的socket要执行http协议，所以每个新连接创建过来时，专门分配一个stream类（中有httpsession，httpsession继承自stream，他每个连接产生过来的时候，会产生一个httpsession，httpsession负责整个连接的请求，比如说你怎么发http协议报文给我，我就解析他，响应一些东西等等）

### Servlet模块
Servlet， NotFoundServlet， FunctionServlet
serverlet就是抽象服务器端的逻辑，至少是抽象http服务端的逻辑，因为我们httpserver是做成一个比较通用的，外部使用httpserver只要来操作api就可以。比如说我开了一个httpserver端口，要接受某些请求，某些请求命中某个url，要执行某个函数。实现某个函数具体逻辑抽象在servlet中，可以写一个类继承这个servlet，再跟httpserver注册上这个路径， 某个路径映射到某个servlet， 以及某个路径映射到某个函数，都可以通过httpserver的方法来实现。

http模块重要的模块是协议解析，主要http协议解析用的range， range是一个有限状态机的方式，通过自定义的语法生成一些有限状态机的程序代码，执行性能强悍，
基于httpserver的range源码修改，并不支持分段解析协议，未必支持中文

### HttpConnection模块
HttpConnection, HttpConnectionPool
封装连接，发请求。正常http请求比较麻烦，发起很多步骤，一个url，解析发给某个地址，将这个地址转成address类，能不能转成ip地址，转不成说明有问题，转成之后，连接上，连上之后把url的文本内容，转换成http的协议文本，然后再发过去，发过去之后，收响应，收到响应之后，将响应反馈出来。  整个流程如果不进行封装的话是比较麻烦的，通过httpconnection封装了几个便捷的方法，比如说发请求，直接传url，传head就传head,传body就传body，设置超时时间，把这些参数设置好之后就可以很简单地封装底层复杂地操作，发送出去，数据回来时再唤醒

HttpConnectionPool针对长连接场景，长连接场景有的时候很大情况，服务器跑起来后，我们要请求和连接的地方一般都很固定，此时为了优化性能，并不是说每次请求的时候都创建一个连接，请求完之后再释放这个连接。当请求的频次非常高时，来回的创建和回收socket的耗时大，成本高，基于这种情况封装了httpconnectionpool。它专门针对某个地址创建一个连接池，使用完之后，并不会马上释放这个连接，而是放在连接池中，按照一定的生存周期或者运行规则，决定这个连接是否有效，是否需要放进去等等。









