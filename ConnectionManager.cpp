#include "ConnectionManager.h"
#include "UDPThread.h"

#define BUFSIZE 1024
unsigned __stdcall StartRecv(void* arguments)
{
	CConnectionManager *pCM = (CConnectionManager *)arguments;
	while (true)  
	{
		bool bIdle = false;
		CConnectionLayer *pCL = pCM->RecvData();
		if (pCL == NULL)
		{
			bIdle = true;
		}
		if (!pCM->TraverseConnection())
		{
			bIdle = true;
		}
		if (bIdle)
		{
			Sleep(10);
		}
	}


	/*DWORD nSocket; 
	BOOL b; 
	OVERLAPPED ovl; 
	LPOVERLAPPED lpo=&ovl; 
	DWORD nBytesRead=0; 
	DWORD nBytesToBeRead; 
	UCHAR ReadBuffer[BUFSIZE]; 
	LPVOID lpMsgBuf; 

	memset(&ReadBuffer,0,BUFSIZE); 
	for (;;) 
	{ 
		b = GetQueuedCompletionStatus(g_hCompletionPort,
			&nBytesToBeRead,&nSocket,&lpo,INFINITE); 
		if (b || lpo) 
		{ 
			if (b) 
			{ 
				// 
				// Determine how long a response was desired by the client. 
				// 

				OVERLAPPED ol; 
				ol.hEvent = g_hReadEvent; 
				ol.Offset = 0; 
				ol.OffsetHigh = 0; 

				b = ReadFile ((HANDLE)nSocket,&ReadBuffer,
					nBytesToBeRead,&nBytesRead,&ol); 
				if (!b )  
				{ 
					DWORD dwErrCode = GetLastError(); 
					if( dwErrCode != ERROR_IO_PENDING ) 
					{ 
						// something has gone wrong here... 
						printf("Something has gone wrong:Error code - %d\n",dwErrCode ); 

					  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
						NULL, dwErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),// Default language
						(LPTSTR) &lpMsgBuf, 0, NULL); 

						OutputDebugString((LPCTSTR)lpMsgBuf); 
						//Free the buffer. 

						LocalFree(lpMsgBuf ); 
					} 
					else if( dwErrCode == ERROR_IO_PENDING ) 
					{
						// I had to do this for my UDP sample 
						//Never did for my TCP servers 
						WaitForSingleObject(ol.hEvent,INFINITE); 

						InterlockedIncrement(&nCount); 
						SYSTEMTIME *lpstSysTime; 

						lpstSysTime = (SYSTEMTIME *)(ReadBuffer); 
						printf("[%d]UTC Time %02d:%02d:%02d:%03d on %02d-%02d-%d \n",nCount, 
							lpstSysTime->wHour, lpstSysTime->wMinute, 
							lpstSysTime->wSecond, lpstSysTime->wMilliseconds, 
							lpstSysTime->wMonth, lpstSysTime->wDay, lpstSysTime->wYear); 
						memset(&ReadBuffer,0,BUFSIZE); 
						//just making sure that i am not showing stale data 

					} 
				} 
				else 
				{ 
					InterlockedIncrement(&nCount); 
					SYSTEMTIME *lpstSysTime; 

					lpstSysTime = (SYSTEMTIME *)(ReadBuffer); 
					printf("[%d]UTC Time %02d:%02d:%02d:%03d on %02d-%02d-%d \n",nCount, 
						lpstSysTime->wHour, lpstSysTime->wMinute, 
						lpstSysTime->wSecond, lpstSysTime->wMilliseconds, 
						lpstSysTime->wMonth, lpstSysTime->wDay, lpstSysTime->wYear); 
					memset(&ReadBuffer,0,BUFSIZE); 
					//just making sure that i am not showing stale data 

				} 
				continue; 
			} 
			else 
			{ 
				fprintf (stdout, "WorkThread Wait Failed\n"); 
				//exit (1); 
			} 
		} 
		return 1; 
	}*/
	return 0;
}

unsigned __stdcall StartFetch(void *arguments)
{
	CConnectionManager *pCM = (CConnectionManager *)arguments;
	while (1)
	{
		if (!pCM->TraverseConnection())
		{
			Sleep(10);
		}
	}
	return 0;
}

bool CConnectionManager::Init()
{
	if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0)  
	{  
		printf("failed to load winsock!\n");  
		return false;  
	}  
	
	return true;
}

void CConnectionManager::UnInit()
{
	WSACleanup(); 
}

bool CConnectionManager::CreateConnection()
{
	m_sock = socket(AF_INET, SOCK_DGRAM, 0); // 创建数据报sock  
	if (m_sock == INVALID_SOCKET)  
	{  
		printf("socket()\n");  
		return false;  
	}  
	memset(&m_addr, 0, sizeof(m_addr));  

/*	struct ip_mreq stMreq; // Multicast interface structure

	int iFlag = 1;
	int nRet = setsockopt(m_sock,SOL_SOCKET, SO_REUSEADDR, (char *)&iFlag, sizeof(iFlag));  
	if (nRet == SOCKET_ERROR)  
	{ 
		printf ("setsockopt() SO_REUSEADDR failed, Err: %d\n",WSAGetLastError()); 
	} */


	m_addr.sin_family = AF_INET;   // IP协议  
	m_addr.sin_port = htons(SERVER_PORT); // 端口  
	m_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 在本机的所有ip上开始监听  
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR)// bind socka  
	{  
		printf("bind()\n");  
		return false;  
	}  

	// Join the multicast group so we can receive from it  
	/*stMreq.imr_multiaddr.s_addr = inet_addr(achMCAddr); 
	stMreq.imr_interface.s_addr = INADDR_ANY; 
	nRet = setsockopt(g_hSocket,IPPROTO_IP, 
		IP_ADD_MEMBERSHIP,(char *)&stMreq,sizeof(stMreq)); 

	if (nRet == SOCKET_ERROR)  
	{ 
		printf("setsockopt() IP_ADD_MEMBERSHIP address %s failed, 
Err: %d\n",achMCAddr,
	 WSAGetLastError()); 
	}*/

	unsigned long iNonBlock = 1;   
	if (ioctlsocket(m_sock, FIONBIO, &iNonBlock) == SOCKET_ERROR)   
	{   
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());   
		return false;   
	} 

	//下面这段是防止windows本身的bug，当sendto对方主机不能到达时，本机的读事件一直产生recvfrom返回SOCKET_ERROR，错误代码为10054,此解决方案，源于网络
	BOOL bNewBehavior = FALSE;
	DWORD dwBytesReturned = 0;
	WSAIoctl(m_sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);
	//
	//note the 10 says how many concurrent cpu bound threads to allow thru 
	//this should be tunable based on the requests. CPU bound requests will 
	// really really honor this. 
	// 

	/*g_hCompletionPort = CreateIoCompletionPort (INVALID_HANDLE_VALUE,
		NULL,0,3); 
	if (!g_hCompletionPort) 
	{ 
		fprintf (stdout, "g_hCompletionPort Create Failed\n"); 
		return FALSE; 
	} 
	//Associate this socket to this I/O completion port 
	CreateIoCompletionPort((HANDLE)m_sock,g_hCompletionPort,(DWORD)m_sock,3);  

	//
	// Start off an asynchronous read on the socket.  
	//  
	g_Overlapped.hEvent = g_hReadEvent;  
	g_Overlapped.Internal = 0;  
	g_Overlapped.InternalHigh = 0;  
	g_Overlapped.Offset = 0;  
	g_Overlapped.OffsetHigh = 0;  

	DWORD dwBytes;
	BOOL b = ReadFile ((HANDLE)m_sock,&m_pRecvBuf,
		sizeof(m_pRecvBuf),&dwBytes,&g_Overlapped);  

	if (!b && GetLastError () != ERROR_IO_PENDING)  
	{  
		fprintf (stdout, "ReadFile Failed\n");  
		return FALSE;  
	} */


	// 设置超时时间为6s  
	m_tvTimer.tv_sec = 6;   
	m_tvTimer.tv_usec = 0; 

	int iRet1 = CUDPThread::Create(&StartRecv, this, 0);
	//int iRet2 = CUDPThread::Create(&StartFetch, this, 0);
	return iRet1/* & iRet2*/;
}


CConnectionLayer * CConnectionManager::RecvData()
{
	FD_ZERO(&m_rfd); // 在使用之前总是要清空  

	FD_ZERO(&m_wfd);

	// 开始使用select  
	FD_SET(m_sock, &m_rfd); // 把sock放入要测试的描述符集中  
	FD_SET(m_sock, &m_wfd);

	int nRet; // select返回值  
	nRet = select(0, &m_rfd, &m_wfd, NULL, &m_tvTimer);// 检测是否有套接字是否可读  
	if (nRet == SOCKET_ERROR)     
	{  
		printf("select()\n");  
		return NULL;  
	}  
	else if (nRet == 0) // 超时  
	{  
		printf("timeout\n");  
		return NULL;
	}  
	else    // 检测到有套接字可读  
	{  
		if (FD_ISSET(m_sock, &m_rfd))  // sock可读  
		{  
			m_nRecLen = sizeof(m_cli);
			memset(m_pRecvBuf, 0, sizeof(m_pRecvBuf)); // 清空接收缓冲区
			int nRecEcho = recvfrom(m_sock, m_pRecvBuf, RECV_BUF_SIZE, 0, (sockaddr*)&m_cli, &m_nRecLen);  
			if (nRecEcho == INVALID_SOCKET)  
			{  
				printf("recvfrom(): %d\n", WSAGetLastError());  
				return NULL; 
			}
			if (nRecEcho > 0)
			{
				/*SConnectionAddr *pSA = new SConnectionAddr;
				if (pSA == NULL)
				{
					return NULL;
				}
				pSA->sin_addr = m_cli.sin_addr;
				pSA->wPort = m_cli.sin_port;*/
				map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp>::iterator itMap = m_conClientRecvMap.find(m_cli);
				if (itMap != m_conClientRecvMap.end())
				{
					if (itMap->second->PushRecvQueue(m_pRecvBuf, nRecEcho))
					{
						return itMap->second;
					}
				}
				else
				{
					CConnectionLayer* pCL = new CConnectionLayer;
					if (pCL == NULL)
					{
						return false;
					}
					pCL->PushRecvQueue(m_pRecvBuf, nRecEcho);
					m_conClientRecvMap.insert(make_pair(m_cli, pCL));
					return pCL;
				}
			}
		}
		if (FD_ISSET(m_sock, &m_wfd))  // sock可写  
		{  
			map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp>::iterator itMap;// = m_conClientRecvMap.find(pSA);
			SReliabilityPacket * pRP;
	/*		sockaddr_in sockAddr;*/
			//不应该直接fetch，而是先检测发了一半的包
			for (itMap = m_conClientRecvMap.begin(); itMap != m_conClientRecvMap.end(); ++itMap)
			{
				pRP = itMap->second->GetSendQueuePacket();
				if (pRP != NULL)
				{
					memset(m_pSendBuf, 0, sizeof(m_pSendBuf)); // 清空接收缓冲区
					memcpy(m_pSendBuf, &pRP->sRPH, RPH_SIZE);
					memcpy(m_pSendBuf+RPH_SIZE, pRP->pBuf, pRP->sRPH.iSingleSize - RPH_SIZE);
					//memset(&sockAddr, 0, sizeof(sockaddr_in));
					int nRecEcho = sendto(m_sock, (char *)&m_pSendBuf, pRP->sRPH.iSingleSize, 0, (sockaddr*)&itMap->first, sizeof(itMap->first)); //这个iTotalSize要改的 
					if (nRecEcho == INVALID_SOCKET)  
					{  
						printf("recvfrom()\n");  
						return NULL; 
					}
					
					if (nRecEcho < pRP->sRPH.iSingleSize)
					{
						//做标记，记录offset;
						break;
					}
				}
			}
		}
	}
	return NULL;
}

bool CConnectionManager::TraverseConnection()
{
	bool bBusy = false;
	SRecvBuf *pRB;
	map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp>::iterator itMap = m_conClientRecvMap.begin();
	for (;itMap != m_conClientRecvMap.end();++itMap)
	{
		pRB = itMap->second->PopRecvQueue();
		if (pRB != NULL)
		{
			itMap->second->m_pRL->Recieve(pRB->pBuf, pRB->iRecvLen);
			printf("RB recv cache queue size:%d\n", itMap->second->m_pRL->GetRBRecieveSize());
			printf("PB ack message queue size:%d\n", itMap->second->m_pRL->GetRBRecvSize());
			bBusy = true;
		}
	}
	return bBusy;
}

//void CALLBACK CConnectionManager::OnTimer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
//{
//	map<SConnectionAddr *, CConnectionLayer *, ConnectionAddrComp>::iterator itMap = m_conClientRecvMap.begin();
//	for (;itMap != m_conClientRecvMap.end();++itMap)
//	{
//		itMap->second->m_pRL->
//	}
//}