#include "ConnectionManager.h"
#include "UDPThread.h"

#define BUFSIZE 1024
unsigned __stdcall StartHandler(void* arguments)
{
	CConnectionManager *pCM = (CConnectionManager *)arguments;
	while (true)  
	{
		bool bIdle = false;
		CConnectionLayer *pCL = pCM->HandleConnection();
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
	return 0;
}

IMPLEMENT_SINGLETON_INSTANCE(CConnectionManager)

bool CConnectionManager::Init()
{
	m_qwGlobalToken = 1;
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
	m_sock = socket(AF_INET, SOCK_DGRAM, 0); // �������ݱ�sock  
	if (m_sock == INVALID_SOCKET)  
	{  
		printf("socket()\n");  
		return false;  
	}  
	memset(&m_addr, 0, sizeof(m_addr));  

	m_addr.sin_family = AF_INET;   // IPЭ��
#if TYPE_SERVER == 1
	m_addr.sin_port = htons(SERVER_PORT); // �������˿� 
#else
	m_addr.sin_port = htons(CLIENT_PORT); // �ͻ��˶˿� 
#endif
	m_addr.sin_addr.s_addr = htonl(INADDR_ANY); // �ڱ���������ip�Ͽ�ʼ����  
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR)// bind socka  
	{  
		printf("bind()\n");  
		return false;  
	}  
	/*int iRecvBufSize = SYS_RECV_BUFFER_SIZE;
	setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char*)&iRecvBufSize, sizeof(int));
	int iSendBufSize = SYS_SEND_BUFFER_SIZE;
	setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&iSendBufSize, sizeof(int));*/


	unsigned long iNonBlock = 1;   
	if (ioctlsocket(m_sock, FIONBIO, &iNonBlock) == SOCKET_ERROR)   
	{   
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());   
		return false;   
	} 

	//��������Ƿ�ֹwindows�����bug����sendto�Է��������ܵ���ʱ�������Ķ��¼�һֱ����recvfrom����SOCKET_ERROR���������Ϊ10054,�˽��������Դ������
	BOOL bNewBehavior = FALSE;
	DWORD dwBytesReturned = 0;
	WSAIoctl(m_sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);

	// ���ó�ʱʱ��Ϊ6s  
	m_tvTimer.tv_sec = 6;   
	m_tvTimer.tv_usec = 0; 

	int iRet1 = CUDPThread::Create(&StartHandler, this, 0);

	return 1;
}

bool CConnectionManager::Connect(const char *czServerIP, const WORD wPort)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));   

	addr.sin_family = AF_INET;   
	addr.sin_port = htons(wPort) ;      
	addr.sin_addr.s_addr = inet_addr(czServerIP); 

	CConnectionLayer *pCL = AllocCL();
	if (pCL != NULL)
	{
		pCL->SetToken(0);
	}
	m_conClientRecvMap.insert(make_pair(addr, pCL));
	SReliabilityPacket *pRP = pCL->m_pRL->BuildToken(0);
	pCL->PushSendQueue(pRP);
	return true;
}


CConnectionLayer * CConnectionManager::HandleConnection()
{
	FD_ZERO(&m_rfd); // ��ʹ��֮ǰ����Ҫ���  

	FD_ZERO(&m_wfd);

	// ��ʼʹ��select  
	FD_SET(m_sock, &m_rfd); // ��sock����Ҫ���Ե�����������  
	FD_SET(m_sock, &m_wfd);

	int nRet; // select����ֵ  
	nRet = select(0, &m_rfd, &m_wfd, NULL, &m_tvTimer);// ����Ƿ����׽����Ƿ�ɶ�  
	if (nRet == SOCKET_ERROR)     
	{  
		printf("select()\n");  
		return NULL;  
	}  
	else if (nRet == 0) // ��ʱ  
	{  
		printf("timeout\n");  
		return NULL;
	}  
	else    // ��⵽���׽��ֿɶ�  
	{  
		if (FD_ISSET(m_sock, &m_rfd))  // sock�ɶ�  
		{  
			m_nRecLen = sizeof(m_cli);
			memset(m_pRecvBuf, 0, sizeof(m_pRecvBuf)); // ��ս��ջ�����
			int iRecvCount = recvfrom(m_sock, m_pRecvBuf, RECV_BUF_SIZE, 0, (sockaddr*)&m_cli, &m_nRecLen);  
			if (iRecvCount == INVALID_SOCKET)  
			{  
				printf("recvfrom(): %d\n", WSAGetLastError());  
				return NULL; 
			}
			if (iRecvCount > 0)
			{
				/*SConnectionAddr *pSA = new SConnectionAddr;
				if (pSA == NULL)
				{
					return NULL;
				}
				pSA->sin_addr = m_cli.sin_addr;
				pSA->wPort = m_cli.sin_port;*/

				if (m_conBlackNameSet.count(m_cli) > 0) //�鿴������
				{
					return NULL;
				}
				map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp>::iterator itMap = m_conClientRecvMap.find(m_cli);
				if (itMap != m_conClientRecvMap.end())
				{
					if (itMap->second->PushRB(m_pRecvBuf, iRecvCount))
					{
						return itMap->second;
					}
				}
				else
				{
					CConnectionLayer* pCL = AllocCL();
					if (pCL == NULL)
					{
						return NULL;
					}
					pCL->SetToken(m_qwGlobalToken++);
					pCL->PushRB(m_pRecvBuf, iRecvCount);
					m_conClientRecvMap.insert(make_pair(m_cli, pCL));
					return pCL;
				}
			}
		}
		if (FD_ISSET(m_sock, &m_wfd))  // sock��д  
		{  
			map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp>::iterator itMap;// = m_conClientRecvMap.find(pSA);
			SReliabilityPacket * pRP;
	/*		sockaddr_in sockAddr;*/
			//��Ӧ��ֱ��fetch�������ȼ�ⷢ��һ��İ�
			for (itMap = m_conClientRecvMap.begin(); itMap != m_conClientRecvMap.end(); ++itMap)
			{
				pRP = itMap->second->PopSendQueuePacket();
				if (pRP != NULL)
				{
					memset(m_pSendBuf, 0, sizeof(m_pSendBuf)); // ��ս��ջ�����
					memcpy(m_pSendBuf, &pRP->sRPH, RPH_SIZE);
					memcpy(m_pSendBuf+RPH_SIZE, pRP->pBuf, pRP->sRPH.iSingleSize - RPH_SIZE);
					//memset(&sockAddr, 0, sizeof(sockaddr_in));
					int iSentCount = 0;
					do 
					{
						iSentCount = sendto(m_sock, (char *)&m_pSendBuf, pRP->sRPH.iSingleSize/*-itMap->second->m_iSentOffset*/, 0, (sockaddr*)&itMap->first, sizeof(itMap->first)); //���iTotalSizeҪ�ĵ� 
						if (iSentCount == INVALID_SOCKET)  
						{  
							printf("recvfrom()\n");  
							return NULL; 
						}

						if (iSentCount < pRP->sRPH.iSingleSize)
						{
						//	//����ǣ���¼offset;
						//	itMap->second->m_iSentOffset += nRecEcho;
							break;
						}
					} while (iSentCount == 0);
					//itMap->second->DeleteRP(pRP);
					//itMap->second->m_pCurrSendPacket = NULL;
				}
			}
		}
	}
	return NULL;
}

void CConnectionManager::AddBlackName(sockaddr_in sockAddr)
{
	m_conBlackNameSet.insert(sockAddr);
}

bool CConnectionManager::TraverseConnection()
{
	bool bBusy = false;
	SRecvBuf *pRB;
	DWORD dwTickcount = GetTickCount();
	map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp>::iterator itMap = m_conClientRecvMap.begin();
	for (;itMap != m_conClientRecvMap.end();)
	{
		pRB = itMap->second->PopRB();
		if (pRB != NULL)
		{
			bool bValid = itMap->second->m_pRL->Recieve(pRB->pBuf, pRB->iRecvLen);
			//printf("RB recv cache queue size:%d\n", itMap->second->m_pRL->GetRBRecvBufSize());
			//printf("PB ack message queue size:%d\n", itMap->second->m_pRL->GetRBRecvQueueSize());
			bBusy = true;
			CConnectionManager::GetInstance()->DeleteRB(pRB);
			if (!bValid)
			{
				AddBlackName(itMap->first);
			}
			++itMap;
		}
		else
		{
			//�϶�Ҫ�ȴ�����ʣ�µİ���������߶������������������ӳ�ʱ��ɾ������
			if (dwTickcount > itMap->second->GetLastRecvTime() + 20000)
			{
				DeleteCL(itMap->second);
				itMap = m_conClientRecvMap.erase(itMap);
			}
			else
			{
				++itMap;
			}
		}
	}
	return bBusy;
}

char * CConnectionManager::AllocBuf(int iSize)
{
	char *pBuf = NULL;
	//m_PoolMutex.Lock();
	if (iSize < 32)
	{
		pBuf = (char *)m_char32Pool.New();
	}
	else if (iSize < 64)
	{
		pBuf = (char *)m_char64Pool.GetNextWithoutInitializing();
	}
	else if (iSize < 128)
	{
		pBuf = (char *)m_char128Pool.New();
	}
	else if (iSize < 256)
	{
		pBuf = (char *)m_char256Pool.New();
	}
	else if (iSize < 512)
	{
		pBuf = (char *)m_char512Pool.New();
	}
	else if (iSize < 1024)
	{
		pBuf = (char *)m_char1KPool.New();
	}
	else if (iSize < 2048)
	{
		pBuf = (char *)m_char2KPool.New();
	}
	else if (iSize < MAX_VALID_SIZE)
	{
		pBuf = (char *)m_ValidPacketBufPool.New();
	}
	//m_PoolMutex.Unlock();
	return pBuf;
}

void CConnectionManager::DeleteBuf(char *pBuf, int iSize)
{
	if (iSize < 32)
	{
		m_char32Pool.DeleteWithoutDestroying((SBuf32 *)pBuf);
	}
	else if (iSize < 64)
	{
		m_char64Pool.DeleteWithoutDestroying((SBuf64 *)pBuf);
	}
	else if (iSize < 128)
	{
		m_char128Pool.DeleteWithoutDestroying((SBuf128 *)pBuf);
	}
	else if (iSize < 256)
	{
		m_char256Pool.DeleteWithoutDestroying((SBuf256 *)pBuf);
	}
	else if (iSize < 512)
	{
		m_char512Pool.DeleteWithoutDestroying((SBuf512 *)pBuf);
	}
	else if (iSize < 1024)
	{
		m_char1KPool.DeleteWithoutDestroying((SBuf1K *)pBuf);
	}
	else if (iSize < 2048)
	{
		m_char2KPool.DeleteWithoutDestroying((SBuf2K *)pBuf);
	}
	else if (iSize < MAX_VALID_SIZE)
	{
		m_ValidPacketBufPool.DeleteWithoutDestroying((SValidPacketMax *)pBuf);
	}
}

SRecvBuf * CConnectionManager::AllocRB()
{
	////m_PoolMutex.Lock();
	return m_RBPool.New();
	////m_PoolMutex.Unlock();
}

SReliabilityPacket * CConnectionManager::AllocRP()
{
	SReliabilityPacket *pRP = NULL;
	//m_PoolMutex.Lock();
	pRP = m_RPPool.New();
	//m_PoolMutex.Unlock();
	return pRP;
}

CConnectionLayer * CConnectionManager::AllocCL()
{
	CConnectionLayer * pCL = NULL;

	pCL = m_CLPool.New();

	pCL->Reset();
	return pCL;
}

CReliabilityLayer * CConnectionManager::AllocRL()
{
	CReliabilityLayer *pRL = NULL;
	pRL = m_RLPool.New();
	pRL->Reset();
	return pRL;
}

void CConnectionManager::DeleteRP(SReliabilityPacket *pRP)
{
	//m_PoolMutex.Lock();
	DeleteBuf(pRP->pBuf, pRP->sRPH.iSingleSize-RPH_SIZE);
	m_RPPool.DeleteWithoutDestroying(pRP);
	//m_PoolMutex.Unlock();
}

void CConnectionManager::DeleteRB(SRecvBuf *pRB)
{
	//m_PoolMutex.Lock();
	DeleteBuf(pRB->pBuf, pRB->iTotalLen);
	m_RBPool.DeleteWithoutDestroying(pRB);
	//m_PoolMutex.Unlock();
}

void CConnectionManager::DeleteCL(CConnectionLayer *pCL)
{
	DeleteRL(pCL->m_pRL);
	m_CLPool.DeleteWithoutDestroying(pCL);
}

void CConnectionManager::DeleteRL(CReliabilityLayer *pRL)
{
	pRL->ClearRecvBufMap();
	pRL->ClearCombinedPacket();
	pRL->ClearSendBufMap();
	m_RLPool.DeleteWithoutDestroying(pRL);
}