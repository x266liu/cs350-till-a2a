#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <mips/trapframe.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include "opt-A2.h"
#include <synch.h>
#include <limits.h>//PATH_MAX
#include <vfs.h>
#include <kern/fcntl.h>
#include <test.h>
#include <vm.h>
#include "autoconf.h"
  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */
#if OPT_A2
pid_t sys_fork(pid_t *retval,struct trapframe *tf){
 // KASSERT(curproc != NULL);
  //KASSERT(sizeof(struct trapframe) == (37*4));
 // struct proc *cp = curproc;
  int errthread;
  struct proc * child = proc_create_runprogram(curproc->p_name);
  if(child == NULL){
    return ENOMEM;
  }
  
  as_copy(curproc_getas(), &(child->p_addrspace));
  if(child->p_addrspace == NULL){
    proc_destroy(child);
    return ENOMEM;
  }
  struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));
  if(child_tf == NULL){
    proc_destroy(child);
    return ENOMEM;
  }

  memcpy(child_tf, tf, sizeof(struct trapframe));
  errthread = thread_fork(curthread->t_name, child, &enter_forked_process, child_tf,0);
  if(errthread){
    proc_destroy(child);
    kfree(child_tf);
    child_tf = NULL;
    return errthread;
  }
  
  array_add(&curproc->p_children, child, NULL);
  
  lock_acquire(child->p_exit_lk);

  
  *retval = child->p_pid;
  return 0;
}

#endif
#if OPT_A2
int
sys_execv(const char *program, char **args, int32_t *retval){
 /* int errcp, errpath;
  struct vnode *v;
  struct addrspace *exev_as;
  vaddr_t entrypoint, stackptr;

  KASSERT(program != NULL);
  KASSERT(args != NULL);//assertion


  char **kernargs = (char **) kmalloc(sizeof(char**));  
  int number = 0;
  int *str = (int *)kmalloc(sizeof(int *));
  while(args[number] != NULL){
    str[number] = strlen(args[number]) + 1;//plus one for null terminator
    number++; 
  }
  
  if(number == 0){
    errcp = copyin((const_userptr_t) args, kernargs, (size_t)0);//case that there are no argument
  }  else{
    errcp = copyin((const_userptr_t) args, kernargs, (size_t)number);
  } //for copy args into kernel
  if(errcp){
    kfree(str);
    kfree(kernargs);
    return 1;
  }

  char *kernpath = (char *) kmalloc(sizeof(char*) * PATH_MAX); 
  size_t path;
  
  errpath = copyinstr((const_userptr_t) program, kernpath, PATH_MAX, &path);
  if(errpath){
    kfree(str);
    kfree(kernargs);
    kfree(kernpath);
    return EFAULT;
  }//copy program path into kernel
  if(path == 1){
    kfree(str);
    kfree(kernargs);
    kfree(kernpath);
    return  path;
  }
  for(int i = 0; i < number; i++){
        kernargs[i] = (char *)kmalloc(sizeof(char *) * PATH_MAX);
        errcp = copyinstr((const_userptr_t) args[i], kernargs[i], PATH_MAX, &path);
        if(errcp){
          kfree(str);
          kfree(kernpath);
          kfree(kernargs);
          return errcp;
        }
        if(i == number - 1){
          kernargs[i] = NULL;
        }
  }

  
  int result;
  result = vfs_open(kernpath, O_RDONLY, 0, &v);
  if(result){
    return result;
  }

  //  KASSERT(curproc_getas() == NULL);

  //exev_as = as_create();
  exev_as = curthread->t_addrspace;

  if(curthread->t_addrspace != NULL){
    as_destroy(curthread->t_addrspace);
    curthread->t_addrspace = NULL;
  }
  KASSERT(curthread->t_addrspace == NULL);
  //curproc_setas(exev_as);


  if ((curthread->t_addrspace = as_create()) == NULL ) {
    kfree(kernpath);
    kfree(kernargs);
    vfs_close(v);
    return ENOMEM;
  }


  
  

as_activate(curthread->t_addrspace);

  result = load_elf(v, &entrypoint);
  if(result){
    kfree(kernargs);
    kfree(str);
    kfree(kernpath);
    vfs_close(v);
    return result;
  }
 
  vfs_close(v);

  result = as_define_stack(exev_as, &stackptr);
  if(result){
    kfree(kernargs);
    kfree(str);
    kfree(kernpath);
    return result;
  }

  int i = 0;
  while(kernargs[i] != NULL){
    char *cpargs;
    int aligned = str[i];
    if(aligned % 4 != 0){
      aligned = aligned + (4 - aligned % 4);
    }
    cpargs = kmalloc(sizeof(char) *aligned);
    for(int j = 0; j < aligned; j++){
      if(j < str[i]){
        cpargs[j] = kernargs[i][j];
      } else{
        cpargs[j] = '\0';
      }  
    }
    stackptr -= aligned;
    result = copyout((const void *) cpargs, (userptr_t) stackptr, (size_t) aligned);
    if(result){
      kfree(str);
      kfree(kernargs);
      kfree(cpargs);
      kfree(kernpath);
      return result;
    }
    kfree(cpargs);
    kernargs[i] = (char *)stackptr;

    i++;
  } //copy the string
  if(kernargs[i] == NULL){
    stackptr -= sizeof(char) * 4;
  } //space for last null 

  for(int index = 0; index < i; index++){
    stackptr -= sizeof(char *);
    result = copyout((const void *)kernargs[index], (userptr_t) stackptr, (size_t) (sizeof(char *)));
    if(result){
      kfree(kernargs);
      kfree(kernpath);
      kfree(str);
      return result;
    }
  }//copy the array

  kfree(kernargs);
  kfree(kernpath);
  kfree(str);

  enter_new_process(i, (userptr_t) stackptr, stackptr, entrypoint);

  panic("execv failed\n");
  return EINVAL;
*/
  DEBUG(DB_SYSCALL, "sys_execv: Entering execv syscall\n");

  unsigned long argc = 0;

  size_t programlen = strlen((char *)program) + 1; // + 1 for '\0'
  size_t totalargslen = 0;

  // Figure out how many arguments there are
  while (args[argc] != NULL) {
    totalargslen += strlen(((char **)args)[argc]) + 1; // + 1 for '\0'
    argc++;
  }

  if (programlen + totalargslen > ARG_MAX || argc > ARG_MAX / PATH_MAX) {
    // Args too big
    DEBUG(DB_SYSCALL, "sys_execv: Too many execv arguments ");
    DEBUG(DB_SYSCALL, "(%lu arguments with a total length of %d)\n", argc, totalargslen);
    *retval = E2BIG;
    return 1;
  }

  DEBUG(DB_SYSCALL, "sys_execv: Calling %s with %lu arguments\n", (char *)program, argc);

  char kprogram[programlen];
  char kargsraw[totalargslen]; // string containing all args
  char *kargs[argc];
  int copyresult;
  int copiedsofar = 0; // number of characters copied so far

  // Copy program name into `kprogram`
  copyresult = copyinstr((const_userptr_t)program, kprogram, programlen, NULL);
  if (copyresult) {
    // Copy error
    DEBUG(DB_SYSCALL, "sys_execv: Program name copy error\n");
    *retval = copyresult;
    return 1;
  }

  DEBUG(DB_SYSCALL, "sys_execv: Program name copied: %s\n", kprogram);

  // Copy user arguments into kernel address space
  for (unsigned long i = 0; i < argc; i++) {
    size_t got;
    size_t arglen = strlen(((char **)args)[i]) + 1; // + 1 for '\0'
    copyresult = copyinstr((const_userptr_t)args[i], (char *)(kargsraw + copiedsofar), arglen, &got);
    if (copyresult) {
      // Copy error
      DEBUG(DB_SYSCALL, "sys_execv: Program argument copy error");
      *retval = copyresult;
      return 1;
    }
    kargs[i] = kargsraw + copiedsofar;
    copiedsofar += got;
    DEBUG(DB_SYSCALL, "sys_execv: Program argument copied: %s\n", kargs[i]);
  }

  int err = runprogram(kprogram, kargs, argc);

  // Should not return, this implies an error
  *retval = err;
  return 1;
}
#endif



/*
int
sys_waitpid(pid_t pid,
            int * status,
            int options,){
  int exitstatus;
  int result;

  struct proc * waitproc = existingproc[pid];
  if(waitproc == NULL){
    return ESRCH;
  }
  if(waitproc == curproc){
    return ECHILD;
  }
  if(option != 0) return EINVAL;
  
  lock_acquire(waitproc->p_waitlk);
  while(!waitproc->p_ifexit){
    cv_wait(waitproc->waitcv, waitproc->p_waitlk);
  }
  lock_release(waitproc->p_waitlk);
  exitstatus = waitproc->p_ifexit;

}
*/


void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  //  (void)exitcode;
  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);
  
  KASSERT(curproc->p_addrspace != NULL);

  for (unsigned i = array_num(&p->p_children); i > 0 ; i--) {
    struct proc *pr = array_get(&p->p_children, i - 1);
    lock_release(pr->p_exit_lk);
    array_remove(&p->p_children, i - 1);
  }
  
  KASSERT(array_num(&p->p_children) == 0);

  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);
#if OPT_A2
  p->p_exitcode = _MKWAIT_EXIT( exitcode);
  p->p_exitcode = true;
  
  cv_broadcast(p->p_waitcv, p->p_waitlk);
  lock_acquire(p->p_exit_lk);
  lock_release(p->p_exit_lk);
#endif
  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */


  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");

}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
#if OPT_A2
  *retval = curproc->p_pid;
  return (0);
#else
  *retval = 1;
  return(0);
#endif
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;
  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */
#if OPT_A2
  struct proc * waitproc = find_proc_pid(pid);
  if(waitproc == NULL){
    return ESRCH;
  }
  if(waitproc == curproc){
    return ECHILD;
  }
#endif
  if(options != 0) return EINVAL;
#if OPT_A2
  lock_acquire(waitproc->p_waitlk);
  while(!waitproc->p_ifexit){
    cv_wait(waitproc->p_waitcv, waitproc->p_waitlk);
  }
  lock_release(waitproc->p_waitlk);
  exitstatus = waitproc->p_ifexit;
#else
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
#endif
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

