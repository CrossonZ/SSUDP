#include "ConnectionLayer.h"
#include "ConnectionManager.h"


bool CConnectionLayer::Reset()
{
	m_eCS = E_STATE_NEW;
	m_pRL = CConnectionManager::GetInstance()->AllocRL();
	m_pRL->SetCL(this);
	m_qwPrivateCode = -1;
	m_dwLastRecvTime = GetTickCount();

	while (!m_conRecvBufQueue.empty())
	{
		CConnectionManager::GetInstance()->DeleteRB(m_conRecvBufQueue.front());
		m_conRecvBufQueue.pop();
	}

	while (!m_conSendBufQueue.empty())
	{
		CConnectionManager::GetInstance()->DeleteRP(m_conSendBufQueue.front());
		m_conSendBufQueue.pop();
	}

	while (!m_conResendBufQueue.empty())
	{
		CConnectionManager::GetInstance()->DeleteRP(m_conResendBufQueue.front());
		m_conResendBufQueue.pop();
	}
	return true;
}

SRecvBuf *CConnectionLayer::PopRB()
{
	if (m_conRecvBufQueue.empty())
	{
		return NULL;
	}
	SRecvBuf *pRB = m_conRecvBufQueue.front();
	if (pRB->iRecvLen == pRB->iTotalLen)
	{
		m_conRecvBufQueue.pop();
		return pRB;
	}
	return NULL;
}

bool CConnectionLayer::PushRB(const char *pBuf, int iLen)
{
	SRecvBuf *pRB;

	if (m_conRecvBufQueue.empty() || m_conRecvBufQueue.back()->iTotalLen == m_conRecvBufQueue.back()->iRecvLen)
	{
		if (iLen < RPH_SIZE)
		{
			pRB = CConnectionManager::GetInstance()->AllocRB();//new SRecvBuf;
			if (pRB == NULL)
			{
				return false;
			}
			pRB->iRecvLen = iLen;
			pRB->iTotalLen = -1;
			pRB->pBuf = CConnectionManager::GetInstance()->AllocBuf(pRB->iRecvLen);//new char[pRB->iRecvLen];
			memcpy(pRB->pBuf, pBuf, iLen);
			m_conRecvBufQueue.push(pRB);
		}
		else
		{
			SReliabilityPacketHeader *pRPH;
			int iOffset = 0;
			while (iOffset < iLen)
			{
				pRPH = (SReliabilityPacketHeader *)(pBuf+iOffset);
				pRB = CConnectionManager::GetInstance()->AllocRB();//new SRecvBuf;
				if (pRB == NULL)
				{
					return false;
				}
				pRB->iTotalLen = pRPH->iSingleSize;
				pRB->iRecvLen = pRB->iTotalLen + iOffset > iLen ? iLen - iOffset : pRB->iTotalLen;
				pRB->pBuf = CConnectionManager::GetInstance()->AllocBuf(pRB->iTotalLen);//new char[pRB->iTotalLen];
				if (pRB->pBuf == NULL)
				{
					return false;
				}
				memcpy(pRB->pBuf, pBuf+iOffset, pRB->iRecvLen);
				iOffset += pRB->iRecvLen;
				m_conRecvBufQueue.push(pRB);
			}
		}
	}
	else
	{
		pRB = m_conRecvBufQueue.back();
		if (pRB->iRecvLen + iLen > pRB->iTotalLen)
		{
			SReliabilityPacketHeader *pRPH;
			int iOffset = pRB->iTotalLen-pRB->iRecvLen;
			memcpy(pRB->pBuf+pRB->iRecvLen, pBuf, iOffset); 
			pRB->iRecvLen = pRB->iTotalLen;
			while (iOffset < iLen)
			{
				pRPH = (SReliabilityPacketHeader *)(pBuf+iOffset);
				pRB = CConnectionManager::GetInstance()->AllocRB();//new SRecvBuf;
				if (pRB == NULL)
				{
					return false;
				}
				pRB->iTotalLen = pRPH->iSingleSize;
				pRB->iRecvLen = pRB->iTotalLen + iOffset > iLen ? iLen - iOffset : pRB->iTotalLen;
				pRB->pBuf = CConnectionManager::GetInstance()->AllocBuf(pRB->iTotalLen);//new char[pRB->iTotalLen];
				if (pRB->pBuf == NULL)
				{
					return false;
				}
				memcpy(pRB->pBuf, pBuf+iOffset, pRB->iRecvLen);
				iOffset += pRB->iRecvLen;
				m_conRecvBufQueue.push(pRB);
			}
		}
		else
		{
			memcpy(pRB->pBuf+pRB->iRecvLen, pBuf, iLen); 
			pRB->iRecvLen += iLen;
		}
	}

	SetLastRecvTime(GetTickCount());
	return true;
}

SReliabilityPacket *CConnectionLayer::PopSendQueuePacket()
{
	SReliabilityPacket *pRP = NULL;
	if (m_conResendBufQueue.empty())
	{

		m_SendBufQueueMutex.Lock();
		if (!m_conSendBufQueue.empty())
		{
			pRP = m_conSendBufQueue.front();
			m_conSendBufQueue.pop();
		}
		m_SendBufQueueMutex.Unlock();
	}
	else
	{
		pRP = m_conResendBufQueue.front();
		m_conResendBufQueue.pop();
	}
	return pRP;
}

void CConnectionLayer::PushSendQueue(SReliabilityPacket * pRP)
{
	m_SendBufQueueMutex.Lock();
	m_conSendBufQueue.push(pRP);//其实这玩意肯定要判定队列长度的
	m_SendBufQueueMutex.Unlock();
} 

void CConnectionLayer::PushResendQueue(SReliabilityPacket *pRP)
{
	m_conResendBufQueue.push(pRP);
}
