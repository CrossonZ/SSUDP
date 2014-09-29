//#include "ReliabilityLayer.h"
#include "ConnectionManager.h"

int main()
{
	CConnectionManager *pCM = CConnectionManager::GetInstance();
	
	//Initialize winsock 
	pCM->Init(); 

	//We want to keep the main thread running 
	/*HANDLE hWait2Exit = CreateEvent(NULL,FALSE,TRUE,"MCLIENT"); 
	ResetEvent(hWait2Exit ); 

	//This OVERLAPPED event 
	g_hReadEvent = CreateEvent(NULL,TRUE,TRUE,NULL); 

	// 
	// try to get timing more accurate... Avoid context 
	// switch that could occur when threads are released 
	// 

	SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL); */
	if (!pCM->CreateConnection()) 
	{ 
/*		printf("Error condition @ CreateNetConnections , exiting\n"); 
		return 1; */
	} 

	HANDLE pFile1 = CreateFile("E:\\project\\SSUDP\\dest_1", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
	HANDLE pFile2 = CreateFile("E:\\project\\SSUDP\\dest_2", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
	HANDLE pFile3 = CreateFile("E:\\project\\SSUDP\\dest_3", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
	HANDLE pFile4 = CreateFile("E:\\project\\SSUDP\\dest_4", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);

	/*if (pFile == INVALID_HANDLE_VALUE)
	{
		return 1;
	}

	DWORD dwFileType = GetFileType(pFile);
	if (dwFileType != FILE_TYPE_DISK)
	{
		return 1;
	}*/
	UINT64 qwToken = 0;
	HANDLE pFile = NULL;
	int iFileCode = 1;
	DWORD tTimeout = GetTickCount();
	bool bCloseFlag[4] = {false}, bStartFlag = false;
	DWORD dwActualWrite = 0;
	SReliabilityPacket *pPacket = NULL;
	while(1)
	{
		map<sockaddr_in, CConnectionLayer *, ConnectionAddrComp>::iterator itMap = pCM->m_conClientRecvMap.begin();
		for (;itMap!=pCM->m_conClientRecvMap.end();++itMap)
		{
			pPacket = itMap->second->m_pRL->FetchPacket();
			if (pPacket != NULL)
			{
				qwToken = itMap->second->GetToken();
				switch(qwToken)
				{
				case 1:
					pFile = pFile1;
					iFileCode = 1;
					break;
				case 2:
					pFile = pFile2;
					iFileCode = 2;
					break;
				case 3:
					pFile = pFile3;
					iFileCode = 3;
					break;
				case 4:
					pFile = pFile4;
					iFileCode = 4;
					break;
				default:
					break;
				}
				WriteFile(pFile, pPacket->pBuf,pPacket->sRPH.iTotalSize, &dwActualWrite, NULL);
				//fwrite(pPacket->pBuf,  pPacket->sRPH.iTotalSize, fp);
				printf("token:%llu buffer seq :%d totalsize :%d\n",qwToken, pPacket->sRPH.iPacketSeq, pPacket->sRPH.iTotalSize);
				tTimeout = GetTickCount();
				CConnectionManager::GetInstance()->DeleteRP(pPacket);
				bStartFlag = true;
			}

		}
		if (GetTickCount() > tTimeout + 15000 && !bCloseFlag[iFileCode-1] && bStartFlag)
		{
			//fclose(fp);
			FlushFileBuffers(pFile);
			bCloseFlag[iFileCode-1] = true;
		}
		Sleep(100);
	}
	//if (!pCM->CreateWorkingThread(5)) 
	//{ 
	//	printf("Error condition @CreateWorkers, exiting\n"); 
	//	return 1; 
	//} 

	//WaitForSingleObject(hWait2Exit,INFINITE); 
	pCM->UnInit(); 
	return 0; 
}
