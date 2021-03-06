#ifndef PROC_H
#define PROC_H
#include "valType.h"
#include<utils.h>
#include<ku_proc.h>


#define TASKMOD_EMPTY 0 	/**related process control block not initialized*/

#define TASKMOD_ACTIVE 1	/**process still has timeslice now*/
#define TASKMOD_EXPIRE 2	/**process uses out it's timeslice*/
#define TASKMOD_SLEEP 3		/**process halts waiting a signal*/


#define MSGTYPE_TIMER 255
#define MSGTYPE_DEEP 0
#define MSGTYPE_CHAR 1
#define MSGTYPE_FS_ASK 2
#define MSGTYPE_HD_DONE 3
#define MSGTYPE_HS_READY 4
#define MSGTYPE_HS_DONE 5
#define MSGTYPE_FS_READY 8
#define MSGTYPE_USR_ASK 6
#define MSGTYPE_FS_DONE 7


/**some important process's pid is fixed under papaya*/
#define HS_PID 1
#define FS_EXT_PID 2
/**papaya will support fat32,ntfs soon*/
#define FS_NTFS_PID 
#define FS_FAT16_PID
#define FS_FAT32_PID

/**keyboard buffer,the keyboard driver will send key-code to this structure if 
 * this process is listening on MSGTYPE_CHAR
 */
#define size_buffer 16
typedef struct{
	char c[size_buffer];
	int head;
	int tail;
	int num;
}OBUFFER;

/**1 these descriptor selectors are defined in kernel.asm
 * 2 why we need them?		every time we init a new pcb for a burning process,we will set it's ds,cs,fs,gs,ss as plain-memory mod
 * XXX this seems to be a ugly action,we can init these fields in a breath just when pcb_table is created,because they will be never touched any more.ok).
 * */
extern int selector_plain_c0,selector_plain_d0,selector_plain_c1,selector_plain_d1,selector_plain_c3,selector_plain_d3;
typedef struct{
	u32 gs;
	u32 fs;
	u32 es;
	u32 ds;
	u32 edii;//避免与disp.j模块的edi冲突
	u32 esi;
	u32 ebp;
	u32 kernel_esp;
	u32 ebx;
	u32 edx;
	u32 ecx;
	u32 eax;
	u32 retAddr;
	u32 eip;
	u32 cs;
	u32 eflags;
	u32 esp;
	u32 ss;
}STACK_FRAME;


typedef struct{
	STACK_FRAME regs;
	u32 pid;
	char* p_name;
	u32 prio;
	u32 time_slice;
	u32 time_slice_full;
	u32 mod;
	u32 msg_bind;
	u32 msg_type;
	OBUFFER obuffer;
	u32 cr3;
	u32 ring;
	int fix_cr3;//you must recover my cr3 when fire me
	//u16 sel_ldt;
	//DESCRIPTOR ldt[PCB_LDT_SIZE];
}PCB;

/**we record and update the basic information of pcb_table to this structure.(some members seem to be unnecessary,like num_active,num_expire..)*/
typedef struct{
	int num_task;
	int num_active;
	int num_expire;
	int num_sleep;
	int curr_pid;
}PCB_TABLE_INFO;

typedef struct{
	unsigned short back_link,__blh;
	unsigned long esp0;
	unsigned short ss0,__ss0h;
		unsigned long esp1;
	unsigned short ss1,__ss1h;
		unsigned long esp2;
	unsigned short ss2,__ss2h;
		unsigned long cr3;
	unsigned long eip;
	unsigned long eflags;
	unsigned long eax,ecx,edx,ebx;
	unsigned long esp;
	unsigned long ebp;
	unsigned long esi;
	unsigned long edii;
	unsigned short es, __esh;
	unsigned short cs, __csh;
	unsigned short ss, __ssh;
	unsigned short ds, __dsh;
	unsigned short fs, __fsh;
	unsigned short gs, __gsh;
	unsigned short ldt, __ldth;
	unsigned short trace, bitmap;
	unsigned long __cacheline_filler[5]  ;
}TSS;

/**macros for task mode switch*/
#define ACTIVE_EXPIRE(pid)\
do{\
	assert(pcb_table[pid].mod==TASKMOD_ACTIVE)\
	pcb_table_info.num_expire++;\
	pcb_table_info.num_active--;\
	pcb_table[pid].mod=TASKMOD_EXPIRE;\
} while(0)

#define ACTIVE_SLEEP(pid)\
do{\
	assert(pcb_table[pid].mod==TASKMOD_ACTIVE)\
	pcb_table_info.num_sleep++;\
	pcb_table_info.num_active--;\
	pcb_table[pid].mod=TASKMOD_SLEEP;\
	pcb_table[pid].msg_type=MSGTYPE_DEEP;\
} while(0)

#define ACTIVE_SLEEP1(pid,msg_type,msg_bind)\
do{\
	assert(pcb_table[pid].mod==TASKMOD_ACTIVE)\
	pcb_table_info.num_sleep++;\
	pcb_table_info.num_active--;\
	pcb_table[pid].msg_type=msg_type;\
	pcb_table[pid].msg_bind=msg_bind;\
	pcb_table[pid].mod=TASKMOD_SLEEP;\
} while(0)

#define SLEEP_EXPIRE(pid)\
do{\
assert(pcb_table[pid].mod==TASKMOD_SLEEP)\
pcb_table_info.num_expire++;\
pcb_table_info.num_sleep--;\
pcb_table[pid].mod=TASKMOD_EXPIRE;\
} while(0)

#define SLEEP_ACTIVE(pid)\
do{\
assert(pcb_table[pid].mod==TASKMOD_SLEEP)\
	pcb_table_info.num_active++;\
	pcb_table_info.num_sleep--;\
	pcb_table[pid].mod=TASKMOD_ACTIVE;\
} while(0)

#define EXPIRE_SLEEP(pid)\
do{\
	assert(pcb_table[pid].mod==TASKMOD_EXPIRE)\
	pcb_table_info.num_sleep++;\
	pcb_table_info.num_expire--;\
	pcb_table[pid].mod=TASKMOD_SLEEP;\
} while(0)

#define EXPIRE_ACTIVE(pid)\
do{\
	assert(pcb_table[pid].mod==TASKMOD_EXPIRE)\
	pcb_table_info.num_active++;\
	pcb_table_info.num_expire--;\
	pcb_table[pid].mod=TASKMOD_ACTIVE;\
} while(0)

/**system call always return a value,obviouslly we can not write like 'rerurn xx',but things are similarily easy,just set the 'eax'-field of process who traps into kernel mod*/
#define SET_PID_EAX(pid,return_val)	pcb_table[pid].regs.eax=return_val

void proc_dispatch(void);
int pickNext(void);
void fire(int pid);
void fire_asm(u32 addr_pcb);
void proc_init(void);
void create_kernel_process(u32 addr,int prio,int time_slice,char*p_name,int ring);
void create_usr_process(char*exec_file,int prio,int time_slice,char*p_name,int father_pid);
PCB pcb_table[MAX_TASK];
PCB_TABLE_INFO pcb_table_info;

void obuffer_init(OBUFFER* pt_obuffer);
void obuffer_push(OBUFFER* pt_obuffer,char c);
unsigned char obuffer_shift(OBUFFER* pt_obuffer);
int getEmpty(void);
int pickEmpty(void);
void kill(int pid);
#endif



























