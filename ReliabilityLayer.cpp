#include "ReliabilityLayer.h"
#include "ConnectionLayer.h"
#include "ConnectionManager.h"
#include <time.h>
#include <stdio.h>

void CReliabilityLayer::Reset()
{
	m_iGlobalSeq = 0;
	m_iNextRecvSeq = 1;
	m_wNextRecvSplitSeq = 1;

	m_iLastNAKSeq = 0;
	m_wLastNAKSplitSeq = 0;
	ClearRecvBufMap();
	ClearCombinedPacket();
	ClearSendBufMap();
}

bool CReliabilityLayer::Send(const char *pBuf, const int iBufLen)
{
	if (pBuf==NULL || iBufLen > MAX_VALID_SIZE)
	{
		return false;
	}

	WORD wSplitNum = iBufLen / MAX_PACKET_SIZE;
	WORD wSplitSeq = 0;
	++m_iGlobalSeq;

	SSplitPacketHeader sSPH;
	SReliabilityPacket *pRP;
	map<WORD, pair<SReliabilityPacket*, DWORD> > conSendCtrlMap;
	DWORD dwTime = (DWORD)time(NULL);//GetTickCount();
	if (wSplitNum > 0)  //若需要分包发送
	{
		wSplitNum = iBufLen / MAX_SPLIT_SIZE + 1;
		WORD wSplitCount = wSplitNum;
		while (wSplitNum > 1)
		{
			pRP = CConnectionManager::GetInstance()->AllocRP();
			if (pRP == NULL)
			{
				return false;
			}
			pRP->sRPH.iPacketSeq = m_iGlobalSeq;
			pRP->sRPH.iTotalSize = iBufLen;
			pRP->sRPH.iSingleSize = MAX_MTU_SIZE;
			pRP->sRPH.wSplitNum = wSplitCount;
			pRP->sRPH.qwToken = m_pCL->GetToken();
			pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(MAX_PACKET_SIZE);//new char [MAX_PACKET_SIZE];
			
			//sSPH.iSplitPacketSize = MAX_SPLIT_SIZE;
			sSPH.wSplitSeq = ++wSplitSeq;
			memcpy(pRP->pBuf,  &sSPH, SPH_SIZE);
			memcpy(pRP->pBuf+SPH_SIZE, pBuf+(wSplitSeq-1)*MAX_SPLIT_SIZE, MAX_SPLIT_SIZE);

			conSendCtrlMap.insert(make_pair(wSplitSeq, make_pair(pRP, dwTime+SEND_TIME_OUT)));
#ifdef FOR_TEST
			RandomDropPacket(pRP);
#else
			m_pCL->PushSendQueue(pRP);
#endif
			--wSplitNum;
		}
		if (iBufLen % MAX_SPLIT_SIZE)
		{
			pRP = CConnectionManager::GetInstance()->AllocRP();
			if (pRP == NULL)
			{
				return false;
			}
			pRP->sRPH.iPacketSeq = m_iGlobalSeq;
			pRP->sRPH.iTotalSize = iBufLen;
			pRP->sRPH.iSingleSize = iBufLen % MAX_SPLIT_SIZE + RPH_SPH_SIZE;
			pRP->sRPH.wSplitNum = wSplitCount;
			pRP->sRPH.qwToken = m_pCL->GetToken();
			pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(iBufLen % MAX_SPLIT_SIZE + SPH_SIZE);//new char [iBufLen % MAX_SPLIT_SIZE + SPH_SIZE];

			//sSPH.iSplitPacketSize = iBufLen % MAX_SPLIT_SIZE;
			sSPH.wSplitSeq = ++wSplitSeq;
			memcpy(pRP->pBuf,  &sSPH, SPH_SIZE);
			memcpy(pRP->pBuf+SPH_SIZE, pBuf+(wSplitSeq-1)*MAX_SPLIT_SIZE, iBufLen % MAX_SPLIT_SIZE);

			conSendCtrlMap.insert(make_pair(wSplitSeq, make_pair(pRP, dwTime+SEND_TIME_OUT)));
#ifdef FOR_TEST
			RandomDropPacket(pRP);
#else
			m_pCL->PushSendQueue(pRP);
#endif
		}
	} 
	else
	{
		pRP = CConnectionManager::GetInstance()->AllocRP();
		if (pRP == NULL)
		{
			return false;
		}
		pRP->sRPH.iPacketSeq = m_iGlobalSeq;
		pRP->sRPH.iTotalSize = iBufLen;
		pRP->sRPH.iSingleSize = iBufLen + RPH_SIZE;
		pRP->sRPH.wSplitNum = 1;
		pRP->sRPH.qwToken = m_pCL->GetToken();
		pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(iBufLen);//new char [iBufLen];

		memcpy(pRP->pBuf, pBuf, iBufLen);

		conSendCtrlMap.insert(make_pair(1, make_pair(pRP, dwTime+SEND_TIME_OUT)));
#ifdef FOR_TEST
		RandomDropPacket(pRP);
#else
		m_pCL->PushSendQueue(pRP);
#endif
		--wSplitNum;
	}
	m_PacketBufMapMutex.Lock();
	m_conPacketBufMap.insert(make_pair(m_iGlobalSeq, conSendCtrlMap));
	m_PacketBufMapMutex.Unlock();
	return true;
}

SReliabilityPacket * CReliabilityLayer::BuildNAK(const int iNAKSeq, const WORD wSplitSeq)
{
	SReliabilityPacket * pRP = CConnectionManager::GetInstance()->AllocRP();
	pRP->sRPH.iPacketSeq = PACKET_SEQ_NAK;
	pRP->sRPH.iTotalSize = 4;
	pRP->sRPH.iSingleSize = NAK_PACKET_SIZE;
	pRP->sRPH.wSplitNum = wSplitSeq;
	pRP->sRPH.qwToken = m_pCL->GetToken();
	pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(4);//new char[4];
	memcpy(pRP->pBuf, (char *)&iNAKSeq, 4);
	printf("A packet lost seq:%d, sub:%d\n", iNAKSeq, wSplitSeq);
	return pRP;
}

SReliabilityPacket * CReliabilityLayer::BuildACK(const int iACKSeq, const WORD wSplitSeq)
{
	SReliabilityPacket * pRP = CConnectionManager::GetInstance()->AllocRP();
	pRP->sRPH.iPacketSeq = PACKET_SEQ_ACK;
	pRP->sRPH.iTotalSize = 4;
	pRP->sRPH.iSingleSize = ACK_PACKET_SIZE;
	pRP->sRPH.wSplitNum = wSplitSeq;
	pRP->sRPH.qwToken = m_pCL->GetToken();
	pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(4);//new char[4];
	memcpy(pRP->pBuf, (char *)&iACKSeq, 4);
	return pRP;
}

SReliabilityPacket * CReliabilityLayer::BuildToken(const UINT64 qwToken)
{
	SReliabilityPacket * pRP = CConnectionManager::GetInstance()->AllocRP();
	pRP->sRPH.iPacketSeq = 0;
	pRP->sRPH.iTotalSize = 8;
	pRP->sRPH.iSingleSize = TOKEN_PACKET_SIZE;
	pRP->sRPH.wSplitNum = 1;
	pRP->sRPH.qwToken = qwToken;
	pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(8);
	memcpy(pRP->pBuf, (char *)&qwToken, 8);
	return pRP;
}

bool CReliabilityLayer::ClearSendBufMap()
{
	map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itBaseMap;
	map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itSubMap;
	m_PacketBufMapMutex.Lock();
	for (itBaseMap=m_conPacketBufMap.begin();itBaseMap!=m_conPacketBufMap.end();)
	{
		for (itSubMap = itBaseMap->second.begin();itSubMap!=itBaseMap->second.end();)
		{
			CConnectionManager::GetInstance()->DeleteRP(itSubMap->second.first);
			itSubMap = itBaseMap->second.erase(itSubMap);
		}
		itBaseMap = m_conPacketBufMap.erase(itBaseMap);
	}
	m_PacketBufMapMutex.Unlock();
	return true;
}

bool CReliabilityLayer::Recieve(const char *pBuf, const int iBufLen)
{
	if (iBufLen < SPH_SIZE)
	{
		return false;
	}

	SReliabilityPacketHeader *pRPH = (SReliabilityPacketHeader *)pBuf;
	int iRet = CheckConnection(pRPH->qwToken);
	switch (iRet)
	{
	case -1:
		{
			return false;
		}
	case 0:
		{
			return true;
		}
	case 1:
		{
			
		}break;
	default:
		{
			return false;
		}
	}

	//这里需要做一个包合法性检测机制

	if (pRPH->iPacketSeq == PACKET_SEQ_ACK)
	{
		//进行删除操作
		int iACKSeq = *(DWORD *)(pBuf+RPH_SIZE);
		map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itMap;
		map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itSubMap;
		m_PacketBufMapMutex.Lock();
		//itMap = m_conPacketBufMap.find(iResendSeq);
		itMap=m_conPacketBufMap.begin();
		while (itMap!=m_conPacketBufMap.end() && itMap->first<=iACKSeq)
		{
			itSubMap = itMap->second.begin();
			for (;itSubMap != itMap->second.end();++itSubMap)
			{
				CConnectionManager::GetInstance()->DeleteRP(itSubMap->second.first);
			}
			itMap = m_conPacketBufMap.erase(itMap);
		}
		m_PacketBufMapMutex.Unlock();
		printf("_________________________________ACK________________________________________\n");

		return true;
	}
	else if (pRPH->iPacketSeq == PACKET_SEQ_NAK)
	{
		//进行丢包重传操作
		WORD wSplitSeq = pRPH->wSplitNum;
		int iResendSeq = *(DWORD *)(pBuf+RPH_SIZE);

		map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itMap;
		map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itSubMap;
		m_PacketBufMapMutex.Lock();
		itMap = m_conPacketBufMap.find(iResendSeq);
		if (itMap != m_conPacketBufMap.end())
		{
			itSubMap = itMap->second.find(wSplitSeq);
			if (itSubMap != itMap->second.end())
			{
				m_pCL->PushResendQueue(itSubMap->second.first);
			}
		}
		m_PacketBufMapMutex.Unlock();
		printf("Resend packet seq:%d, sub:%d\n", iResendSeq, wSplitSeq);
		return true;
	}

	if (pRPH->iPacketSeq < m_iNextRecvSeq || pRPH->wSplitNum < m_wNextRecvSplitSeq)
	{
		//已经确认的数据包，丢弃
		return true;
	}

	if (pRPH->wSplitNum == 1) //无分包
	{
		if (m_conRecvPacketBufMap.find(pRPH->iPacketSeq) != m_conRecvPacketBufMap.end())
		{
			return true; //重复包,丢掉
		}
		SReliabilityPacket *pRP = CConnectionManager::GetInstance()->AllocRP();
		pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(iBufLen-RPH_SIZE);//new char[iBufLen-RPH_SIZE];
		memcpy(&pRP->sRPH, pRPH, RPH_SIZE); 
		memcpy(pRP->pBuf, pBuf+RPH_SIZE, iBufLen-RPH_SIZE);
		//做丢包重传检测
		if (CheckPacketLost(pRPH->iPacketSeq, 1))
		{
			map<WORD, pair<SReliabilityPacket*, DWORD> > conMap;
			conMap.insert(make_pair(1, make_pair(pRP, GetTickCount())));
			m_conRecvPacketBufMap.insert(make_pair(pRP->sRPH.iPacketSeq, conMap));	
		}
		else
		{
			PushRecvQueue(pRP);
			CheckSendACK(pRP->sRPH.iPacketSeq);
			map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itBaseMap = m_conRecvPacketBufMap.find(m_iNextRecvSeq);
			while(itBaseMap != m_conRecvPacketBufMap.end())
			{
				if (!RecombinationPacket(itBaseMap->second))
				{
					break;
				}
				else
				{
					//指针需回收，在有池之后做
					itBaseMap = m_conRecvPacketBufMap.erase(itBaseMap);
				}
			}
		}
	}
	else 
	{
		//分包处理
		SSplitPacketHeader *pSPH = (SSplitPacketHeader *)(pBuf+RPH_SIZE);
		bool bPacketLost = CheckPacketLost(pRPH->iPacketSeq, pSPH->wSplitSeq);
		map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itBaseMap = m_conRecvPacketBufMap.find(pRPH->iPacketSeq);
		if (itBaseMap == m_conRecvPacketBufMap.end()) //分包中的第一个包
		{
			SReliabilityPacket *pRP = CConnectionManager::GetInstance()->AllocRP();
			pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(iBufLen-RPH_SIZE);//new char[iBufLen-RPH_SIZE];

			memcpy(&pRP->sRPH, pRPH, RPH_SIZE);
			memcpy(pRP->pBuf, pBuf+RPH_SIZE, iBufLen-RPH_SIZE);
			map<WORD, pair<SReliabilityPacket*, DWORD> > conMap;
			conMap.insert(make_pair(pSPH->wSplitSeq, make_pair(pRP, GetTickCount())));
			m_conRecvPacketBufMap.insert(make_pair(pRP->sRPH.iPacketSeq, conMap));
			//此时不进行包合并处理，原因是不可能成完整包
		}
		else
		{
			map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itSubMap = itBaseMap->second.find(pSPH->wSplitSeq);
			if (itSubMap != itBaseMap->second.end())
			{
				//if (bPacketLost)
				//{
				//	map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itTmpMap;
				//	do
				//	{
				//		itTmpMap = itBaseMap->second.find(++m_wNextRecvSplitSeq);
				//	}while(itTmpMap == itBaseMap->second.end());
				//}
				return true; //重复包,丢掉
			}
			else
			{
				SReliabilityPacket *pRP = CConnectionManager::GetInstance()->AllocRP();
				pRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(iBufLen-RPH_SIZE);//new char[iBufLen-RPH_SIZE];
				memcpy(&pRP->sRPH, pRPH, RPH_SIZE);
				memcpy(pRP->pBuf, pBuf+RPH_SIZE, iBufLen-RPH_SIZE);
				itBaseMap->second.insert(make_pair(pSPH->wSplitSeq, make_pair(pRP,GetTickCount())));
				//这里做丢包检测

				if (!bPacketLost)
				{
					while(itBaseMap != m_conRecvPacketBufMap.end())
					{
						if (!RecombinationPacket(itBaseMap->second))
						{
							break;
						}
						else
						{
							//指针需回收，在有池之后做
							for (itSubMap = itBaseMap->second.begin();itSubMap!=itBaseMap->second.end();++itSubMap)
							{
								CConnectionManager::GetInstance()->DeleteRP(itSubMap->second.first);
							}
							itBaseMap = m_conRecvPacketBufMap.erase(itBaseMap);
						}
					}
				}
			}

		}
	}
	//SReliabilityPacket *pBackPacket = GetRecvQueueBack();
	//if (pBackPacket != NULL)
	//{
	//	m_iNextRecvSeq = pBackPacket->sRPH.iPacketSeq + 1;
	//	m_wNextRecvSplitSeq = 1;
	//}
	map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itTmpMap = m_conRecvPacketBufMap.find(m_iNextRecvSeq);
	if (itTmpMap != m_conRecvPacketBufMap.end())
	{
		map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itTmpSubMap = itTmpMap->second.begin();
		WORD wTmpSeq = 1;
		while (itTmpSubMap != itTmpMap->second.end())
		{
			if (wTmpSeq == itTmpSubMap->first)
			{
				++wTmpSeq;
				++itTmpSubMap;
			}
			else
			{
				break;
			}
		}
		m_wNextRecvSplitSeq = wTmpSeq;
	}

	return true;
}


int CReliabilityLayer::GetRBRecvQueueSize()
{
	int iSize = 0;
	m_RecvQueueMutex.Lock();
	iSize = m_conRecvQueue.size();
	m_RecvQueueMutex.Unlock();
	return iSize;
}

bool CReliabilityLayer::CheckPacketLost(const int iRecvSeq, const WORD wSplitSeq)
{
	if (iRecvSeq == m_iNextRecvSeq && wSplitSeq == m_wNextRecvSplitSeq)
	{
		return false;
	}
	if (m_iNextRecvSeq == m_iLastNAKSeq && m_wNextRecvSplitSeq == m_wLastNAKSplitSeq && m_wDumplicateNAKNum < DUMPLICATE_NAK)
	{
		++m_wDumplicateNAKNum;
		return true;
	}
	SReliabilityPacket *pRP = BuildNAK(m_iNextRecvSeq, m_wNextRecvSplitSeq);
	m_pCL->PushResendQueue(pRP);
	m_wDumplicateNAKNum = 0;
	return true;
}

bool CReliabilityLayer::RecombinationPacket(map<WORD, pair<SReliabilityPacket*, DWORD/*recvTime*/> > &conSplitPackets)
{

	map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itSubMap = conSplitPackets.begin();
	if (itSubMap->second.first->sRPH.wSplitNum == 1)
	{
		//其实还需要考虑itSubMap.size() > 1时的异常情况
		PushRecvQueue(itSubMap->second.first);
		CheckSendACK(itSubMap->second.first->sRPH.iPacketSeq);
		return true;
	}
	else if (itSubMap->second.first->sRPH.wSplitNum == conSplitPackets.size()) //所有分包都已到齐，可以拼成整包了, 这个过程需要仔细看看
	{
		SReliabilityPacket *pWholeRP = CConnectionManager::GetInstance()->AllocRP();
		memcpy(&pWholeRP->sRPH, &itSubMap->second.first->sRPH, RPH_SIZE);
		pWholeRP->pBuf = CConnectionManager::GetInstance()->AllocBuf(pWholeRP->sRPH.iTotalSize);//new char[pWholeRP->sRPH.iTotalSize];
		pWholeRP->sRPH.iSingleSize = pWholeRP->sRPH.iTotalSize;
		WORD wWholeOffset = 0;
		int iSubBufLen = 0;
		for (;itSubMap!=conSplitPackets.end();++itSubMap)
		{
			iSubBufLen = itSubMap->second.first->sRPH.iSingleSize-RPH_SPH_SIZE;//((SSplitPacket *)(itSubMap->second.first->pBuf))->sSPH.iSplitPacketSize;
			memcpy(pWholeRP->pBuf + wWholeOffset, itSubMap->second.first->pBuf+SPH_SIZE, iSubBufLen);
			wWholeOffset += iSubBufLen;
		}
		PushRecvQueue(pWholeRP);
		CheckSendACK(pWholeRP->sRPH.iPacketSeq);
		return true;
	}

	return false;
}

int CReliabilityLayer::CheckConnection(const UINT64 qwToken)
{
	EConnectionState eCS = m_pCL->GetConnectionState();
	switch (eCS)
	{
	case E_STATE_NEW:
		{
			//First connect ,We should ignore the token, and then send a token to the client;
			//Server do like this
#if TYPE_SERVER == 1
			UINT64 qwValidToken = m_pCL->GetToken();
			if (qwToken == qwValidToken)
			{
				m_pCL->SetConnectionState(E_STATE_CHECK);
			}
			else
			{
				SReliabilityPacket *pRP = BuildToken(qwValidToken);
				m_pCL->PushSendQueue(pRP);
				map<WORD, pair<SReliabilityPacket*, DWORD> > conMap;
				conMap.insert(make_pair(pRP->sRPH.wSplitNum, make_pair(pRP, GetTickCount())));
				m_conPacketBufMap.insert(make_pair(pRP->sRPH.iPacketSeq, conMap));
				return 0;
			}
#else
			//Client do like this
			m_pCL->SetToken(qwToken);
			m_pCL->SetConnectionState(E_STATE_CHECK);
#endif
		}break;
	case E_STATE_CHECK:
		{
			int iRet = 1;
#if TYPE_SERVER == 1
			iRet = qwToken == m_pCL->GetToken() ? 1 : -1;
#endif
			return iRet;
		}break;
	case E_STATE_CLOSE:
		{
			return 0;
		}break;
	default:
		{
			return -1;
		}
	}
	return 1;
}

void CReliabilityLayer::PushRecvQueue(SReliabilityPacket *pRP)
{
	m_RecvQueueMutex.Lock();
	m_conRecvQueue.push(pRP);
	m_iNextRecvSeq = pRP->sRPH.iPacketSeq + 1;
	m_wNextRecvSplitSeq = 1;
	m_RecvQueueMutex.Unlock();
}

void CReliabilityLayer::CheckSendACK(DWORD dwSeq)
{
	if ((dwSeq+1) % SEND_ACK_PACKET == 0)
	{
		SReliabilityPacket * pRP = BuildACK(dwSeq, 1);
		m_pCL->PushResendQueue(pRP);
	}
}

SReliabilityPacket *CReliabilityLayer::GetRecvQueueBack()
{
	SReliabilityPacket *pRP = NULL;
	m_RecvQueueMutex.Lock();
	if (!m_conRecvQueue.empty())
	{
		pRP = m_conRecvQueue.back();
	}
	m_RecvQueueMutex.Unlock();
	return pRP;
}

SReliabilityPacket *CReliabilityLayer::FetchPacket()
{
	SReliabilityPacket *pRP = NULL;
	m_RecvQueueMutex.Lock();
	if (!m_conRecvQueue.empty())
	{
		pRP = m_conRecvQueue.front();
		m_conRecvQueue.pop();
	}
	m_RecvQueueMutex.Unlock();
	return pRP;
}

bool CReliabilityLayer::ClearCombinedPacket()
{
	SReliabilityPacket *pRP = NULL;
	m_RecvQueueMutex.Lock();
	while (!m_conRecvQueue.empty())
	{
		pRP = m_conRecvQueue.front();
		m_conRecvQueue.pop();
		CConnectionManager::GetInstance()->DeleteRP(pRP);
	}
	m_RecvQueueMutex.Unlock();
	return true;
}

bool CReliabilityLayer::ClearRecvBufMap()
{
	map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itBaseMap;
	map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itSubMap;
	m_PacketBufMapMutex.Lock();
	for (itBaseMap=m_conRecvPacketBufMap.begin();itBaseMap!=m_conRecvPacketBufMap.end();)
	{
		for (itSubMap = itBaseMap->second.begin();itSubMap!=itBaseMap->second.end();)
		{
			CConnectionManager::GetInstance()->DeleteRP(itSubMap->second.first);
			itSubMap = itBaseMap->second.erase(itSubMap);
		}
		itBaseMap = m_conRecvPacketBufMap.erase(itBaseMap);
	}
	m_PacketBufMapMutex.Unlock();
	return true;
}

#ifdef FOR_TEST
void CReliabilityLayer::RandomDropPacket(SReliabilityPacket *pRP)
{
	int iRandom = rand()%100;
	if (!(iRandom < 1 && m_iGlobalSeq != 1)) 
	{
		m_pCL->PushSendQueue(pRP);
	}
	else
	{
		WORD wSplitSeq = pRP->sRPH.wSplitNum > 1 ? *(WORD *)(pRP->pBuf) : 1;
		printf("Random lost:seq = %d, sub = %d\n",pRP->sRPH.iPacketSeq, wSplitSeq);
	}
}
#endif