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
#include <EEPROM.h>


#define RIGHT 7 // Pin 7 connect to RIGHT button
#define LEFT 6  // Pin 6 connect to LEFT button
#define UP 9    // Pin 9 connect to UP button
#define DOWN 8  // Pin 8 connect to DOWN button
#define BEEP 10
#define MAX_LENGTH 100  // Max snake's length, need less than 89 because arduino don't have more memory
#define LEVEL_MAX 3

//Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);
  Adafruit_PCD8544 display = Adafruit_PCD8544(5, 4, 3);

//diameter of the gameItemSize
unsigned int gameItemSize = 2,level=1;
volatile unsigned int snakeSize = 4;
volatile unsigned int snakeDir = 1;
volatile unsigned int speed=1; // Input from button


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

int Sched_Init(void) {
  /* - Initialise data
    structures.
  */
  byte x;
  for (x = 0; x < MAXT; x++)
    Tasks[x].func = 0;
  // note that "func" will be used to see if a TCB
  // is free (func=0) or used (func=pointer to task code)
  /* - Configure interrupt
    that periodically
    calls
    Sched_Schedule().
  */

}

// adding a task to the kernel

int Sched_AddT( void (*f)(void), int d, int p) {
  byte x;
  for (x = 0; x < MAXT; x++)
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

void Sched_Schedule(void) {
  byte x;
  for (x = 0; x < MAXT; x++) {
    if ((Tasks[x].func) && (Tasks[x].offset)) {
      // for all existing tasks (func!=0) and not at 0, yet
      Tasks[x].offset--;  //decrement counter
      if (!Tasks[x].offset) {
        /* offset = 0 --> Schedule Task --> set the "exec" flag/counter */
        // Tasks[x].exec++;  // accummulates activations if overrun UNSUITED TO REAL_TIME
        Tasks[x].exec = 1;  // if overrun, following activation is lost
        Tasks[x].offset = Tasks[x].period;  // reset counter
      }
    }
  }
}

// Kernel dispatcher, takes highest priority ready task and runs it
// calls task directly within the stack scope of the the interrupted (preempted task)
//  --> preemptive -> one-shot model


void Sched_Dispatch(void) {
  // save current task to resume it after preemption
  byte prev_task;
  byte x;
  prev_task = cur_task;  // save currently running task, for the case it is preempted
  for (x = 0; x < cur_task; x++) {
    // x searches from 0 (highest priority) up to x (current task)
    if ((Tasks[x].func) && (Tasks[x].exec)) {
      // if a TCB has a task (func!=0) and there is a pending activation
      Tasks[x].exec--;  // decrement (reset) "exec" flag/counter

      cur_task = x;  // preempt current task with x (new high priority one)
      interrupts(); // enable interrupts so that this task can be preempted

      Tasks[x].func();  // Execute the task

      noInterrupts(); // disable interrupts again to continue the dispatcher cycle
      cur_task = prev_task;  // resume the task that was preempted (if any)

      // Delete task if one-shot, i.e., only runs once (period=0 && offset!0)
      if (!Tasks[x].period)
        Tasks[x].func = 0;
    }
  }
}
void drawGameOver() {
  int points; // put '1' instead of '0' , it will ignore :v

  points= snakeSize-5;
 
  while(1)
  {
      
      display.clearDisplay();
      display.setTextColor(BLACK);       
      display.setTextSize(1);
      display.setCursor(display.width()/2-display.width()/4-5,0);
      display.print("GAME OVER");
      display.setCursor(display.width()/2-display.width()/2,display.height()/2);
      display.print("SCORE: ");
      display.print(points);
      display.setCursor(display.width()/2-display.width()/2,display.height()/2+display.height()/4);
      display.print("HIGH SCORE: ");
      if(level==1)
      {
        if(speed==0)
        {
          if(points > EEPROM.read(0))
            EEPROM.write(0,points);
          display.print(EEPROM.read(0));
          Serial.println("OLA "+EEPROM.read(0));
        }
        else if(speed==1)
        {
          if(points > EEPROM.read(1))
            EEPROM.write(1,points);
          display.print(EEPROM.read(1));
        }
         else if(speed==2)
        {
          if(points > EEPROM.read(2))
            EEPROM.write(2,points);
          display.print(EEPROM.read(2));
        }
      }
      else if(level==2)
      {
        if(speed==0)
        {
          if(points > EEPROM.read(3))
            EEPROM.write(3,points);
          display.print(EEPROM.read(3));
        }
        else if(speed==1)
        {
          if(points > EEPROM.read(4))
            EEPROM.write(4,points);
          display.print(EEPROM.read(4));
        }
         else if(speed==2)
        {
          if(points > EEPROM.read(5))
            EEPROM.write(5,points);
          display.print(EEPROM.read(5));
        }
      }
      else if(level==3)
      {
        if(speed==0)
        {
          if(points > EEPROM.read(6))
            EEPROM.write(6,points);
          display.print(EEPROM.read(6));
        }
        else if(speed==1)
        {
          if(points > EEPROM.read(7))
            EEPROM.write(7,points);
          display.print(EEPROM.read(7));
        }
         else if(speed==2)
        {
          if(points > EEPROM.read(8))
            EEPROM.write(8,points);
          display.print(EEPROM.read(8));
        }
      }
      display.display();
  }
}
void drawFood() {
    
    display.fillRect(snakeFood.X,snakeFood.Y,2,2,BLACK);          
              
  }

void spawnSnakeFood() {
  //generate snake Food position and avoid generate on position of snake
  unsigned int i = 1;
  unsigned int f = 1;

  randomSeed(analogRead(A0));
  
    while (snakeFood.X %4 != 0 || snakeFood.Y % 4 != 0 || f==1)
    {             
        snakeFood.X = random(2, display.width()-2);
        snakeFood.Y = random(2, display.height()-2);
        f=0;
       
        for(int i=0; i<=snakeSize;i++)
        {
          if(snakeFood.X == snake[i].X && snakeFood.Y == snake[i].Y)
          {
            f=1;
          }
        }
    
    } 
  Serial.print(snakeFood.X );
  Serial.write("  ");
  Serial.print(snakeFood.Y );
}
void spawnSnakeFood3() {
  //generate snake Food position and avoid generate on position of snake
  unsigned int i = 1;
  unsigned int f = 1;

  randomSeed(analogRead(A0));
  
    while (snakeFood.X %4 != 0 || snakeFood.Y % 4 != 0 || f==1)
    {             
        snakeFood.X = random(2, display.width()-2);
        snakeFood.Y = random(2, display.height()-2);
        f=0;
       
        for(int i=0; i<=snakeSize;i++)
        {
          if((snakeFood.X == snake[i].X && snakeFood.Y == snake[i].Y)|| (snake[0].Y > display.height()-2) || (snake[0].X >=18 && snake[0].X<66 && snake[0].Y==12) ||(snake[0].X >=18 && snake[0].X<66 && snake[0].Y==36) || (snake[0].Y >=14 && snake[0].Y<36 && snake[0].X==12 )|| (snake[0].Y >=14 && snake[0].Y<36 && snake[0].X==72 ))
          {
            f=1;
          }
        }
    
    } 
  Serial.print(snakeFood.X );
  Serial.write("  ");
  Serial.print(snakeFood.Y );
}
void drawSnake() {
  for (unsigned int i = 0; i < snakeSize; i++) {
    display.fillRect(snake[i].X,snake[i].Y,2,2,BLACK);  
    
 }
}
void updateValues() {
  //update all body parts of the snake excpet the head
  unsigned int i;
  for (i = snakeSize - 1; i > 0; i--)
    snake[i] = snake[i - 1];

  //Now update the head
  //move left
  if (snakeDir == 0 )
    snake[0].X -= gameItemSize;
    
  //move right
  else if (snakeDir == 1)
    snake[0].X += gameItemSize;

  //move down
  else if (snakeDir == 2)
    snake[0].Y += gameItemSize;

  //move up
  else if (snakeDir == 3) 
    snake[0].Y -= gameItemSize;
    
 if((snake[0].X==snake[2].X && snake[0].Y==snake[2].Y)|| (snake[0].X==snake[1].X && snake[0].Y==snake[1].Y))
  {
    Serial.println("ERRROOO");
    //Now update the head
    //move left
   if (snakeDir == 0 )
    snake[0].X += gameItemSize*2;
    //move right
    else if (snakeDir == 1)
    snake[0].X -= gameItemSize*2;

    //move down
     else if (snakeDir == 2)
    snake[0].Y -= gameItemSize*2;

    //move up
    else if (snakeDir == 3) 
    snake[0].Y += gameItemSize*2;
  }
   
}

//VER
void handleColisions3() {
  //check if snake eats food
  if (snake[0].X == snakeFood.X && snake[0].Y == snakeFood.Y) {
    //increase snakeSize
   //Serial.write("sup");
    snakeSize++;
    //regen food
    //spawnSnakeFood();
    Sched_AddT(spawnSnakeFood3, 1, 0);
     Sched_AddT(beepComida, 10, 0);
  }

  //check if snake collides with itself
  else {
    for (unsigned int i = 1; i < snakeSize; i++) {
      if (snake[0].X == snake[i].X && snake[0].Y == snake[i].Y) {
        drawGameOver();
      }
    }
  }
  //check for wall collisions
  if ((snake[0].X < 2) || (snake[0].Y < 2) || (snake[0].X > display.width()-2) || (snake[0].Y > display.height()-2) || (snake[0].X >=20 && snake[0].X<64 && snake[0].Y==12) ||(snake[0].X >=20 && snake[0].X<64 && snake[0].Y==36) || (snake[0].Y >=16 && snake[0].Y<34 && snake[0].X==12 )|| (snake[0].Y >=16 && snake[0].Y<34 && snake[0].X==72 )) {
    drawGameOver();
  }
  
}

void handleColisions2() {
  //check if snake eats food
  if (snake[0].X == snakeFood.X && snake[0].Y == snakeFood.Y) {
    //increase snakeSize
   //Serial.write("sup");
    snakeSize++;
    //regen food
    //spawnSnakeFood();
    Sched_AddT(spawnSnakeFood, 1, 0);
     Sched_AddT(beepComida, 10, 0);
  }

  //check if snake collides with itself
  else {
    for (unsigned int i = 1; i < snakeSize; i++) {
      if (snake[0].X == snake[i].X && snake[0].Y == snake[i].Y) {
        drawGameOver();
      }
    }
  }
  //check for wall collisions
  if ((snake[0].X < 2) || (snake[0].Y < 2) || (snake[0].X > display.width()-2) || (snake[0].Y > display.height()-2)) {
    drawGameOver();
  }
}
void handleColisions() {
  //check if snake eats food
  if (snake[0].X == snakeFood.X && snake[0].Y == snakeFood.Y) {
    //increase snakeSize
   //Serial.write("sup");
    snakeSize++;
    //regen food
    //spawnSnakeFood();
    Sched_AddT(spawnSnakeFood, 1, 0);
     Sched_AddT(beepComida, 10, 0);
  }

  //check if snake collides with itself
  else {
    for (unsigned int i = 1; i < snakeSize; i++) {
      if (snake[0].X == snake[i].X && snake[0].Y == snake[i].Y) {
        drawGameOver();
      }
    }
  }
  //check for wall collisions
  if ((snake[0].X < 2)) {
      snake[0].X=display.width()-2;
  }
  else if((snake[0].Y < 2))
  {
    snake[0].Y=display.height()-2;
  }
  else if((snake[0].X > display.width()-2))
  {
    snake[0].X=2;
  }
  else if((snake[0].Y > display.height()-2))
  {
     snake[0].Y=2;
  }
}
//*************** tasks code ******************
// This is the code of the tasks, normally they would something more useful !

void playGame() {
  handleColisions();
  updateValues(); 
}
void playGame2() {
  handleColisions2();
  updateValues(); 
}
void playGame3() {
  handleColisions3();
  updateValues(); 
}
void draw3(){
  display.clearDisplay();
  display.drawRect(0, 0, display.width(), display.height(), BLACK); // area limite
  display.drawRect(20, 12, 44, 2, BLACK); // linha cima
  display.drawRect(12, 16, 2, 18, BLACK); // linha esquerda
  display.drawRect(72, 16, 2, 18, BLACK); // linha direita
  display.drawRect(20, 36,44,2, BLACK); // linha de baixo
   drawSnake();
   drawFood();
   display.display();
}
void draw2(){
  display.clearDisplay();
  display.drawRect(0, 0, display.width(), display.height(), BLACK);
   drawSnake();
   drawFood();
   display.display();
}
void draw(){
  unsigned long a=micros();
  display.clearDisplay();
    drawSnake();
    drawFood();
      display.display(); 
  //Serial.println(micros()-a);
 
}
void get_key() {
  
  if (digitalRead(LEFT) == 0 && snakeDir != 1)
  {
    Serial.write("LEFT\n");
    snakeDir = 0; // left
  }
  else if (digitalRead(RIGHT) == 0 && snakeDir != 0)
  {
    Serial.write("RIGTH\n");
    snakeDir = 1; // right
  }
  else if (digitalRead(DOWN) == 0 && snakeDir != 3)
  {
    Serial.write("DOWN\n");
    snakeDir = 2; // down
  }
  else if (digitalRead(UP) == 0 && snakeDir != 2)
  {
    Serial.write("UP\n");
    snakeDir = 3; // up
  }
}
void beepComida()
{
   digitalWrite(BEEP,HIGH);
   delay(100);
   digitalWrite(BEEP,LOW);
}

/*****************  Arduino framework  ********************/

// the setup function runs once when you press reset or power the board
// used to configure hardware resources and software structures

void setup() {
  Serial.begin(9600);
  snake[0].X=6;
  snake[0].Y=6;
  snakeFood.X=6;
  snakeFood.Y=6;
  //Inicializar butões
    pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(UP, INPUT_PULLUP);
  pinMode(BEEP,OUTPUT);

  display.begin();
  // init done

  // you can change the contrast around to adapt the display
  // for the best viewing!
  display.setContrast(100);
  display.display();// show splashscreen
  display.clearDisplay();
  // falta o resto (MENU)
    display.setTextSize(1);      
  display.setTextColor(BLACK);
  display.setCursor(display.width()/2-display.width()/2,0);
  display.print("SNAKUINO MENU");
  display.fillTriangle(display.width()/2+2,display.height()/2-5,display.width()/2+7,display.height()/2-10,display.width()/2+12,display.height()/2-5,BLACK);
  display.setCursor(display.width()/2-display.width()/2,display.height()/2);
  display.print("LEVEL:");
  display.setCursor(display.width()/2+5,display.height()/2);
  display.print(level);
  display.fillTriangle(display.width()/2+2,display.height()/2+10,display.width()/2+7,display.height()/2+15,display.width()/2+12,display.height()/2+10,BLACK);
  display.display();
  while(digitalRead(RIGHT) != 0){

       if(digitalRead(UP) == 0)
       {
        display.clearDisplay();
        display.setTextSize(1);      
        display.setTextColor(BLACK);
        display.setCursor(display.width()/2-display.width()/2,0);
        display.print("SNAKUINO MENU");
        display.setCursor(display.width()/2-display.width()/2,display.height()/2);
        display.print("LEVEL:");
        display.setCursor(display.width()/2+5,display.height()/2);
       
        display.fillTriangle(display.width()/2+2,display.height()/2+10,display.width()/2+7,display.height()/2+15,display.width()/2+12,display.height()/2+10,BLACK);
        display.print(level);
        display.display();
        
        delay(200);
        if(level<LEVEL_MAX)
          level= level+1;
        display.clearDisplay();
        display.setTextSize(1);      
        display.setTextColor(BLACK);
        display.setCursor(display.width()/2-display.width()/2,0);
        display.print("SNAKUINO MENU");
        display.setCursor(display.width()/2-display.width()/2,display.height()/2);
        display.print("LEVEL:");
        display.setCursor(display.width()/2+5,display.height()/2);
        display.fillTriangle(display.width()/2+2,display.height()/2+10,display.width()/2+7,display.height()/2+15,display.width()/2+12,display.height()/2+10,BLACK);
        display.print(level);       
        display.fillTriangle(display.width()/2+2,display.height()/2-5,display.width()/2+7,display.height()/2-10,display.width()/2+12,display.height()/2-5,BLACK);
         display.display();
       
       }
       else if(digitalRead(DOWN) == 0)
       {
        
        display.clearDisplay();
        display.setTextSize(1);      
        display.setTextColor(BLACK);
        display.setCursor(display.width()/2-display.width()/2,0);
        display.print("SNAKUINO MENU");
        display.setCursor(display.width()/2-display.width()/2,display.height()/2);
        display.print("LEVEL:");
        display.setCursor(display.width()/2+5,display.height()/2);
        display.fillTriangle(display.width()/2+2,display.height()/2-5,display.width()/2+7,display.height()/2-10,display.width()/2+12,display.height()/2-5,BLACK);       
        display.print(level);
        display.display();
        
        delay(200);
        if(level>1)
         level= level-1;
        display.clearDisplay();
        display.setTextSize(1);      
        display.setTextColor(BLACK);
        display.setCursor(display.width()/2-display.width()/2,0);
        display.print("SNAKUINO MENU");
        display.setCursor(display.width()/2-display.width()/2,display.height()/2);
        display.print("LEVEL:");
        display.setCursor(display.width()/2+5,display.height()/2);
        display.fillTriangle(display.width()/2+2,display.height()/2+10,display.width()/2+7,display.height()/2+15,display.width()/2+12,display.height()/2+10,BLACK);
        display.fillTriangle(display.width()/2+2,display.height()/2-5,display.width()/2+7,display.height()/2-10,display.width()/2+12,display.height()/2-5,BLACK);       
        display.print(level);     
        display.display();
        
       }
    
    }
    delay(1000);
  display.clearDisplay();
  // falta o resto (MENU)
  display.setTextSize(1);      
  display.setTextColor(BLACK);
  display.setCursor(display.width()/2-display.width()/2,0);
  display.print("SNAKUINO MENU");
  display.fillTriangle(display.width()/2+2,display.height()/2-5,display.width()/2+7,display.height()/2-10,display.width()/2+12,display.height()/2-5,BLACK);
  display.setCursor(display.width()/2-display.width()/2,display.height()/2);
  display.print("Speed:");
  display.setCursor(display.width()/2-5,display.height()/2);
  display.print("NORMAL");
  display.fillTriangle(display.width()/2+2,display.height()/2+10,display.width()/2+7,display.height()/2+15,display.width()/2+12,display.height()/2+10,BLACK);
  display.display();
  
  while(digitalRead(RIGHT) != 0){
      if(digitalRead(UP) == 0)
       {
        display.clearDisplay();
        display.setTextSize(1);      
        display.setTextColor(BLACK);
        display.setCursor(display.width()/2-display.width()/2,0);
        display.print("SNAKUINO MENU");
        display.setCursor(display.width()/2-display.width()/2,display.height()/2);
        display.print("Speed:");
        display.setCursor(display.width()/2-5,display.height()/2);
       
        display.fillTriangle(display.width()/2+2,display.height()/2+10,display.width()/2+7,display.height()/2+15,display.width()/2+12,display.height()/2+10,BLACK);
        if(speed==1)
          display.print("NORMAL");
        else if(speed==2)
          display.print("FAST");
        else if(speed==0)
          display.print("SLOW");
        display.display();
        
        delay(200);
        if(speed<2)
         speed = speed+1;
        display.clearDisplay();
        display.setTextSize(1);      
        display.setTextColor(BLACK);
        display.setCursor(display.width()/2-display.width()/2,0);
        display.print("SNAKUINO MENU");
        display.setCursor(display.width()/2-display.width()/2,display.height()/2);
        display.print("Speed:");
        display.setCursor(display.width()/2-5,display.height()/2);
        display.fillTriangle(display.width()/2+2,display.height()/2+10,display.width()/2+7,display.height()/2+15,display.width()/2+12,display.height()/2+10,BLACK);
        if(speed==1)
          display.print("NORMAL");
        else if(speed==2)
          display.print("FAST");
        else if(speed==0)
          display.print("SLOW");      
        display.fillTriangle(display.width()/2+2,display.height()/2-5,display.width()/2+7,display.height()/2-10,display.width()/2+12,display.height()/2-5,BLACK);
         display.display();
       
       }
       else if(digitalRead(DOWN) == 0)
       {
        
        display.clearDisplay();
        display.setTextSize(1);      
        display.setTextColor(BLACK);
        display.setCursor(display.width()/2-display.width()/2,0);
        display.print("SNAKUINO MENU");
        display.setCursor(display.width()/2-display.width()/2,display.height()/2);
        display.print("Speed:");
        display.setCursor(display.width()/2-5,display.height()/2);
        display.fillTriangle(display.width()/2+2,display.height()/2-5,display.width()/2+7,display.height()/2-10,display.width()/2+12,display.height()/2-5,BLACK);       
        if(speed==1)
          display.print("NORMAL");
        else if(speed==2)
          display.print("FAST");
        else if(speed==0)
          display.print("SLOW");
        display.display();
        
        delay(200);
        if(speed>0)
         speed = speed-1;
        display.clearDisplay();
        display.setTextSize(1);      
        display.setTextColor(BLACK);
        display.setCursor(display.width()/2-display.width()/2,0);
        display.print("SNAKUINO MENU");
        display.setCursor(display.width()/2-display.width()/2,display.height()/2);
        display.print("Speed:");
        display.setCursor(display.width()/2-5,display.height()/2);
        display.fillTriangle(display.width()/2+2,display.height()/2+10,display.width()/2+7,display.height()/2+15,display.width()/2+12,display.height()/2+10,BLACK);
        display.fillTriangle(display.width()/2+2,display.height()/2-5,display.width()/2+7,display.height()/2-10,display.width()/2+12,display.height()/2-5,BLACK);       
       if(speed==1)
          display.print("NORMAL");
        else if(speed==2)
          display.print("FAST");
        else if(speed==0)
          display.print("SLOW");     
        display.display();
        
       }
  }
  delay(1000);
  // run the kernel initialization routine
  Sched_Init();
  
  // add all periodic tasks  (code, offset, period) in ticks
  // for the moment, ticks in 10ms -- see below timer frequency
  // ticks a cada 2ms
  if(level==1)
  {
    if(speed==1)
   {  
    Sched_AddT(get_key, 1, 20);   // highest priority
    Sched_AddT( playGame, 1, 100);
    Sched_AddT(draw, 1,120);
   }
  else if(speed==2)
  {
    Sched_AddT(get_key, 1, 20);   // highest priority
    Sched_AddT( playGame, 1, 50);
    Sched_AddT(draw, 1,70);
  }
  else if(speed==0)
  {
     Sched_AddT(get_key, 1, 20);   // highest priority
    Sched_AddT( playGame, 1, 150);
    Sched_AddT(draw, 1,170);
  }
  
  }
  else if(level ==2)
  {
    if(speed==1)
   {  
     Sched_AddT(get_key, 1, 20);   // highest priority
      Sched_AddT( playGame2, 1, 100);
      Sched_AddT(draw2, 1,100);
   }
    else if(speed==2)
  {
     Sched_AddT(get_key, 1, 20);   // highest priority
      Sched_AddT( playGame2, 1, 50);
      Sched_AddT(draw2, 1,50);
  }
    else if(speed==0)
  {
      Sched_AddT(get_key, 1, 20);   // highest priority
      Sched_AddT( playGame2, 1, 150);
      Sched_AddT(draw2, 1,150);
  }
     
  }
  else if(level == 3)
  {
      if(speed==1)
   {  
    Sched_AddT(get_key, 1, 20);   // highest priority
      Sched_AddT( playGame3, 1, 100);
      Sched_AddT(draw3, 1,100);
   }
    else if(speed==2)
  {
     Sched_AddT(get_key, 1, 20);   // highest priority
      Sched_AddT( playGame3, 1, 50);
      Sched_AddT(draw3, 1,50);
  }
    else if(speed==0)
  {
      Sched_AddT(get_key, 1, 20);   // highest priority
      Sched_AddT( playGame3, 1, 150);
      Sched_AddT(draw3, 1,150);
  }
  }
  noInterrupts(); // disable all interrupts

  // timer 1 control registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // register for the frequency of timer 1
  //OCR1A = 625; // compare match register 16MHz/256/100Hz -- tick = 10ms
  
     OCR1A=  125;  // 500 HZ
  
   
  //OCR1A = 6250; // compare match register 16MHz/256/10Hz
  //OCR1A = 31250; // compare match register 16MHz/256/2Hz
 //OCR1A = 31;    // compare match register 16MHz/256/2kHz
  TCCR1B |= (1 << WGM12); // CTC mode
  TCCR1B |= (1 << CS12); // 256 prescaler
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt

  interrupts(); // enable all interrupts
}


//timer1 interrupt service routine (ISR)
ISR(TIMER1_COMPA_vect) {
  Sched_Schedule();  // invokes the scheduler to update tasks activations
  // invokes the dispatcher to execute the highest priority ready task
  Sched_Dispatch();
}


// the loop function runs over and over again forever
// in this case, it does nothing, all tasks are executed by the kernel
void loop() {
}
