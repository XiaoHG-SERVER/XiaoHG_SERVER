
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_Global.h"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: PrintTDInfo
 * discription: print TD msg (dbg msg)
 * parameter:
 * =================================================================*/
void CSocket::PrintTDInfo()
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::PrintTDInfo track");

    time_t CurrTime = time(NULL);
    /* 10s iTimer */
    if((CurrTime - m_LastPrintTime) >= 9)
    {
        /* recv list */
        int tmprmqc = g_ThreadPool.getRecvMsgQueueCount(); 
        m_LastPrintTime = CurrTime;
        int tmpoLUC = m_OnLineUserCount;    /* Atomic transfer, print atomic type error directly */
        int tmpsmqc = m_iSendMsgQueueCount; /* Atomic transfer, print atomic type error directly */
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "------------------------------------begin--------------------------------------");
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Current Online/Total(%d/%d)", tmpoLUC, m_EpollCreateConnectCount);
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Connection free conn/Total/Release connection(%d/%d/%d)", 
                                                m_FreeConnectionList.size(), m_ConnectionList.size(), m_RecyConnectionList.size());   
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Time queue size(%d)", m_TimerQueueMultiMap.size());     
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Current receive queue size/send queue size(%d/%d)，Drop packets: %d",
                                                tmprmqc, tmpsmqc, m_iDiscardSendPkgCount);  
        if( tmprmqc > 100000)
        {
            /* The receiving queue is too large, report it, this should be alert, consider speed limit and other means */
            XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "The number of receive queues is too large(%d)，Need more！！！！！！", tmprmqc);  
        }
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "------------------------------------end--------------------------------------");
    }
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetVersion
 * discription: Get current version.
 * parameter:
 * =================================================================*/
const char* GetVersion()
{
    return __XiaoHG_VERSION__;
}