/*
 * sigsetjmp/siglongjmp demo program.
 * Hebrew University OS course.
 * Author: OS, os@cs.huji.ac.il
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

//every time a Thread calls to 'switchThreads' the control is transferred to the second Thread


#define SECOND 1000000
#define STACK_SIZE 4096

//the stack for each thread
char stack1[STACK_SIZE];
char stack2[STACK_SIZE];

//we define 2 buffers
sigjmp_buf env[2];


#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

// the functions for translating the address we dont need to understand it we just need it to work :

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
		"rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

void switchThreads(void)
{
  static int currentThread = 0;

  // ret_val will be 0 at the first time switchThread is called
  // saves the values from thread 0 at the first time switchThread is called

  //at the second time the switchThread called
  int ret_val = sigsetjmp(env[currentThread],1);
  printf("SWITCH: ret_val=%d\n", ret_val); 
  if (ret_val == 1) {
      return;
  }
  currentThread = 1 - currentThread;
    //jumps to env[1] and then the pc points to g so the g
    // function will run now.
  siglongjmp(env[currentThread],1);

}

void f(void)
{
  int i = 0;
  while(1){ // an infinite loop, where each iteration takes one sec
    ++i;
    printf("in f (%d)\n",i);
    if (i % 3 == 0) {
      printf("f: switching\n");

      // every 3 sec, it calls the function switchThreads() that will switch to second thread
      // that will run the g() function
        switchThreads();
    }
    usleep(SECOND);
  }
}

void g(void)
{
  int i = 0;
  while(1){
    ++i;
    printf("in g (%d)\n",i);
    if (i % 5 == 0) {
      printf("g: switching\n");

      //every 5 sec, calling switchThreads() and goes to the second thread that
      // run the f() function
        switchThreads();
    }
    usleep(SECOND);
  }
}

void setup(void)
{
  address_t sp, pc;

  //we define the sp to point to the address of  : stack1 plus the size of the stack minus the
  // size of one address because the stack is working from bottom to top
  sp = (address_t)stack1 + STACK_SIZE - sizeof(address_t);
  //the pc points to the function f()
  pc = (address_t)f;

  //save all the reg to env[0], just so it wont be empty and have a reasonable values.
  sigsetjmp(env[0], 1);

  //overriding the reg in env[0] to the correct ones
  (env[0]->__jmpbuf)[JB_SP] = translate_address(sp);
  (env[0]->__jmpbuf)[JB_PC] = translate_address(pc);
  sigemptyset(&env[0]->__saved_mask);


  //like the creation of thread 1 to create the second one.
  sp = (address_t)stack2 + STACK_SIZE - sizeof(address_t);
  pc = (address_t)g;
  sigsetjmp(env[1], 1);
  (env[1]->__jmpbuf)[JB_SP] = translate_address(sp);
  (env[1]->__jmpbuf)[JB_PC] = translate_address(pc);
  sigemptyset(&env[1]->__saved_mask);         
}

int main(void)
{
  setup();		
  siglongjmp(env[0], 1); //this run thread number 0, that will simply run the function f()
  return 0;
}

