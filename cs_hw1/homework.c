-------- Context switching ----------

/*
 * file:        homework.c
 * description: Skeleton for homework 1
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, Jan. 2012
 * $Id: homework.c 500 2012-01-15 16:15:23Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uprog.h"

/***********************************/
/* Declarations for code in misc.c */
/***********************************/

typedef int *stack_ptr_t;
extern void init_memory(void);
extern void do_switch(stack_ptr_t *location_for_old_sp, stack_ptr_t new_value);
extern stack_ptr_t setup_stack(int *stack, void *func);
extern int get_term_input(char *buf, size_t len);
extern void init_terms(void);

extern void  *proc1;
extern void  *proc1_stack;
extern void  *proc2;
extern void  *proc2_stack;
extern void **vector;


/***********************************************/
/********* Your code starts here ***************/
/***********************************************/

/*
 * Function to read a file named filename at memory location 
 * pointed to by file_ptr. 
 */

void readfile(void *file_ptr, char *filename)
{    
     /*initialized the variable to zero*/
     int c = 0;
   
      /*
       * Below line opens the file, filename and reads it and is 
       * pointed by a file pointer "file_ptr"
       */     
     FILE *filehandle = fopen(filename, "r");

     /* making a temporary pointer point to the memory location file_ptr*/ 
     char *temp_ptr = file_ptr;
     
     /*
      * function getc() takes the pointer pointing to the memory
      * location of the file, filename and reads it character by
      * character till EOF and each character is pushed into the
      * memory location pointed by temp_ptr pointer.
      */
     
     while((c = getc(filehandle)) != EOF)
          *temp_ptr++ = c;
          
     /* closing the file read*/
     fclose(filehandle);     
}


/*
 * Question 1.
 *
 * The micro-program q1prog.c has already been written, and uses the
 * 'print' micro-system-call (index 0 in the vector table) to print
 * out "Hello world".
 *
 * You'll need to write the (very simple) print() function below, and
 * then put a pointer to it in vector[0].
 *
 * Then you read the micro-program 'q1prog' into memory starting at
 * address 'proc1', and execute it, printing "Hello world".
 *
 */

/* 
 * print() function simply displays the statement to which *line
 * pointer is pointing to
 */
void print(char *line)
{
    printf("%s",line);
}

void q1(void)
{    
     /* adding the entry of print function into the vector 
      * table at 0th location
      */
     *vector = print;
     
     /* reading the file q1prog into the memory location pointed by
      the proc1 pointer */
     readfile(proc1, "q1prog");

     /*
      * declaring a function pointer, pointing to the memory address
      * proc1 and then executing that function 
      */
     void (*func_ptr)();
     func_ptr = proc1;
     func_ptr();     
}


/*
 * Question 2.
 *
 * Add two more functions to the vector table:
 *   void readline(char *buf, int len) - read a line of input into 'buf'
 *   char *getarg(int i) - gets the i'th argument (see below)

 * Write a simple command line which prints a prompt and reads command
 * lines of the form 'cmd arg1 arg2 ...'. For each command line:
 *   - save arg1, arg2, ... in a location where they can be retrieved
 *     by 'getarg'
 *   - load and run the micro-program named by 'cmd'
 *   - if the command is "quit", then exit rather than running anything
 *
 * Note that this should be a general command line, allowing you to
 * execute arbitrary commands that you may not have written yet. You
 * are provided with a command that will work with this - 'q2prog',
 * which is a simple version of the 'grep' command.
 *
 * NOTE - your vector assignments have to mirror the ones in vector.s:
 *   0 = print
 *   1 = readline
 *   2 = getarg
 */

/* declaring a 128 bit buffer array initiaized to NULL initially*/
static char *args[128] = {NULL};

/* This function returns the 'i'th argument passed 
 * while executing the script 
 */
char *getarg(int i)  /* to be stored at vector index = 2 */
{
  /* Returning null if the requested argument number is greater than 127 */
  if (i > 127)   
  {
     return NULL;  
  }
  return args[i];
}

/*
 * This function reads a line of length 'len' from the standard 
 * input into the buffer located at address buf
 */
void readline(char *buf, int len) /* vector index = 1 */
{
  char c = '\0';
  
  /* a temporary pointer pointing the startign 
   * address given by buf
   */
  char *buf_ptr = buf;
  int chk_length = 0;

  /*
   * This loops reads the standard input character by character
   * untill either a newline character is received or 'len-1'
   * characters have been read and stored into location pointed
   * pointed by buf_ptr
   */
  while((c = getc(stdin)) != '\n' && chk_length < len-1){
    chk_length++;
    *buf_ptr++=c;
  }
  
  /* If the last character read is a newline character it stores 
   * it into the memory location pointed by buf_ptr
   */
  if(c == '\n'){
    *buf_ptr++ = c;
  }

  /* appends the null character to the end of the string */
  *buf_ptr = '\0';
}



void q2(void)
{
  /* initializing the vector table */
  vector[0] = print;
  vector[1] = readline ;
  vector[2] = getarg;

  while (1) {

    /* get a line from standard input*/
    printf(">");
    
    /* allocating memory buffer of size 128*/ 
    char *argbuf = malloc(128);
    
    /* reading the line from the standard input and storing
     * at address pointed by argbuf and of length 128
     */
    readline(argbuf, 128);


    /* 
     * Tokenizes argbuf into words (splits into words) with
     * delimiter as SPACE and newline character
     */
    char *cmd = strtok(argbuf, " \n");

    /* if no words found then continue */
    if (cmd == NULL)
      continue;
    
    /* if cmd entered is "quit", then break */
    if (!strcmp(cmd, "quit"))
      break;

    /* split it into tokens/words */
    int i = 0;
    for (i = 0; i < 128; i++) {
      char *word = strtok(NULL, " \n");
      if (word != NULL){
         /* store each word/token created into the args array*/
         args[i] = word;
      }
    }
         
      /*
       * Below line opens the file "cmd" and reads it and is 
       * pointed by a file pointer "file_ptr"
       */     
      FILE *file_ptr = fopen(cmd, "r");
      int temp_char = 0;
    
      /*assigning a temporary pointer to point to the proc1 memory location*/
      char *temp_ptr = proc1;
      
      /* if file_ptr is NULL the print appropriate message*/
      if(! file_ptr){
         printf("Not a valid command \n");

      }
      else{
          /* 
           * If command is found then read it character by character
           * and store it into location pointed by temp_ptr till EOF
	   */
          while((temp_char =getc(file_ptr)) != EOF){
	     *temp_ptr++ = temp_char;
          }
       
      /* closing the file*/
      fclose(file_ptr); 
           
     /*
      * declaring a function pointer, pointing to the memory address
      * proc1 and then executing that function 
      */
      void (*func_ptr)(void);
      func_ptr = proc1;
      func_ptr();

      }
        
      /* free the argument buffer*/
      free(argbuf);
    
      /* reseting the arguments in the args array to 0*/
      memset(&args[0], 0, sizeof(args));
  }

}
	
/*
 * Question 3.
 *
 * Create two processes which switch back and forth.
 *
 * You will need to add another 3 functions to the table:
 *   void yield12(void) - save process 1, switch to process 2
 *   void yield21(void) - save process 2, switch to process 1
 *   void uexit(void) - return to original homework.c stack
 *
 * The code for this question will load 2 micro-programs, q3prog1 and
 * q3prog2, which are provided and merely consists of interleaved
 * calls to yield12() or yield21() and print(), finishing with uexit().
 *
 * Hints:
 * - Use setup_stack() to set up the stack for each process. It returns
 *   a stack pointer value which you can switch to.
 * - you need a global variable for each process to store its context
 *   (i.e. stack pointer)
 * - To start you use do_switch() to switch to the stack pointer for 
 *   process 1
 */

/* stack pointer of process 1 initialized to 0 */
static stack_ptr_t sp_1 = 0;

/* stack pointer of process 2 initialized to 0 */
static stack_ptr_t sp_2 = 0;

/* temporary stack pointer 1 pointing to stack of homework.c */
static stack_ptr_t temp_ptr1 = 0;

/* temporary stack pointer 2 pointing to stack of homework.c*/
static stack_ptr_t temp_ptr2 = 0;

/* function to switch from process 1 to process 2 */
void yield12(void)		/* vector index = 3 */
{
    /* calling the function do_switch which saves the 
     * stack pointer of the current process 1 into sp_1 and
     * and points the stack to the stack pointer sp_2 
     */
    do_switch(&sp_1, sp_2);
}

/* function to switch from process 2 to process 1 */
void yield21(void)		/* vector index = 4 */
{
    /* calling the function do_switch to switch the stack
     * pointer to pointer sp_1 from sp_2
     */
    do_switch(&sp_2, sp_1);
}

void uexit(void)		/* vector index = 5 */
{
    /* calling the function do_switch to switch the stack
     * pointer to the temporary pointer of the process homework.c
     * temp_ptr2
     */

    do_switch(&temp_ptr1, temp_ptr2); 
}

void q3(void)
{
    /* initializing the vector table */
    vector[0] = print;
    vector[1] = readline;
    vector[2] = getarg;
    vector[3] = yield12;
    vector[4] = yield21;
    vector[5] = uexit;

    /* load q3prog1 into process 1 and q3prog2 into process 2 */

    /* read the file q3prog1 into the memory location proc1 */
    readfile(proc1, "q3prog1");

    /* setup the stack pointer sp_1 for process 1 */
    sp_1 = setup_stack(proc1_stack, proc1);

    /* read the file q3prog2 into the memory location proc2*/
    readfile(proc2, "q3prog2");

    /* setup the stack pointer sp_2 for process 2 */
    sp_2 = setup_stack(proc2_stack, proc2);
   
    /* calling do_switch to switch from the current process
     * homework.c to process 1
     */
    do_switch(&temp_ptr2, sp_1);    

}


/***********************************************/
/*********** Your code ends here ***************/
/***********************************************/
