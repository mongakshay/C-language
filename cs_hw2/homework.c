/* 
 * file:        homework.c
 * description: Skeleton code for CS 5600 Homework 2
 *
 * Peter Desnoyers, Northeastern CCIS, 2011
 * $Id: homework.c 530 2012-01-31 19:55:02Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include "hw2.h"

/********** YOUR CODE STARTS HERE ******************/

/*
 * Here's how you can initialize global mutex and cond variables
 */
 
/* mutex */
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/* condition variable that denotes the arrival of customer for starting the hair cut */
pthread_cond_t Barber = PTHREAD_COND_INITIALIZER;

/* condition variable that denotes that the barber is busying cutting a customer's hair*/
pthread_cond_t barber_busy = PTHREAD_COND_INITIALIZER;

/* condition variable that denotes that the hair cut is done */
pthread_cond_t haircut_done = PTHREAD_COND_INITIALIZER; 

/* condition variable that denotes that the customer has left after hair cut */
pthread_cond_t c_left = PTHREAD_COND_INITIALIZER; 

/* condition variable that denotes that the haircut has started */
pthread_cond_t haircut_start = PTHREAD_COND_INITIALIZER; 

/* variable that keeps track of the customers that have been turned away due to a full shop */
int cust_turned_away = 0; 

/* a counter to keep track of status of barber's chair */
void *bchair_stat = NULL;

/* a counter to keep track of number of customers in the shop */
void *customer_in_shop = NULL; 

/* a timer to keep track of customers time in shop */
void *time_in_shop = NULL;

/* Number of customer threads created */
int cust_threads = 10;

/* Number of chairs present in the Barber's shop */
int total_chairs = 5;

/* number of customers present in the shop */
int customers_in_shop = 0;

/* number of customers who had entered in the barber's shop */
int total_cust_entered = 0;


/* the barber method
 */
void barber(void)
{
    
    /* locking the mutex */
    pthread_mutex_lock(&m);

    while (1) {

        if( customers_in_shop == 0)
        {
           printf("DEBUG: %f barber goes to sleep \n", timestamp() );
           
           /* barber waiting for a customer to enter the shop */
           pthread_cond_wait(&Barber, &m);

           /* barber is signaled by the customer to wake up and it wakes up to cut hair */
           printf("DEBUG: %f barber wakes up \n", timestamp());
        }

        // signal to start haircut of customer
        pthread_cond_signal(&haircut_start);

        /* incerment the barber's chair status to denote that its occupied*/
        stat_count_incr(bchair_stat);
        
        /* time what barber takes to cut hair */
        sleep_exp(1.2, &m);
  
        /* barber signal the customer on completion of the hair cut */
        pthread_cond_signal(&haircut_done);

        /* barber waits inorder to check the status of number of customers in shop */
        pthread_cond_wait(&c_left, &m);
    }
  
    /* unlock the mutex before returning from the function */
    pthread_mutex_unlock(&m);
}

/* the customer method
 */
void customer(int customer_num)
{
  
    /* lock the mutex */
    pthread_mutex_lock(&m);
  
    /* incerment the counter to denote that customer is in the shop now */
    stat_count_incr(customer_in_shop);
  
    /* increment the count of the total number of customers as a customer enters the barber shop */
    total_cust_entered++;

  
    /* check to see if the customers in the shop is greater than the number of chairs available */
    if (customers_in_shop >= total_chairs)
    {

       /* increment the counter of turned away customers as the barber shop is already full */
       cust_turned_away++;
  
       /* decrement the counter to denote that customer has left the shop now */
       stat_count_decr(customer_in_shop);
    }
    else
    {
    
       printf("DEBUG: %f customer %d enters shop \n", timestamp(), customer_num);

       /* start the timer when customer enters the shop */
       stat_timer_start(time_in_shop);
      
       /* check if the current customer is the first one in the shop */ 
       if(customers_in_shop == 0)
         { 
           /* customer signals the barber to wake up so that it starts the hair cut */ 
           pthread_cond_signal(&Barber);
         }

       /* increment the count of the customers currently in the shop as a customer enters the barber shop */
       customers_in_shop++;

       /* customer waits for the haircut to start */
       pthread_cond_wait(&haircut_start, &m);

       printf("DEBUG: %f customer %d starts haircut \n", timestamp(), customer_num);
    
       /* the customer waits for his hair cut to be done */ 
       pthread_cond_wait(&haircut_done, &m);

       /* stop the timer when customer leaves the shop */ 
       stat_timer_stop(time_in_shop);
  
       /* decrement the barber's chair status to denote that its unoccupied*/
       stat_count_decr(bchair_stat);

       printf("DEBUG: %f customer %d leaves shop \n", timestamp(), customer_num);

       /* decrement the number of customers in the shop as the customer's hair cut is done */ 
       customers_in_shop--;

       /* signal the barber when customer has left the shop */
       pthread_cond_signal(&c_left);
       
       /* decrement the counter when the customer leaves the shop */
       stat_count_decr(customer_in_shop);
    }

    /* unlock the mutex before returning from the method */ 
    pthread_mutex_unlock(&m);
}

/* Threads which call these methods. Note that the pthread create
 * function allows you to pass a single void* pointer value to each
 * thread you create; we actually pass an integer (the customer number)
 * as that argument instead, using a "cast" to pretend it's a pointer.
 */

/* the customer thread function - create 10 threads, each of which calls
 * this function with its customer number 0..9
 */
void *customer_thread(void *context) 
{
    int customer_num = (int)context; 
    while(1){
       
       /* maintaing a 10 secs gap between creation of two customer threads */ 
       sleep_exp(10.0, NULL);
       
       /* calling the customer function with the customer number as the argument */ 
       customer(customer_num);
    }

    return 0;
}

/*  barber thread
 */
void *barber_thread(void *context)
{
    /* calling the barber method */ 
    barber(); /* never returns */
    
    return 0;
}

void q2(void)
{
    int i = 0;
    
    /* declaring the barber thread */ 
    pthread_t barber;
    
    /* creating the barber thread */ 
    pthread_create(&barber, NULL, barber_thread, (void*) -1);
    
    /* declaring the customer thread */ 
    pthread_t cust;

    for( i=0; i < cust_threads; i++)
    {
    
       /* creating the customer thread */ 
       pthread_create(&cust , NULL, customer_thread, (void*) i);
    
    }

    wait_until_done();
    
}

/* For question 3 you need to measure the following statistics:
 *
 * 1. fraction of  customer visits result in turning away due to a full shop 
 *    (calculate this one yourself - count total customers, those turned away)
 * 2. average time spent in the shop (including haircut) by a customer 
 *     *** who does not find a full shop ***. (timer)
 * 3. average number of customers in the shop (counter)
 * 4. fraction of time someone is sitting in the barber's chair (counter)
 *
 * The stat_* functions (counter, timer) are described in the PDF. 
 */

void q3(void)
{
  
  int i = 0;
    
  /* initialize the timer to keep track of customers time in shop */
  time_in_shop = stat_timer();
  
  /* initialize the counter to keep track of number of customers in the shop */
  customer_in_shop = stat_counter();
  
  /* initialize the counter to keep track of status of barber's chair*/
  bchair_stat = stat_counter();

  /* declaring the barber thread */   
  pthread_t barber;
 
  /* creating the barber thread */ 
  pthread_create(&barber, NULL, barber_thread, (void*) -1);
 
  /* declaring the customer thread */ 
  pthread_t cust;

  for( i=0; i < cust_threads; i++)
  {
      /* creating the customer thread */ 
      pthread_create(&cust , NULL, customer_thread, (void*) i);
  }

  wait_until_done();
  
  /* average number of customers turned away */
  float avg_cust = (float) cust_turned_away/total_cust_entered;
  
  /* Average number of customers in the shop */
  double avg_customer_in_shop = stat_count_mean(customer_in_shop);
  
  /* Average time spent in the shop by a customer who does not find a full shop */
  double avg_cust_time_shop = stat_timer_mean(time_in_shop);
  
  /* Fraction of time someone is siting on the barber's chair */
  double avg_bchair_stat = stat_count_mean(bchair_stat);

  printf("Fraction of customer visits result in turning away due to a full shop = %f \n", avg_cust);
  printf("Average time spent in the shop by a customer who does not find a full shop = %f \n", avg_cust_time_shop);
  printf("Average number of customers in the shop = %f \n", avg_customer_in_shop);
  printf("Fraction of time someone is siting on the barber's chair  = %f \n", avg_bchair_stat);
}
