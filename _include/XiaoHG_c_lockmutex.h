
#ifndef __XiaoHG_LOCKMUTEX_H__
#define __XiaoHG_LOCKMUTEX_H__

#include <pthread.h>

/* This class is used to automatically release the mutex 
 * to prevent the situation of forgetting to call pthread_mutex_unlock */
class CLock
{
public:
	CLock(pthread_mutex_t *pMutex)
	{
		m_pMutex = pMutex;
		pthread_mutex_lock(m_pMutex); 	/* lock */
	}
	~CLock()
	{
		pthread_mutex_unlock(m_pMutex); /* unlock */
	}
private:
	pthread_mutex_t *m_pMutex;
};

#endif //!__XiaoHG_LOCKMUTEX_H__
