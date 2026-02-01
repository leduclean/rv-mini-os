#include "process.h"
#include "scheduler.h"

/** We prefer an english semantic for uniformity
 * and code clarity but we need aliases for CI eval **/
#define cree_processus spawn_process
#define ordonnance scheduler_rotate
#define dors scheduler_sleep
#define fin_processus scheduler_terminate
#define mon_pid get_active_pid
#define mon_nom get_active_name
#define nbr_secondes seconds
