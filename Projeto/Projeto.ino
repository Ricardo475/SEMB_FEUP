/*
  SEMB / SETR - FEUP
  
  Example of a simple task manager (multi-tasking kernel)
  The tasks just switch on, wait a bit and switch off a different LED
    
  This version adds control of the activation period per task
  => each task is now triggered periodically by the kernel

  When  multiple tasks are ready, the one with highest priority executes

  Preemptive --> if a higher priority task becomes ready during 
                 the execution of another one, this one is preempted and
                 resumed after the termination of the hiher priority one

  The kernel time base is a periodic tick (set by a timer)
  All tasks periods are expressed as integer multiples of ticks

*/



/*
    Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define RIGHT 8 // Pin 2 connect to RIGHT button
#define LEFT 9  // Pin 3 connect to LEFT button
#define UP 10    // Pin 4 connect to UP button
#define DOWN 11  // Pin 5 connect to DOWN button
#define MAX_LENGTH 100  // Max snake's length, need less than 89 because arduino don't have more memory

Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);
// Adafruit_PCD8544 display = Adafruit_PCD8544(5, 4, 3);

// these are HW definitions 
// LEDs ports
#define d1 13  // built-in LED - positive logic

#define ON LOW   // built-in LED has positive logic
#define OFF HIGH

//diameter of the gameItemSize
unsigned int gameItemSize = 4;
volatile unsigned int snakeSize = 4;
volatile unsigned int snakeDir = 1;
volatile int SPEED = 1; // Input from button


struct gameItem {
  volatile unsigned int X; // x position
  volatile unsigned int Y;  //y position
};

//array to store all snake body part positions
gameItem snake[MAX_LENGTH];

//snake food item
gameItem snakeFood;




//*************** Multi-tasking kernel ******************

//*************** kernel variables ******************
// defines max number of tasks supported
#define MAXT 5

// kernel structure that defines the properties of each task
// --> the Task Control Block (TCB)
typedef struct {
  
  int period;
  /* ticks until next activation */
  int offset;
  /* function pointer */
  void (*func)(void);
  /* activation counter */
  int exec;
} Sched_Task_t;

// array of Task Control Blocks - TCBs
Sched_Task_t Tasks[MAXT];

// index of currently running task (MAXT to force searching all TCBs initially)
byte cur_task = MAXT;



// kernel initialization routine

int Sched_Init(void){
  /* - Initialise data
  * structures.
  */
  byte x;
  for(x=0; x<MAXT; x++)
    Tasks[x].func = 0;
    // note that "func" will be used to see if a TCB
    // is free (func=0) or used (func=pointer to task code)
  /* - Configure interrupt
  * that periodically
  * calls
  * Sched_Schedule().
  */
 
}

// adding a task to the kernel

int Sched_AddT( void (*f)(void), int d, int p){
    byte x;
    for(x=0; x<MAXT; x++)
      if (!Tasks[x].func) {
        // finds the first free TCB
        Tasks[x].period = p;
        Tasks[x].offset = d;  //first activation is "d" after kernel start
        Tasks[x].exec = 0;
        Tasks[x].func = f;
        return x;
      }
    return -1;  // if no free TCB --> return error
}

// Kernel scheduler, just activates periodic tasks
// "offset" is always counting down, activate task when 0
// then reset to "period"
// --> 1st activation at "offset" and then on every "period"

void Sched_Schedule(void){
  byte x;
  for(x=0; x<MAXT; x++) {
    if((Tasks[x].func)&&(Tasks[x].offset)){
      // for all existing tasks (func!=0) and not at 0, yet
      Tasks[x].offset--;  //decrement counter
      if(!Tasks[x].offset){
        /* offset = 0 --> Schedule Task --> set the "exec" flag/counter */
        // Tasks[x].exec++;  // accummulates activations if overrun UNSUITED TO REAL_TIME
        Tasks[x].exec=1;    // if overrun, following activation is lost
        Tasks[x].offset = Tasks[x].period;  // reset counter
      }
    }
  }
}

// Kernel dispatcher, takes highest priority ready task and runs it
// calls task directly within the stack scope of the the interrupted (preempted task)
//  --> preemptive -> one-shot model


void Sched_Dispatch(void){
  // save current task to resume it after preemption
  byte prev_task;
  byte x;
  prev_task = cur_task;  // save currently running task, for the case it is preempted
  for(x=0; x<cur_task; x++) {
  // x searches from 0 (highest priority) up to x (current task)
    if((Tasks[x].func)&&(Tasks[x].exec)) {
      // if a TCB has a task (func!=0) and there is a pending activation
      Tasks[x].exec--;  // decrement (reset) "exec" flag/counter

      cur_task = x;  // preempt current task with x (new high priority one)
      interrupts(); // enable interrupts so that this task can be preempted

      Tasks[x].func();  // Execute the task

      noInterrupts(); // disable interrupts again to continue the dispatcher cycle
      cur_task = prev_task;  // resume the task that was preempted (if any)

      // Delete task if one-shot, i.e., only runs once (period=0 && offset!0)
      if(!Tasks[x].period)
        Tasks[x].func = 0;
    }
  }
}




//*************** tasks code ******************
// This is the code of the tasks, normally they would something more useful !



void T1() {
  
  Serial.write('S');
  digitalWrite(d1,ON); 
  delay(random(200,500));  // random "execution time" between 200 and 500 ms
  digitalWrite(d1,OFF);    // despite being a "long" task, it is preempted to allow
  Serial.println('F');     // other tasks to execute (see their numbers between S and F
 }


void T2() {
  
  Serial.write('2');
  delay(70);
 }


void T3() {

  Serial.write('3');
  delay(30);
  }
  

void T4() {

  Serial.write('4');
  delay(80);
  }




/*****************  Arduino framework  ********************/

// the setup function runs once when you press reset or power the board
// used to configure hardware resources and software structures

void setup() {
   Serial.begin(9600);

  display.begin();
  // init done

  // you can change the contrast around to adapt the display
  // for the best viewing!
  display.setContrast(50);

  display.display(); // show splashscreen
 // falta o resto (MENU)
  
  // run the kernel initialization routine
  Sched_Init();

  // add all periodic tasks  (code, offset, period) in ticks
  // for the moment, ticks in 10ms -- see below timer frequency
  Sched_AddT(T4, 1, 10);   // highest priority
  Sched_AddT(T3, 1, 40);
  Sched_AddT(T2, 1, 80);
  Sched_AddT(T1, 1, 160);  // T1 with lowest priority, observe preemption
  
  noInterrupts(); // disable all interrupts

  // timer 1 control registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
 
  // register for the frequency of timer 1  
  OCR1A = 625; // compare match register 16MHz/256/100Hz -- tick = 10ms
  //OCR1A = 6250; // compare match register 16MHz/256/10Hz
  //OCR1A = 31250; // compare match register 16MHz/256/2Hz
  //OCR1A = 31;    // compare match register 16MHz/256/2kHz
  TCCR1B |= (1 << WGM12); // CTC mode
  TCCR1B |= (1 << CS12); // 256 prescaler
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt

  interrupts(); // enable all interrupts  
}


//timer1 interrupt service routine (ISR)
ISR(TIMER1_COMPA_vect){
  Sched_Schedule();  // invokes the scheduler to update tasks activations
  // invokes the dispatcher to execute the highest priority ready task
  Sched_Dispatch();  
}


// the loop function runs over and over again forever
// in this case, it does nothing, all tasks are executed by the kernel 
void loop() {
}
