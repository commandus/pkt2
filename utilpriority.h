/**
  *	utilpriority.h
  *	\sa	V:\_Шал\Program_заготовки\shm_splc2s_sec\shm_splc2s_sec.c main.c
  */
#ifndef UTIL_PRIORITY_H
#define UTIL_PRIORITY_H     1

namespace pkt2utilpriority {
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
int setPriority(
	int priority,
	int shedulepolicy,
	int dive
);

int setMaxPriority(int dive);

}
#endif