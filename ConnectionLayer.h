#ifndef __INCLUDED_CONNECTION_LAYER_H_
#define __INCLUDED_CONNECTION_LAYER_H_

#include "GlobalDefines.h"
#include "ReliabilityLayer.h"

#include "ObjectPool.h"
#define SERVER_PORT 60000
#define CLIENT_PORT 60001

struct SRecvBuf
{
	SRecvBuf():
		pBuf(NULL),
		iTotalLen(0),
		iRecvLen(0)
	{
	}
	char *pBuf;
	int  iTotalLen;
	int  iRecvLen;
};

enum EConnectionState
{
	E_STATE_NEW = 1,
	E_STATE_CHECK = 2,
	E_STATE_CLOSE = 3,

	E_STATE_FAULT,
};

class CConnectionLayer
{
public:
	CConnectionLayer() {
		/*m_eCS = E_STATE_NEW;
		m_qwPrivateCode = -1;
		m_dwLastRecvTime = 0;*/

	}

	~CConnectionLayer() { 
		/*if (m_pRL != NULL)
		{
			delete m_pRL;
			m_pRL = NULL;
		}*/
	}

	bool Reset();

	SRecvBuf *PopRB();

	bool PushRB(const char *pBuf, int iLen);

	SReliabilityPacket *PopSendQueuePacket();

	void PushSendQueue(SReliabilityPacket * pRP);//其实这玩意肯定要判定队列长度的
	CReliabilityLayer *m_pRL;

	void PushResendQueue(SReliabilityPacket *pRP);
	//int m_iSentOffset;

	void SetToken(const UINT64 qwToken) { m_qwPrivateCode = qwToken;}
	UINT64 GetToken() const {return m_qwPrivateCode;}

	void SetLastRecvTime(const DWORD dwTime) {m_dwLastRecvTime = dwTime;}
	DWORD GetLastRecvTime() const {return m_dwLastRecvTime;}

	void SetConnectionState(const EConnectionState eCS) {m_eCS = eCS;}
	EConnectionState GetConnectionState() {return m_eCS;}
private:

	//SReliabilityPacket *m_pCurrSendPacket;
	queue<SReliabilityPacket *> m_conResendBufQueue;
	queue<SRecvBuf *> m_conRecvBufQueue; //接收队列都是同一个线程做的，应该没必要上锁
	queue<SReliabilityPacket *> m_conSendBufQueue;
	CSimpleMutex m_SendBufQueueMutex;

	EConnectionState m_eCS;
	UINT64 m_qwPrivateCode;

	DWORD m_dwLastRecvTime;
};

#endif