#ifndef __INCLUDED_CONNECTION_MANAGER_H_
#define __INCLUDED_CONNECTION_MANAGER_H_

#define MAX_CLIENT_NUM 1000

#define RECV_BUF_SIZE 4096
#define SEND_BUF_SIZE 4096

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12) 

#include "ConnectionLayer.h"

struct SConnectionAddr
{
	SConnectionAddr():
	/*qwToken(0),*/wPort(0)
	{
	}
	//UINT64 qwToken;
	WORD wPort;
	IN_ADDR sin_addr;
};

class ConnectionAddrComp
{
public:
	bool operator()(const sockaddr_in &pSH1, const sockaddr_in &pSH2) const
	{
		/*return pSH1->qwToken > pSH2->qwToken;*/
		if (pSH1.sin_addr.S_un.S_addr < pSH2.sin_addr.S_un.S_addr)
		{
			return false;
		}
		else if (pSH1.sin_addr.S_un.S_addr == pSH2.sin_addr.S_un.S_addr)
		{
			if (pSH1.sin_port <= pSH2.sin_port)
			{
				return false;
			}
			//else if (pSH1->wPort == pSH2->wPort)
			//{
			//	if (pSH1->qwToken < pSH2->qwToken)
			//	{
			//		return false;
			//	}
			//}
		}
		return true;
	}
};

//unsigned __stdcall functionName( void* arguments )

unsigned __stdcall StartRecv(void *arguments);
unsigned __stdcall StartFetch(void *arguments);

//HANDLE g_hCompletionPort;
//HANDLE g_hReadEvent;
//OVERLAPPED g_Overlapped;
//LONG nCount = 0;

class CConnectionManager
{
public:
	CConnectionManager() {m_qwToken = 1;}
	~CConnectionManager() {;}
	bool Init();
	void UnInit();

	bool CreateConnection();
	//bool StartWorking();

	CConnectionLayer * RecvData();
	bool TraverseConnection();
	
	//void CALLBACK OnTimer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);
private:
	SOCKET  m_sock;     
	WSADATA m_wsaData; // 套接字信息数据

	timeval m_tvTimer;    // 定时变量  
	char    m_pRecvBuf[RECV_BUF_SIZE];    // 接收缓冲区 
	char    m_pSendBuf[SEND_BUF_SIZE];    // 发送缓冲区

	fd_set m_rfd;     // 读描述符集 

	fd_set m_wfd;    // 写描述符集

	sockaddr_in m_addr; // 告诉sock 应该在什么地方licence  

	int m_nRecLen; // 客户端地址长度!!!!!!  

	sockaddr_in m_cli;    // 客户端地址  
	////////////////////////////////////////////////////////////////////////////////

	UINT64 m_qwToken;

	map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp> m_conClientRecvMap;
};

#endif