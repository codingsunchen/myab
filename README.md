mytest apache bench
多线程http请求，测试服务器qps

项目描述：
本测压工具是一款测试
web 服务器（如 Nginx 、 Apache ）压力的测试工具，模拟 ApacheBench ab
采用 libevent 和非阻塞 socket 来处理并发量，将总并发量和总连接数都平均分配给各个子线程来完成。子线程
通过创建并发量个数个 struct connection 来动态的保持并发个数，当有一个连接断开则立即重新启动一个新的
连接保持并发量。为每一个 connection 都枚举了可能出现的四种连接状态（ STAT_UNCONNECTED,
STAT_CONNECTING, STAT_CONNECTED, STAT_READ ），根据 connection 所处于的状态分别在 libevent 上
关注对应的事件进行监听，当有事件发生时处理 connection 完成相应的动作。当完成了给定的总连接数后，退
出测试过程，用计时器对整个过程进行计时，求出在整个过程中 web 服务器的压力。使用的第三方库 libevent 。
系统功能：
测试 web 服务器在给定并发量和总连接数情况下的压力，即每秒 web 服务器所能处理的连接数。
