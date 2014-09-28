#include "ConnectionLayer.h"

SRecvBuf *CConnectionLayer::PopRecvQueue()
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

bool CConnectionLayer::PushRecvQueue(const char *pBuf, int iLen)
{
	//SSecurityChecker sSH;
	//sSH.sin_addr = pSockAddr->sin_addr;
	//sSH.wPort = pSockAddr->sin_port;

	SRecvBuf *pRB;
	//map<SSecurityChecker *, queue<SRecvBuf *>, CCompSecurityHeader>::iterator itMap = m_conClientRecvMap.find(&sSH);
	//if (itMap != m_conClientRecvMap.end())
	//{
	//	queue<SRecvBuf *> &conRecvBufQueue = itMap->second;
		//if ()
		//{
		//	//异常数据， 处理为非法ip应用
		//	return false;
		//}
		if (m_conRecvBufQueue.empty() || m_conRecvBufQueue.back()->iTotalLen == m_conRecvBufQueue.back()->iRecvLen)
		{
			if (iLen < RPH_SIZE)
			{
				pRB = new SRecvBuf;
				if (pRB == NULL)
				{
					return false;
				}
				pRB->iRecvLen = iLen;
				pRB->iTotalLen = -1;
				pRB->pBuf = new char[pRB->iRecvLen];
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
					pRB = new SRecvBuf;
					if (pRB == NULL)
					{
						return false;
					}
					pRB->iTotalLen = pRPH->iSingleSize;
					pRB->iRecvLen = pRB->iTotalLen + iOffset > iLen ? iLen - iOffset : pRB->iTotalLen;
					pRB->pBuf = new char[pRB->iTotalLen];
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
					pRB = new SRecvBuf;
					if (pRB == NULL)
					{
						return false;
					}
					pRB->iTotalLen = pRPH->iSingleSize;
					pRB->iRecvLen = pRB->iTotalLen + iOffset > iLen ? iLen - iOffset : pRB->iTotalLen;
					pRB->pBuf = new char[pRB->iTotalLen];
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
	/*}
	else
	{
		queue<SRecvBuf *> conRecvBufQueue;
		if (iLen < SPH_SIZE)
		{
			pRB = new SRecvBuf;
			if (pRB == NULL)
			{
				return false;
			}
			pRB->iRecvLen = iLen;
			pRB->iTotalLen = -1;
			pRB->pBuf = new char[pRB->iRecvLen];
			memcpy(pRB->pBuf, pBuf, iLen);
			conRecvBufQueue.push(pRB);
		}
		else
		{
			SReliabilityPacketHeader *pRPH;
			int iOffset = 0;
			while (iOffset < iLen)
			{
				pRPH = (SReliabilityPacketHeader *)(pBuf+iOffset);
				pRB = new SRecvBuf;
				if (pRB == NULL)
				{
					return false;
				}
				pRB->iTotalLen = pRPH->iTotalSize;
				pRB->iRecvLen = pRB->iTotalLen + iOffset > iLen ? iLen - iOffset : pRB->iTotalLen;
				pRB->pBuf = new char[pRB->iTotalLen];
				if (pRB->pBuf == NULL)
				{
					return false;
				}
				memcpy(pRB->pBuf, pBuf+iOffset, pRB->iRecvLen);
				iOffset += pRB->iRecvLen;
				conRecvBufQueue.push(pRB);
			}
		}
		m_conClientRecvMap.insert(make_pair(&sSH, conRecvBufQueue));
	}*/
	return true;
}

//SReliabilityPacket *CConnectionLayer::PopSendQueue()
//{
//	if (m_conSendBufQueue.empty())
//	{
//		return NULL;
//	}
//	SReliabilityPacket *pRP = m_conSendBufQueue.front();
//	m_conSendBufQueue.pop();
//	return pRP;
//}
