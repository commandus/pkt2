#include <stdio.h> 
#include <string.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
#define SCHED_FIFO	0
#else
#include <sched.h>
#include <unistd.h>
#include <sys/time.h>

#include <sys/resource.h>
#include <sys/mman.h>
#endif

#include "utilpriority.h"

/**
  *	Set current process priority
  *	\param [in]	priority	-20..20, maximum- -20
  *	\param [in]	shedulepolicy	SCHED_FIFO
  *	\param [in]	dive		0, 1
  *	\return		0	success
  *			1	error fork
  *			2	error set priority
  *			3	error set RR sheduling
  */
int pkt2utilpriority::setPriority(
	int priority,
	int shedulepolicy,
	int dive
)
{
#if defined(_WIN32) || defined(_WIN64)
	// TODO
#else
	if (dive)
	{
		struct rlimit rl;
		if (fork() != 0)	// Порождает дочерний процесс. Родительскому процессу возвращается ID дочернего процесса, а дочернему 0
			return 1;
		setsid(); 		// Вызывающий процесс становится ведущим в группе, ведущим процессом нового сеанса и не имеет контролирующего терминала
		setpgrp();		
		setsid();
		getrlimit(RLIMIT_NOFILE, &rl);	// Получение различных лимитов для пользователя
		for (int i = 0; i <= rl.rlim_max; i++)
			(void) close(i);
		
	}
	
	mlockall(MCL_FUTURE); 	// разрешает страничный обмен для всех страниц в области памяти вызывающего процесса
	errno = 0;
	setpriority(PRIO_PROCESS, getpid(), priority);
	if (errno) 
		return 2;
	
	struct sched_param sp;

	memset(&sp, 0, sizeof(sp));	// очистка struct sched_param sp, write 0 in memory &sp
	sp.sched_priority = sched_get_priority_max(shedulepolicy);	// Получение максимального/минимального значения приоритета реального времени
	// Установка политики диспетчеризации процесса
	if (sched_setscheduler(0, shedulepolicy, &sp) != 0) 
		return 3;
#endif
	return 0;
}

int pkt2utilpriority::setMaxPriority(int dive)
{
	return pkt2utilpriority::setPriority(dive, -20, SCHED_FIFO);
}
