#ifndef __INCLUDED_RELIABILITY_LAYER_H_
#define __INCLUDED_RELIABILITY_LAYER_H_

#include "GlobalDefines.h"
#include "SimpleMutex.h"
#include <queue>

#define NAK_PACKET_SIZE 26
#define ACK_PACKET_SIZE 26
#define TOKEN_PACKET_SIZE 30


#define MAX_PACKET_SIZE 1470 // MAX_MTU_SIZE - sizeof(SReliabilityPacketHeader)
#define MAX_SPLIT_SIZE 1468  // MAX_PACKET_SIZE - sizeof(SSplitPacketHeader)

#define DUMPLICATE_NAK 10
#define SEND_ACK_PACKET 10

enum
{
	PACKET_SEQ_ACK = -2,
	PACKET_SEQ_NAK = -1,
};

#pragma pack(1)
struct SReliabilityPacketHeader
{
	SReliabilityPacketHeader():
		qwToken(0),iPacketSeq(0),iTotalSize(0),iSingleSize(0),wSplitNum(0)
	{
	}
	UINT64 qwToken;
	int iPacketSeq;
	int iTotalSize;
	int iSingleSize;
	WORD wSplitNum;
};

struct SSplitPacketHeader
{
	SSplitPacketHeader():
		wSplitSeq(0)//,iSplitPacketSize(0)
	{

	}
	WORD wSplitSeq;
	//int iSplitPacketSize;
};
#pragma pack()

#define RPH_SIZE 22
#define SPH_SIZE 2
#define RPH_SPH_SIZE 24

struct SReliabilityPacket
{
	SReliabilityPacket()
	{
		pBuf = NULL;
	}
	SReliabilityPacketHeader sRPH;
	char *pBuf;
};

struct SSplitPacket
{
	SSplitPacket()
	{
		pBuf = NULL;
	}
	SSplitPacketHeader sSPH;
	char *pBuf;
};

class CConnectionLayer;
class CReliabilityLayer
{
public:
	CReliabilityLayer() 
	{

	}
	~CReliabilityLayer() {};
	void SetCL(CConnectionLayer * pCL) {m_pCL = pCL;}
	void Reset();

	bool Send(const char *pBuf, const int iBufLen); //供上层应用调用
	SReliabilityPacket * BuildToken(const UINT64 qwToken);

	bool ClearSendBufMap();
private:

	CConnectionLayer *m_pCL;
	SReliabilityPacket * BuildNAK(const int iNAKSeq, const WORD wSplitSeq);
	SReliabilityPacket * BuildACK(const int iACKSeq, const WORD wSplitSeq);


	map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > > m_conPacketBufMap;
	CSimpleMutex m_PacketBufMapMutex;
	int  m_iGlobalSeq;  //因为token永远为第一个包，用不着锁

//////////////////////////////////////////////////////////////////////////////
public:
	bool Recieve(const char *pBuf, const int iBufLen);
	int GetRBRecvBufSize() {return m_conRecvPacketBufMap.size();}
	int GetRBRecvQueueSize();

	//queue<SReliabilityPacket *> & GetRequestResendQueue() {return m_conRequestResendQueue;}
	SReliabilityPacket *FetchPacket();  //供上层应用调用

	bool ClearCombinedPacket();
	bool ClearRecvBufMap();
private:
	int CheckConnection(const UINT64 qwToken);

	void PushRecvQueue(SReliabilityPacket *pRP);

	void CheckSendACK(DWORD dwSeq);
#ifdef FOR_TEST
	void RandomDropPacket(SReliabilityPacket *pRP);
#endif
	bool CheckPacketLost(const int iRecvSeq, const WORD wSplitSeq);
	bool RecombinationPacket(map<WORD, pair<SReliabilityPacket*, DWORD/*recvTime*/> > &conSplitPackets);

	SReliabilityPacket *GetRecvQueueBack();

	map<int, map<WORD, pair<SReliabilityPacket*, DWORD/*recvTime*/> > > m_conRecvPacketBufMap;   //缓存的所有接收到的，非经确认或非完成重组的数据包

	queue<SReliabilityPacket *> m_conRecvQueue;           //发往应用层的，按序到达的完整数据包（分包重组过的）
	CSimpleMutex m_RecvQueueMutex;                        //完整数据队列锁
	//queue<SReliabilityPacket *> m_conRequestResendQueue;  //ACK和NAK包队列，优先发送
	int m_iNextRecvSeq;                      //上次确认有序包序号
	WORD m_wNextRecvSplitSeq;                //上次确认有序包分包编号

	int m_iLastNAKSeq;
	WORD m_wLastNAKSplitSeq;
	WORD m_wDumplicateNAKNum;                
	//BYTE m_byNAKNums;
};


#endif