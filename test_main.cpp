//#include "ReliabilityLayer.h"
#include "ConnectionManager.h"

int main()
{
	CConnectionManager *pCM = new CConnectionManager;
	//if (pCM->Init())
	//{
	//	while (1)
	//	{
	//		Sleep(1000);
	//	}
	//}
	
	//Initialize winsock 2 
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
	while(1)
	{
		Sleep(10000);
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