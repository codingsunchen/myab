#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

//传给线程函数的参数是接下来这个线程要完成对服务器的http短连接数目
void *pthread_run(void *arg)
{
	int connect_num = (int)arg;
	//printf("connect_num = %d\n",connect_num);

	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(80);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	while(connect_num!=0)
	{
		int sockfd = socket(AF_INET,SOCK_STREAM,0);
		assert(sockfd != -1);
	
		int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
		//缺少一次判断，如果失败了应该重连。
		assert(res != -1);
		
		//组织http短链接请求报文
		char str1[4096] = {0};
		strcat(str1,"GET http://127.0.0.1/index.html HTTP/1.1\r\n");
		strcat(str1,"Host: 127.0.0.1\r\n");
		strcat(str1,"Connection: close\r\n");//短链接
		strcat(str1,"\r\n");
		//缺少一次判断，如果发送失败了应该重连。
		send(sockfd,str1,strlen(str1),0);

		char recv_buf[4096] = {0};
		//缺少一次判断，如果recv失败，返回-1，说明服务器并没有处理这个客户端的请求，那么说明这次http连接没有成功，就得重新连接。
		//如果返回值不是-1，那么再不停的去接收服务器给客户端发送过来的数据，直到服务器发送完数据后关闭这个连接。
		while(0 != recv(sockfd,recv_buf,4095,0))
		{}
		close(sockfd);
		connect_num--;
	}
	pthread_exit(NULL);
}

int* fun(int pth_num,int connect_num,int Concurrent_num)
{
	//申请数组arr用来记录每个子线程的id
	int *arr = (int*)malloc(sizeof(int)*pth_num);
	if(NULL == arr) exit(0);
	int i;

	//给每个子线程均分一定量的连接数
	int average = (connect_num-Concurrent_num)/pth_num;
	int *brr = (int*)malloc(sizeof(int)*pth_num);
	if(NULL == brr) exit(0);
	for(i=0;i<pth_num-1;i++)
	{
		brr[i] = average;
	}
	brr[pth_num-1] = (connect_num-Concurrent_num)-(pth_num-1)*average;

	//创建子线程用短链接完成参数给定的总连接数。
	pthread_t id;
	for(i=0;i<pth_num;i++)
	{
		if(-1 == pthread_create(&id,NULL,pthread_run,(int*)(brr[i])))
			exit(0);
		//记录产生的所有线程的id
		arr[i] = id;
	}
	free(brr);
	return arr;
}


int main()
{
	//线程数
	int pth_num;
	//总连接数
	int connect_num;
	//并发数
	int Concurrent_num;

	printf("输入线程数：\n");
	scanf("%d",&pth_num);

	printf("输入总连接数：\n");
	scanf("%d",&connect_num);

	printf("输入并发数：\n");
	scanf("%d",&Concurrent_num);
	
	if(pth_num<1 || connect_num<1 || Concurrent_num<0 || connect_num<Concurrent_num)
	{
		printf("参数输入有误\n");
		exit(0);
	}

	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(80);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int i;

	//开始计时
	struct timeval starttime,endtime;
	gettimeofday(&starttime,0);

	//保持并发数Concurrent_num个链接不要断开，用这个来模拟并发量,在还没有创建出子线程的时候就把并发量连接好，然后后边让子线程完成总连接数的要求。
	//最终通过计算在并发量的基础上完成总连接数的时间，然后平均求出服务器一秒所能完成的http连接数目。
	for(i=0;i<Concurrent_num;i++)
	{
		int sockfd = socket(AF_INET,SOCK_STREAM,0);
		assert(sockfd != -1);
	
		//如果给定的并发量过大而服务器没有这么多的资源去同时满足这么多的并发量,那么肯定是完成不了这么多的并发量的，那么在连接到最大的并发量时
		//就应该停止长连接，并将Concurrent_num更新为最大的并发量数。
		int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
		if(-1 == res)
		{
			printf("服务器不能达到该并发量,以下数据为服务器在最大并发量下的测压数据,服务器最大并发量为：%d\n",i+1);
			close(sockfd);
			Concurrent_num = i+1;
			break;
		}
		
		//组织长链接的http报文
		char str1[4096] = {0};
		strcat(str1,"GET http://127.0.0.1/n.html HTTP/1.1\r\n");
		strcat(str1,"Host: 127.0.0.1\r\n");
		//用长连接模拟并发量
		strcat(str1,"Connection: keep-alive\r\n");
		strcat(str1,"\r\n");
		send(sockfd,str1,strlen(str1),0);
		
		//将sockfd设置为非阻塞
		int old_option = fcntl(sockfd,F_GETFL);
		int new_option = old_option | O_NONBLOCK;
		fcntl(sockfd,F_SETFL,new_option);

		char recv_buf[4096] = {0};
		//对recv缺少一次判断，如果服务器太忙没有处理这个请求，那么recv返回值是-1，就说明这个连接没有成功，需要重新进行连接。
		while(recv(sockfd,recv_buf,4095,0) > 0)
		{}
	}

	//arr里面记录着所有子线程的id
	int *arr = fun(pth_num,connect_num,Concurrent_num);
	for(i=0;i<pth_num;i++)
	{
		pthread_join(arr[i],NULL);
	}
	free(arr);

	//计时器结束
	gettimeofday(&endtime,0);
	double timeuse = 1000000*(endtime.tv_sec-starttime.tv_sec)+endtime.tv_usec-starttime.tv_usec;
	//将时间转换为秒单位
	timeuse/=1000000;
	printf("总用时：%lf\n",timeuse);
	//用计时器计算出来的时间和总连接数 得出来一秒钟服务器所完成的http连接数。
	printf("平均一秒服务器所能连接的http的个数%d\n",(int)(connect_num/timeuse));
	printf("hello world\n");
	return 0;
}
