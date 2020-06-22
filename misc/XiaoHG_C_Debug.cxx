
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include "XiaoHG_C_Socket.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_C_Log.h"

#define __THIS_FILE__ "XiaoHG_Debug.cxx"

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
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::PrintTDInfo track");

    time_t CurrTime = time(NULL);
    /* 10s iTimer */
    if((CurrTime - m_LastPrintTime) >= 9)
    {
        /* recv list */
        int tmprmqc = g_ThreadPool.getRecvMsgQueueCount(); 
        m_LastPrintTime = CurrTime;
        int tmpoLUC = m_OnLineUserCount;    /* Atomic transfer, print atomic type error directly */
        int tmpsmqc = m_iSendMsgQueueCount; /* Atomic transfer, print atomic type error directly */
        CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "------------------------------------begin--------------------------------------");
        CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "Current Online/Total(%d/%d)", tmpoLUC, m_EpollCreateConnectCount);
        CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "Connection free conn/Total/Release connection(%d/%d/%d)", 
                                                m_FreeConnectionList.size(), m_ConnectionList.size(), m_RecyConnectionList.size());   
        CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "Time queue size(%d)", m_TimerQueueMultiMap.size());     
        CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "Current receive queue size/send queue size(%d/%d)，Drop packets: %d",
                                                tmprmqc, tmpsmqc, m_iDiscardSendPkgCount);  
        if( tmprmqc > 100000)
        {
            /* The receiving queue is too large, report it, this should be alert, consider speed limit and other means */
            CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "The number of receive queues is too large(%d)，Need more！！！！！！", tmprmqc);  
        }
        CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "------------------------------------end--------------------------------------");
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