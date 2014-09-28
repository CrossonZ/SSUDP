#include "ReliabilityLayer.h"
#include "ConnectionLayer.h"
#include <time.h>
#include <stdio.h>

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
	if (wSplitNum > 0)  //����Ҫ�ְ�����
	{
		wSplitNum = iBufLen / MAX_SPLIT_SIZE;
		WORD wSplitCount = wSplitNum + 1? iBufLen % MAX_SPLIT_SIZE : wSplitNum;
		while (wSplitNum > 0)
		{
			pRP = new SReliabilityPacket;
			if (pRP == NULL)
			{
				return false;
			}
			pRP->sRPH.iPacketSeq = m_iGlobalSeq;
			pRP->sRPH.iTotalSize = iBufLen;
			pRP->sRPH.iSingleSize = MAX_SPLIT_SIZE+RPH_SIZE;
			pRP->sRPH.wSplitNum = wSplitCount;
			pRP->pBuf = new char [MAX_PACKET_SIZE];
			
			//sSPH.iSplitPacketSize = MAX_SPLIT_SIZE;
			sSPH.wSplitSeq = ++wSplitSeq;
			memcpy(pRP->pBuf,  &sSPH, SPH_SIZE);
			memcpy(pRP->pBuf+SPH_SIZE, pBuf+(wSplitSeq-1)*MAX_SPLIT_SIZE, MAX_SPLIT_SIZE);

			conSendCtrlMap.insert(make_pair(wSplitSeq, make_pair(pRP, dwTime+SEND_TIME_OUT)));
			m_pCL->PushSendQueue(pRP);
			--wSplitNum;
		}
		if (iBufLen % MAX_SPLIT_SIZE)
		{
			pRP = new SReliabilityPacket;
			if (pRP == NULL)
			{
				return false;
			}
			pRP->sRPH.iPacketSeq = m_iGlobalSeq;
			pRP->sRPH.iTotalSize = iBufLen;
			pRP->sRPH.iSingleSize = iBufLen % MAX_SPLIT_SIZE + RPH_SPH_SIZE;
			pRP->sRPH.wSplitNum = wSplitCount;
			pRP->pBuf = new char [iBufLen % MAX_SPLIT_SIZE + SPH_SIZE];

			//sSPH.iSplitPacketSize = iBufLen % MAX_SPLIT_SIZE;
			sSPH.wSplitSeq = ++wSplitSeq;
			memcpy(pRP->pBuf,  &sSPH, SPH_SIZE);
			memcpy(pRP->pBuf+SPH_SIZE, pBuf+(wSplitSeq-1)*MAX_SPLIT_SIZE, iBufLen % MAX_SPLIT_SIZE);

			conSendCtrlMap.insert(make_pair(wSplitSeq, make_pair(pRP, dwTime+SEND_TIME_OUT)));
			m_pCL->PushSendQueue(pRP);
		}
	} 
	else
	{
		pRP = new SReliabilityPacket;
		if (pRP == NULL)
		{
			return false;
		}
		pRP->sRPH.iPacketSeq = m_iGlobalSeq;
		pRP->sRPH.iTotalSize = iBufLen;
		pRP->sRPH.iSingleSize = iBufLen + RPH_SIZE;
		pRP->sRPH.wSplitNum = 1;
		pRP->pBuf = new char [iBufLen];

		memcpy(pRP->pBuf, pBuf, iBufLen);

		conSendCtrlMap.insert(make_pair(wSplitSeq, make_pair(pRP, dwTime+SEND_TIME_OUT)));
		m_pCL->PushSendQueue(pRP);
		--wSplitNum;
	}
	m_conPacketBufMap.insert(make_pair(m_iGlobalSeq, conSendCtrlMap));
	return true;
}

SReliabilityPacket * CReliabilityLayer::BuildNAK(const int iNAKSeq, const WORD wSplitSeq)
{
	SReliabilityPacket * pRP = new SReliabilityPacket;
	pRP->sRPH.iPacketSeq = PACKET_SEQ_NAK;
	pRP->sRPH.iTotalSize = 4;
	pRP->sRPH.iSingleSize = NAK_PACKET_SIZE;
	pRP->sRPH.wSplitNum = wSplitSeq;
	pRP->pBuf = new char[4];
	memcpy(pRP->pBuf, (char *)&iNAKSeq, 4);
	return pRP;
}

SReliabilityPacket * CReliabilityLayer::BuildACK(const int iACKSeq, const WORD wSplitSeq)
{
	SReliabilityPacket * pRP = new SReliabilityPacket;
	pRP->sRPH.iPacketSeq = PACKET_SEQ_ACK;
	pRP->sRPH.iTotalSize = 4;
	pRP->sRPH.iSingleSize = ACK_PACKET_SIZE;
	pRP->sRPH.wSplitNum = wSplitSeq;
	pRP->pBuf = new char[4];
	memcpy(pRP->pBuf, (char *)&iACKSeq, 4);
	return pRP;
}

bool CReliabilityLayer::Recieve(const char *pBuf, const int iBufLen)
{
	if (iBufLen < SPH_SIZE)
	{
		return false;
	}

	SReliabilityPacketHeader *pRPH = (SReliabilityPacketHeader *)pBuf;

	//������Ҫ��һ�����Ϸ��Լ�����

	if (pRPH->iPacketSeq == PACKET_SEQ_ACK)
	{
		//����ɾ������
		return true;
	}
	else if (pRPH->iPacketSeq == PACKET_SEQ_NAK)
	{
		//���ж����ش�����
		return true;
	}

	if (pRPH->wSplitNum == 1) //�޷ְ�
	{
		if (m_conRecvPacketBufMap.find(pRPH->iPacketSeq) != m_conRecvPacketBufMap.end())
		{
			return true; //�ظ���,����
		}
		SReliabilityPacket *pRP = new SReliabilityPacket;
		pRP->pBuf = new char[iBufLen-RPH_SIZE];
		memcpy(&pRP->sRPH, pRPH, RPH_SIZE); 
		memcpy(pRP->pBuf, pBuf+RPH_SIZE, iBufLen-RPH_SIZE);
		//�������ش����
		if (CheckPacketLost(pRPH->iPacketSeq, 1))
		{
			map<WORD, pair<SReliabilityPacket*, DWORD> > conMap;
			conMap.insert(make_pair(0, make_pair(pRP, GetTickCount())));
			m_conRecvPacketBufMap.insert(make_pair(pRP->sRPH.iPacketSeq, conMap));	
		}
		else
		{
			++m_iNextRecvSeq;
			m_wNextRecvSplitSeq = 1;
			m_conRecvQueue.push(pRP);
			map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itBaseMap = m_conRecvPacketBufMap.find(m_iNextRecvSeq);
			while(itBaseMap != m_conRecvPacketBufMap.end())
			{
				if (!RecombinationPacket(itBaseMap->second))
				{
					break;
				}
				else
				{
					//ָ������գ����г�֮����
					itBaseMap = m_conRecvPacketBufMap.erase(itBaseMap);
				}
			}
		}
	}
	else 
	{
		//�ְ�����
		SSplitPacketHeader *pSPH = (SSplitPacketHeader *)(pBuf+RPH_SIZE);
		map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > >::iterator itBaseMap = m_conRecvPacketBufMap.find(pRPH->iPacketSeq);
		if (itBaseMap == m_conRecvPacketBufMap.end()) //�ְ��еĵ�һ����
		{
			if (!CheckPacketLost(pRPH->iPacketSeq, pSPH->wSplitSeq))
			{
				++m_wNextRecvSplitSeq;
			}
	
			SReliabilityPacket *pRP = new SReliabilityPacket;
			pRP->pBuf = new char[iBufLen-RPH_SIZE];

			memcpy(&pRP->sRPH, pRPH, RPH_SIZE);
			memcpy(pRP->pBuf, pBuf+RPH_SIZE, iBufLen-RPH_SIZE);
			map<WORD, pair<SReliabilityPacket*, DWORD> > conMap;
			conMap.insert(make_pair(1, make_pair(pRP, GetTickCount())));
			m_conRecvPacketBufMap.insert(make_pair(pRP->sRPH.iPacketSeq, conMap));
			//��ʱ�����а��ϲ�����ԭ���ǲ����ܳ�������
		}
		else
		{
			map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itSubMap = itBaseMap->second.find(pSPH->wSplitSeq);
			if (itSubMap != itBaseMap->second.end())
			{
				return false; //�ظ���,����
			}
			SReliabilityPacket *pRP = new SReliabilityPacket;
			pRP->pBuf = new char[iBufLen-RPH_SIZE];
			memcpy(&pRP->sRPH, pRPH, RPH_SIZE);
			memcpy(pRP->pBuf, pBuf+RPH_SIZE, iBufLen-RPH_SIZE);
			itBaseMap->second.insert(make_pair(pSPH->wSplitSeq, make_pair(pRP,GetTickCount())));
			//�������������
			CheckPacketLost(pRPH->iPacketSeq, pSPH->wSplitSeq);

			while(itBaseMap != m_conRecvPacketBufMap.end())
			{
				if (!RecombinationPacket(itBaseMap->second))
				{
					break;
				}
				else
				{
					//ָ������գ����г�֮����
					itBaseMap = m_conRecvPacketBufMap.erase(itBaseMap);
				}
			}
		}
	}
	return true;
}

bool CReliabilityLayer::CheckPacketLost(const int iRecvSeq, const WORD wSplitSeq)
{
	if (iRecvSeq == m_iNextRecvSeq && wSplitSeq == m_wNextRecvSplitSeq)
	{
		return false;
	}
	SReliabilityPacket *pRP = BuildNAK(m_iNextRecvSeq, m_wNextRecvSplitSeq);
	//m_conRequestResendQueue.push(pRP);
	m_pCL->PushSendQueue(pRP);
	return true;
}

bool CReliabilityLayer::RecombinationPacket(map<WORD, pair<SReliabilityPacket*, DWORD/*recvTime*/> > &conSplitPackets)
{

	map<WORD, pair<SReliabilityPacket*, DWORD> >::iterator itSubMap = conSplitPackets.begin();
	if (itSubMap->second.first->sRPH.wSplitNum == 1)
	{
		//��ʵ����Ҫ����itSubMap.size() > 1ʱ���쳣���
		m_conRecvQueue.push(itSubMap->second.first);
		++m_iNextRecvSeq;
		m_wNextRecvSplitSeq = 1;
		return true;
	}
	else if (itSubMap->second.first->sRPH.wSplitNum == conSplitPackets.size()) //���зְ����ѵ��룬����ƴ��������, ���������Ҫ��ϸ����
	{
		SReliabilityPacket *pWholeRP = new SReliabilityPacket;
		memcpy(&pWholeRP->sRPH, &itSubMap->second.first->sRPH, RPH_SIZE);
		pWholeRP->pBuf = new char[pWholeRP->sRPH.iTotalSize];
		WORD wWholeOffset = 0;
		int iSubBufLen = 0;
		for (;itSubMap!=conSplitPackets.end();++itSubMap)
		{
			iSubBufLen = itSubMap->second.first->sRPH.iSingleSize-RPH_SPH_SIZE;//((SSplitPacket *)(itSubMap->second.first->pBuf))->sSPH.iSplitPacketSize;
			memcpy(pWholeRP->pBuf + wWholeOffset, itSubMap->second.first->pBuf+SPH_SIZE, iSubBufLen);
			wWholeOffset += iSubBufLen;
		}
		++m_iNextRecvSeq;
		m_wNextRecvSplitSeq = 1;
		return true;
	}

	return false;
}

SReliabilityPacket *CReliabilityLayer::GetWholePacket()
{
	SReliabilityPacket *pRP = m_conRecvQueue.back();
	m_conRecvQueue.pop();
	return pRP;
}

/*void CALLBACK CReliabilityLayer::OnTimer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	//Send Queue;
	if (!m_conResendQueue.empty())
	{
		//int nRecEcho = sendto(socka, recv_buf, sizeof(recv_buf), 0, (sockaddr*)&cli, &nRecLen);  
	}
	return;
}*/
