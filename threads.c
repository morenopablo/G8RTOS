// G8RTOS_Threads.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for thread functions.

/************************************Includes***************************************/

#include "./threads.h"

#include "./MultimodDrivers/multimod.h"
#include "./MiscFunctions/Signals/inc/goertzel.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "driverlib/timer.h"
#include "driverlib/adc.h"

#define MAX_NUM_SAMPLES      (200)
#define SIGNAL_STEPS         (2)


/*********************************Global Variables**********************************/
uint16_t dac_step = 0;
int16_t dac_signal[SIGNAL_STEPS] = {0x001,0x000};
int16_t current_volume = 0xFFF;
int16_t malletX = 120;
int16_t malletY = 160;
int16_t score = 0;
int16_t gameTime = 1000;
int16_t highScores[5];

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t isVisible;
} Mole;

Mole moles[8];
//srand(time(NULL));



/*********************************Global Variables**********************************/

/********************************Public Functions***********************************/

int16_t Goertzel_ReadSample (int FIFO_index) {
    
    // read sample from FIFO
    uint32_t val = G8RTOS_ReadFIFO(FIFO_index);

    // return sample value
    return (uint16_t)(val & 0xFFFF);
}

/*************************************Threads***************************************/

void Idle_Thread(void) {

    while(1);
}



void Speaker_Thread(void) {

    uint8_t buttons = 0;
    
    while (1)
    {
        // wait for button semaphore
        G8RTOS_WaitSemaphore(&sem_PCA9555_Debounce);

        // debounce buttons
        sleep(15);
        // Get buttons
        //buttons = ~(MultimodButtons_Get());
        buttons = G8RTOS_ReadFIFO(BUTTONS_FIFO);
        // clear button interrupt
        GPIOIntClear(BUTTONS_INT_GPIO_BASE,BUTTONS_INT_PIN);
        // check which buttons are pressed

        //make a smack sound when hammer falls
        if(JOYSTICK_GetPress()){
            uint32_t timer_period = SysCtlClockGet() / 800;
            TimerDisable(TIMER1_BASE, TIMER_A);
            TimerLoadSet(TIMER1_BASE, TIMER_A, timer_period - 1);
            TimerEnable(TIMER1_BASE, TIMER_A);

            sleep(150);
            TimerDisable(TIMER1_BASE, TIMER_A);

        }

        //end smack when let go
        if(!JOYSTICK_GetPress()){
            //TimerIntDisable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
            TimerDisable(TIMER1_BASE, TIMER_A);
         }

        G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);
    }
}

//print scores and time
void text_Thread(void) {

    int16_t prevScore=0;
    int16_t prevTime=100;
    char score_str[3];
    score_str[0] = 0;
    score_str[1] = 0;

    char time_str[3];
    while(1){

        if(gameTime !=0){ //while game is running

            if(score == 0){
                sprintf(score_str, "%d", score); // Convert score to a string
                 display_setCursor(20,20); // Set the cursor position
                 display_print('S');
                 display_print('C');
                 display_print('O');
                 display_print('R');
                 display_print('E');
                 display_print(':');
                 display_print('0');
            }

            if(prevScore != score ){ //only run when score changes
                ST7789_DrawRectangle(80, 5, 50,50,background);
                sleep(5);
                ST7789_DrawRectangle(80, 5, 50,50,background); // doubly cover old score

                sprintf(score_str, "%d", score); // Convert score to a string
                display_setCursor(20,20); // Set the cursor position
                display_print('S');
                display_print('C');
                display_print('O');
                display_print('R');
                display_print('E');
                display_print(':');

                display_print(score_str[0]);
                display_print(score_str[1]);
                prevScore = score;
            }

            if(prevTime != gameTime/10 ){ // only run when timer changes by 10
                ST7789_DrawRectangle(180, 5, 50,20,background);
                sleep(5);

                sprintf(time_str, "%d", gameTime/10); // Convert score to a string
                display_setCursor(140,20 ); // Set the cursor position
                display_print('T');
                display_print('I');
                display_print('M');
                display_print('E');
                display_print(':');
                display_print(time_str[0]);
                display_print(time_str[1]);
                prevTime = gameTime/10;
            }


            sleep(10);
        }
    }
}

void Display_Thread(void) {
    srand((int)time(NULL));
    // Initialize / declare any variables here

    for(int i =0; i <5; i++){
        highScores[i] = 0;
    }

    uint32_t readVal;
    int16_t mag1;
    int16_t mag2;
    int16_t prevMalX = 0;
    int16_t prevMalY = 0;
    uint32_t result;
    int16_t joystickX;
    int16_t joystickY;
    uint8_t buttons = 0;
    uint8_t pressed;

    char score_str[3];

    int16_t contFlag = 0;

    int16_t moleTimer = 100;

        //draw moles and holes
        int16_t moleY = 240;
        for(int i =0; i<4; i++){
            moles[i].x = 60;
            moles[i].y = moleY;
            moleY -= 60;
        }

        moleY = 240;
        for(int i =4; i<8; i++){
            moles[i].x = 180;
            moles[i].y = moleY;
            moleY -= 60;
         }

        for(int i =0; i<8; i++){
               G8RTOS_WaitSemaphore(&sem_SPIA);
               ST7789_DrawRectangle(moles[i].x, moles[i].y, 20, 10,ST7789_BROWN );
               G8RTOS_SignalSemaphore(&sem_SPIA);
        }

        int i = rand() % 8;

        moles[i].isVisible = true;

    while(1) {


        // 1. erase old square

        // 2. update position

        // 3. draw new square

        if(gameTime == 0){ // if game over

            TimerDisable(TIMER1_BASE, TIMER_A); //turn off sound

            for(int i =0; i <5; i++){ //update high scores

                if(score > highScores[i]){
                    for(int j = 4; j >= i; j--){
                        highScores[j] = highScores[j-1];
                    }
                    highScores[i] = score;

                    break;
                }

            }

            UARTprintf("High Scores: ");
            for(int i =0; i <5; i++){
                UARTprintf("%d\n" , highScores[i]);
            }

            ST7789_Fill(gameOver); //endscreen

            sprintf(score_str, "%d", score);



            display_setCursor(70,200 ); // Set the cursor position
            display_print('Y');
            display_print('O');
            display_print('U');
            display_print('R');
            display_print(' ');
            display_print('S');
            display_print('C');
            display_print('O');
            display_print('R');
            display_print('E');
            display_print(':');
            display_print(score_str[0]);
            display_print(score_str[1]);

            display_setCursor(70,150 );
            display_print('H');
            display_print('I');
            display_print('G');
            display_print('H');
            display_print(' ');
            display_print('S');
            display_print('C');
            display_print('O');
            display_print('R');
            display_print('E');
            display_print('S');

            for(int i =0; i < 5; i++){
                display_setCursor(120,130 - i * 20 );
                sprintf(score_str, "%d", highScores[i]);
                display_print(score_str[0]);
                display_print(score_str[1]);
            }



            while(1){ // wait until button is pressed to restart
                //G8RTOS_WaitSemaphore(&sem_PCA9555_Debounce);

                buttons = G8RTOS_ReadFIFO(BUTTONS_FIFO);
                //sleep(15);
                //GPIOIntClear(BUTTONS_INT_GPIO_BASE,BUTTONS_INT_PIN);

                if(buttons & SW1){
                    gameTime = 1000;
                    score = 0;
                    ST7789_Fill(background);
                    //G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);
                    break;

                }
                //G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);
                //sleep(50);
            }
        }


        if(moleTimer == 0){ //change mole

            moleTimer = (rand() % (100 - 50 + 1)) + 50; //mole timer set to 50-100
            UARTprintf("SCORE:: %d\n", score);


            for(int i =0; i<8; i++){ //reset random mole

                if(moles[i].isVisible){
                    moles[i].isVisible = false;
                    G8RTOS_WaitSemaphore(&sem_SPIA);
                    ST7789_DrawRectangle(moles[i].x+3, moles[i].y+10, 14, 15,background );
                    G8RTOS_SignalSemaphore(&sem_SPIA);
                }

            }
            int i = rand() % 8;

            moles[i].isVisible = true;
        }

        pressed = JOYSTICK_GetPress();


        G8RTOS_WaitSemaphore(&sem_SPIA);

        if(!pressed){ //delete old mallet position
                    ST7789_DrawRectangle(malletX, malletY, 3, 20,background );
                    ST7789_DrawRectangle(malletX - 3, malletY + 16,9 , 3,background );
                }else{

                    ST7789_DrawRectangle(malletX, malletY, 20+20, 3+5,background );
                    ST7789_DrawRectangle(malletX + 1, malletY - 4,3 +5, 9+5,background );
                }
//        ST7789_DrawRectangle(prevMalX, prevMalY, 3, 20,ST7789_BLACK );
//        ST7789_DrawRectangle(malletX - 3, malletY + 16,9 , 3,ST7789_BLACK );
        G8RTOS_SignalSemaphore(&sem_SPIA);




        result = G8RTOS_ReadFIFO(JOYSTICK_FIFO); //update location
             // normalize the joystick values
             joystickX = ((result >> 16) & 0xFFFF);
             joystickY = ((result >> 0) & 0xFFFF);


             if(joystickY > 2300 ){

                   malletY += 3;
             }

             if(joystickY < 1700){
                 malletY -= 3;
             }

             if(joystickX > 2300 ){

                 malletX -= 3;
             }

             if(joystickX < 1700){
                 malletX += 3;
             }

             if(malletX < 0 ){
                 malletX = 0;
             } else if (malletX >220 ){
                 malletX = 220;
             }

             if(malletY < 0 ){
                 malletY = 0;
             } else if (malletY >260 ){
                 malletY = 260;
             }


        G8RTOS_WaitSemaphore(&sem_SPIA);

        if(!pressed){

            //redraw mallet

            ST7789_DrawRectangle(malletX, malletY, 3, 20,ST7789_ORANGE );
            ST7789_DrawRectangle(malletX - 3, malletY + 16,9 , 3,ST7789_ORANGE );
        }else{

//            draw hit mallet


            ST7789_DrawRectangle(malletX, malletY, 20, 3,ST7789_RED );
            ST7789_DrawRectangle(malletX + 1, malletY - 4,3 , 9,ST7789_RED );
            sleep(10);

            for(int i =0; i < 8; i++){
                if(moles[i].isVisible){ //detect hit
                    if((malletX >= moles[i].x +3 && malletX <= moles[i].x + 17) && (malletY >= moles[i].y + 10 && malletY <= moles[i].y + 25)){
                        moles[i].isVisible = false;
                        moleTimer = 0;
                        contFlag = 1;

                        ST7789_DrawRectangle(moles[i].x+3, moles[i].y+10, 14, 15,background );
                        score++;

                        //ST7789_DrawRectangle(180,180, 100,100, ST7789_BLUE);
                    }
                }
            }



            sleep(10);
            ST7789_DrawRectangle(malletX, malletY, 20, 3,background );
            ST7789_DrawRectangle(malletX + 1, malletY - 4,3 , 9,background );
        }
        G8RTOS_SignalSemaphore(&sem_SPIA);


        if(contFlag){ //continue round once hit
            contFlag = 0;
            continue;
        }


        if( prevMalX != malletX || prevMalY != malletY){
            // limit the magnitude values to the display range
            float mag1Norm = ((float)mag1 / INT16_MAX) * 320;
            float mag2Norm = ((float)mag2 / INT16_MAX) * 320;
            // clear previous rectangle

            for(int i =0; i<8; i++){ //draw mole holes
                   G8RTOS_WaitSemaphore(&sem_SPIA);
                   ST7789_DrawRectangle(moles[i].x, moles[i].y, 20, 10,ST7789_BROWN );
                   G8RTOS_SignalSemaphore(&sem_SPIA);
            }


            // update previous value
            prevMalX = malletX;
            prevMalY = malletY;
        }

        for(int i =0; i<8; i++){ //draw mole
            G8RTOS_WaitSemaphore(&sem_SPIA);
            if(moles[i].isVisible){
                ST7789_DrawRectangle(moles[i].x+3, moles[i].y+10, 14, 15,ST7789_MOLE );
                ST7789_DrawRectangle(moles[i].x+3+3, moles[i].y+10+11,2,2, ST7789_BLACK);
                ST7789_DrawRectangle(moles[i].x+3+9, moles[i].y+10+11, 2,2, ST7789_BLACK);
            }
            G8RTOS_SignalSemaphore(&sem_SPIA);
          }


        moleTimer--;
        gameTime--;

        sleep(20);
    }

}

void Read_Buttons(void) {

    // Initialize / declare any variables here
    uint8_t buttons;


    while(1) {

        // wait for button semaphore
        G8RTOS_WaitSemaphore(&sem_PCA9555_Debounce);
        // Get buttons
        buttons = ~(MultimodButtons_Get());

        // debounce buttons
        sleep(15);

        // clear button interrupt
        GPIOIntClear(BUTTONS_INT_GPIO_BASE,BUTTONS_INT_PIN);
        // update current_buttons value?????
        G8RTOS_WriteFIFO(BUTTONS_FIFO, buttons);

        G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);

    }
}


/********************************Periodic Threads***********************************/

void Update_Joystick(void) {

    // read joystick values
   uint32_t result = JOYSTICK_GetXY();

   uint8_t pressed = JOYSTICK_GetPress();

    // push joystick value to fifo

    G8RTOS_WriteFIFO(JOYSTICK_FIFO,result );

    G8RTOS_WriteFIFO(JOYSTICK_P_FIFO,pressed );
}


/*******************************Aperiodic Threads***********************************/




void Button_Handler() {

    // disable interrupt and signal semaphore
    GPIOIntDisable(BUTTONS_INT_GPIO_BASE,BUTTONS_INT_PIN);

    G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);

}

void DAC_Timer_Handler() {

    // clear the timer interrupt
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    // read next output sample
    uint32_t output = (current_volume)*(dac_signal[dac_step++ % SIGNAL_STEPS]);

    // BONUS: stream microphone input to DAC output via FIFO

    // write the output value to the dac
    MutimodDAC_Write(DAC_OUT_REG,output);
}

