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
	WSADATA m_wsaData; // �׽�����Ϣ����

	timeval m_tvTimer;    // ��ʱ����  
	char    m_pRecvBuf[RECV_BUF_SIZE];    // ���ջ����� 
	char    m_pSendBuf[SEND_BUF_SIZE];    // ���ͻ�����

	fd_set m_rfd;     // ���������� 

	fd_set m_wfd;    // д��������

	sockaddr_in m_addr; // ����sock Ӧ����ʲô�ط�licence  

	int m_nRecLen; // �ͻ��˵�ַ����!!!!!!  

	sockaddr_in m_cli;    // �ͻ��˵�ַ  
	////////////////////////////////////////////////////////////////////////////////

	UINT64 m_qwToken;

	map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp> m_conClientRecvMap;
};

#endif