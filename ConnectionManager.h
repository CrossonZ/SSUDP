#ifndef __INCLUDED_CONNECTION_MANAGER_H_
#define __INCLUDED_CONNECTION_MANAGER_H_

#define MAX_CLIENT_NUM 1000

#define SYS_SEND_BUFFER_SIZE 131702 //128*1024
#define SYS_RECV_BUFFER_SIZE 262404 //128*1024

#define RECV_BUF_SIZE 4096
#define SEND_BUF_SIZE 4096

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12) 

#include "ConnectionLayer.h"
#include "Singleton.h"


struct SBuf32
{
	char szBuf[32];
};

struct SBuf64
{
	char szBuf[64];
};

struct SBuf128
{
	char szBuf[128];
};

struct SBuf256
{
	char szBuf[256];
};

struct SBuf512
{
	char szBuf[512];
};

struct SBuf1K
{
	char szBuf[1024];
};

struct SBuf2K
{
	char szBuf[2048];
};

struct SBufMAX
{
	char szBuf[MAX_PACKET_SIZE];
};

struct SValidPacketMax
{
	char szBuf[MAX_VALID_SIZE];
};


struct SConnectionAddr
{
	SConnectionAddr():
	wPort(0)
	{
	}
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

unsigned __stdcall StartHandler(void *arguments);


class CConnectionManager
{
	DECLARE_SINGLETON_CLASS(CConnectionManager)

public:

	bool Init();
	void UnInit();

	bool CreateConnection();

	bool Connect(const char *czServerIP, const WORD wPort);//For client;

	CConnectionLayer * HandleConnection();
	bool TraverseConnection();

	char * AllocBuf(int iSize);
	SRecvBuf * AllocRB();
	SReliabilityPacket * AllocRP();
	CConnectionLayer * AllocCL();
	CReliabilityLayer * AllocRL();

	void DeleteBuf(char *pBuf, int iSize);
	void DeleteRP(SReliabilityPacket *pRP);
	void DeleteRB(SRecvBuf *pRB);
	void DeleteCL(CConnectionLayer *pCL);
	void DeleteRL(CReliabilityLayer *pRL);

	map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp> m_conClientRecvMap;
private:

	void AddBlackName(sockaddr_in sockAddr);
	SOCKET  m_sock;     
	WSADATA m_wsaData; // 套接字信息数据

	timeval m_tvTimer;    // 定时变量  
	char    m_pRecvBuf[MAX_MTU_SIZE];    // 接收缓冲区 
	char    m_pSendBuf[MAX_MTU_SIZE];    // 发送缓冲区

	fd_set m_rfd;     // 读描述符集 

	fd_set m_wfd;    // 写描述符集

	sockaddr_in m_addr; // 告诉sock 应该在什么地方licence  

	int m_nRecLen; // 客户端地址长度!!!!!!  

	sockaddr_in m_cli;    // 客户端地址  
	////////////////////////////////////////////////////////////////////////////////

	set<sockaddr_in, ConnectionAddrComp> m_conBlackNameSet;
	UINT64 m_qwGlobalToken;

	ObjectPool<SBuf32> m_char32Pool;
	ObjectPool<SBuf64> m_char64Pool;
	ObjectPool<SBuf128> m_char128Pool;
	ObjectPool<SBuf256> m_char256Pool;
	ObjectPool<SBuf512> m_char512Pool;
	ObjectPool<SBuf1K> m_char1KPool;
	ObjectPool<SBuf2K> m_char2KPool;
	//ObjectPool<SBufMAX> m_conPacketBufPool;
	ObjectPool<SValidPacketMax> m_ValidPacketBufPool;
	ObjectPool<SRecvBuf> m_RBPool;
	ObjectPool<SReliabilityPacket> m_RPPool;
	ObjectPool<CConnectionLayer> m_CLPool;
	ObjectPool<CReliabilityLayer> m_RLPool;
	//CSimpleMutex m_PoolMutex; //这个东西好像也没有必要上锁
};

#endif