#ifndef __INCLUDED_RELIABILITY_LAYER_H_
#define __INCLUDED_RELIABILITY_LAYER_H_

#include "GlobalDefines.h"
#include <queue>

#define NAK_PACKET_SIZE 18
#define ACK_PACKET_SIZE 18

enum
{
	PACKET_SEQ_ACK = -2,
	PACKET_SEQ_NAK = -1,
};

#pragma pack(1)
struct SReliabilityPacketHeader
{
	SReliabilityPacketHeader():
		/*qwToken(0),*/iPacketSeq(0),iTotalSize(0),iSingleSize(0),wSplitNum(0)
	{
	}
	//UINT64 qwToken;
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

#define MAX_PACKET_SIZE 1482 // MAX_MTU_SIZE - sizeof(SReliabilityPacketHeader)
#define MAX_SPLIT_SIZE 1476  // MAX_PACKET_SIZE - sizeof(SSplitPacketHeader)
#define RPH_SIZE 14
#define SPH_SIZE 2
#define RPH_SPH_SIZE 16

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

//struct SPacketSendCtrl
//{
//	SReliabilityPacket *pRP;
//	DWORD dwNextSendTime;
//};

//#define CHECK_UDP_HEADER()

class CConnectionLayer;
class CReliabilityLayer
{
public:
	CReliabilityLayer(CConnectionLayer * pCL) 
	{
		m_pCL = pCL;
		m_iGlobalSeq = 0;
		m_iNextRecvSeq = 1;
		m_wNextRecvSplitSeq = 1;
	}
	~CReliabilityLayer() {};

	void CALLBACK OnTimer(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

	bool Send(const char *pBuf, const int iBufLen); //���ϲ�Ӧ�õ���
private:

	//queue<SReliabilityPacket *> & GetSendQueue() {return m_conSendQueue;}
	//queue<SReliabilityPacket *> & GetResendQueue() {return m_conResendQueue;}

	CConnectionLayer *m_pCL;
private:

	SReliabilityPacket * BuildNAK(const int iNAKSeq, const WORD wSplitSeq);
	SReliabilityPacket * BuildACK(const int iACKSeq, const WORD wSplitSeq);

	map<int, map<WORD, pair<SReliabilityPacket*, DWORD> > > m_conPacketBufMap;
	//queue<SReliabilityPacket *> m_conSendQueue;
	//queue<SReliabilityPacket *> m_conResendQueue;
	int  m_iGlobalSeq;

//////////////////////////////////////////////////////////////////////////////
public:
	bool Recieve(const char *pBuf, const int iBufLen);
	int GetRBRecieveSize() {return m_conRecvPacketBufMap.size();}
	int GetRBRecvSize() {return m_conRecvQueue.size();}

	//queue<SReliabilityPacket *> & GetRequestResendQueue() {return m_conRequestResendQueue;}
	SReliabilityPacket *GetWholePacket();  //���ϲ�Ӧ�õ���
private:

	bool CheckPacketLost(const int iRecvSeq, const WORD wSplitSeq);
	bool RecombinationPacket(map<WORD, pair<SReliabilityPacket*, DWORD/*recvTime*/> > &conSplitPackets);

	map<int, map<WORD, pair<SReliabilityPacket*, DWORD/*recvTime*/> > > m_conRecvPacketBufMap;   //��������н��յ��ģ��Ǿ�ȷ�ϻ�������������ݰ�
	queue<SReliabilityPacket *> m_conRecvQueue;           //����Ӧ�ò�ģ����򵽴���������ݰ����ְ�������ģ�
	//queue<SReliabilityPacket *> m_conRequestResendQueue;  //ACK��NAK�����У����ȷ���
	int m_iNextRecvSeq;                      //�ϴ�ȷ����������
	WORD m_wNextRecvSplitSeq;                //�ϴ�ȷ��������ְ����
	//BYTE m_byNAKNums;
};


#endif