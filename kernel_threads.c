
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "util.h"



/* Initialize a PTCB */
void initialize_PTCB(PTCB* ptcb)
{
  
  ptcb->argl = 0;
  ptcb->args = NULL;
  ptcb->exited=0;
  ptcb->detached=0;
  ptcb->exitval=0;
  ptcb->refcount=0;
  ptcb->exit_cv = COND_INIT;

  rlnode_init(& ptcb->ptcb_list_node, ptcb);
}


/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{   
	  CURPROC->thread_count++;                        /*auxanei ta thread tou pcb*/
    PTCB* ptcb=(PTCB*)malloc(sizeof(PTCB));
    initialize_PTCB(ptcb);
    ptcb->refcount++;
    ptcb->task=task;
    ptcb->argl=argl;
    ptcb->args=args;
    rlist_push_front(& CURPROC->ptcb_list, & ptcb->ptcb_list_node);

    if(task!=NULL){
    ptcb->tcb=spawn_thread(CURPROC,start_thread);
    ptcb->tcb->ptcb=ptcb;                          /*sundeei to tcb me ptcb*/
    wakeup(ptcb->tcb);
    }

  return (Tid_t) ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) CURTHREAD->ptcb;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
	return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
    /*an einai to teleytaio thred*/
 if (CURPROC->thread_count==1){
   PCB *curproc = CURPROC;  /* cache for efficiency */

  /* Do all the other cleanup we want here, close files etc. */
  if(curproc->args) {
    free(curproc->args);
    curproc->args = NULL;
  }

  /* Clean up FIDT */
  for(int i=0;i<MAX_FILEID;i++) {
    if(curproc->FIDT[i] != NULL) {
      FCB_decref(curproc->FIDT[i]);
      curproc->FIDT[i] = NULL;
    }
  }

  /* Reparent any children of the exiting process to the 
     initial task */
  PCB* initpcb = get_pcb(1);
  while(!is_rlist_empty(& curproc->children_list)) {
    rlnode* child = rlist_pop_front(& curproc->children_list);
    child->pcb->parent = initpcb;
    rlist_push_front(& initpcb->children_list, child);
  }

  /* Add exited children to the initial task's exited list 
     and signal the initial task */
  if(!is_rlist_empty(& curproc->exited_list)) {
    rlist_append(& initpcb->exited_list, &curproc->exited_list);
    kernel_broadcast(& initpcb->child_exit);
  }

  /* Put me into my parent's exited list */
  if(curproc->parent != NULL) {   /* Maybe this is init */
    rlist_push_front(& curproc->parent->exited_list, &curproc->exited_node);
    kernel_broadcast(& curproc->parent->child_exit);
  }

  /* Disconnect my main_thread */
  curproc->main_thread = NULL;

  /* Now, mark the process as exited. */
  curproc->pstate = ZOMBIE;
  curproc->exitval = exitval;
 }
 else{
  
  PTCB* ptcb=CURTHREAD->ptcb;
  ptcb->refcount--;
  ptcb->exited=1;
  ptcb->exitval=exitval;
  CURPROC->thread_count--;
  //rlist_remove(ptcb_list_node);
 }
 kernel_sleep(EXITED, SCHED_USER);
}

