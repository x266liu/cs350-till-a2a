#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
static struct cv *intersectioncv;
static struct lock *intersectionlk;
static Direction * in;
static Direction * out;
static int counter;
//static struct semaphore *intersectionSem;


/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

  /* intersectionSem = sem_create("intersectionSem",1);
  if (intersectionSem == NULL) {
    panic("could not create intersection semaphore");
    }*/
  intersectionlk = lock_create("intersectionlk");
  intersectioncv = cv_create("instersectioncv");
  in = kmalloc(sizeof(Direction[10]));
  out = kmalloc(sizeof(Direction[10]));//max thread is 10. So make an array of size 10
  counter = 0;
  if (intersectionlk == NULL) {
    panic("could not create intersection lock");
  }
  if (intersectioncv == NULL) {
    panic("could not create intersection cv");
    }
  return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  KASSERT(intersectioncv != NULL);
  KASSERT(intersectionlk != NULL);
  // KASSERT(intersectionSem != NULL);
  //sem_destroy(intersectionSem);
  cv_destroy(intersectioncv);
  lock_destroy(intersectionlk);
  kfree(in);
  kfree(out);
}


static bool 
right_turn(Direction i, Direction o)
{
  if(((i == west) && (o == south)) ||
     ((i == south) && (o == east)) ||
     ((i == east) && (o == north)) ||
     ((i == north) && (o == west))) {
    return true;
  } else{
    return false;
  }
}


static bool 
intersection_is_full(Direction origin, Direction destination)
{
  for(int i = 0; i < counter; i++){
    if ((out[i] == origin && in[i] == destination) ||
	(in[i] == origin ) ||
	((right_turn(origin, destination)) && (right_turn(in[i], out[i]))
	 && destination != out[i])){
      continue;
    } else{
      return true;
    }
  }
  return false;
}



/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  //(void)origin;  /* avoid compiler complaint about unused parameter */
  //(void)destination; /* avoid compiler complaint about unused parameter */
  KASSERT(intersectioncv != NULL);
  KASSERT(intersectionlk != NULL);
  KASSERT(counter < 10);
  KASSERT(counter >= 0);
  // P(intersectionSem);
  lock_acquire(intersectionlk);
  while(intersection_is_full(origin, destination) == true) {
    cv_wait(intersectioncv, intersectionlk);
  }
  in[counter] = origin;
  out[counter] =destination;
  counter++;
  lock_release(intersectionlk);
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 *  return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  //(void)origin;  /* avoid compiler complaint about unused parameter */
  //(void)destination; /* avoid compiler complaint about unused parameter */
  KASSERT(intersectioncv != NULL);
  KASSERT(intersectionlk != NULL);
  KASSERT(counter < 10);
  KASSERT(counter >= 0);
  // V(intersectionSem);
  lock_acquire(intersectionlk);
  int pos = 0;
  for(int i = 0; i < counter; i++){
    if (in[i] == origin && out[i] == destination){
      pos = i;
      break;
    }
  }
  for(int i = pos; i < counter; i++){
    in[i] = in[i + 1];
    out[i] = out[i + 1];
  }
  counter--;
  cv_broadcast(intersectioncv, intersectionlk);
  lock_release(intersectionlk);
}
