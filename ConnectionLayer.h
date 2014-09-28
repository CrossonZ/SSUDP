#ifndef __INCLUDED_CONNECTION_LAYER_H_
#define __INCLUDED_CONNECTION_LAYER_H_

#include "GlobalDefines.h"
#include "ReliabilityLayer.h"
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

class CConnectionLayer
{
public:
	CConnectionLayer() { m_pRL = new CReliabilityLayer(this);}
	~CConnectionLayer() {;}

	SRecvBuf *PopRecvQueue();

	bool PushRecvQueue(const char *pBuf, int iLen);

	SReliabilityPacket *GetSendQueuePacket() {return m_conSendBufQueue.empty()?NULL:m_conSendBufQueue.front(); }
	void RemoveSendQueuePacket() {m_conSendBufQueue.pop();}

	void PushSendQueue(SReliabilityPacket * pRP) { m_conSendBufQueue.push(pRP); } //其实这玩意肯定要判定队列长度的
	CReliabilityLayer *m_pRL;
private:

	queue<SRecvBuf *> m_conRecvBufQueue;
	queue<SReliabilityPacket *> m_conSendBufQueue;
};

#endif