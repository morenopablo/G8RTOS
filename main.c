// Lab 5, uP2 Fall 2024
// Created: 2023-07-31
// Updated: 2024-08-01
// Lab 5 is intended to introduce you to basic signal processing. In this, you will
// - continuously sample audio data from a microphone
// - process the audio data stream using the Goertzel algorithm
// - output audio data to headphones via a DAC
// - provide user feedback via the display

/************************************Includes***************************************/

#include "G8RTOS/G8RTOS.h"
#include "./MultimodDrivers/multimod.h"

#include "./threads.h"
#include "driverlib/interrupt.h"

/************************************Includes***************************************/

/*************************************Defines***************************************/
/*************************************Defines***************************************/

/********************************Public Variables***********************************/
/********************************Public Variables***********************************/


/************************************MAIN*******************************************/

int main(void)
{
    // Sets clock speed to 80 MHz. You'll need it!
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    // you might want a delay here (~10 ms) to make sure the display has powered up
    SysCtlDelay(10);
    // initialize the G8RTOS framework
    G8RTOS_Init();
    multimod_init();
    ST7789_Fill(background);
    display_setTextSize(2);
    // Add semaphores, threads, FIFOs here

    G8RTOS_InitSemaphore(&sem_I2CA, 1);
    G8RTOS_InitSemaphore(&sem_SPIA, 1);
    G8RTOS_InitSemaphore(&sem_PCA9555_Debounce, 1);
    G8RTOS_InitSemaphore(&sem_Joystick_Debounce, 1);


    G8RTOS_InitFIFO(BUTTONS_FIFO);
    G8RTOS_InitFIFO(JOYSTICK_FIFO);
    G8RTOS_InitFIFO(JOYSTICK_P_FIFO);

    G8RTOS_AddThread(Idle_Thread, 255, "idle\0");
    G8RTOS_AddThread(Display_Thread, 250, "display\0");
    G8RTOS_AddThread(Read_Buttons, 251, "buttons\0");
    G8RTOS_AddThread(text_Thread,252, "text\0");
    G8RTOS_AddThread(Speaker_Thread, 253, "speaker\0");

    // add periodic and aperiodic events here (check multimod_mic.h and multimod_buttons.h for defines)


    G8RTOS_Add_APeriodicEvent(Button_Handler,4, INT_GPIOE );
    G8RTOS_Add_APeriodicEvent(DAC_Timer_Handler,5, DAC_INTERRUPT );

    G8RTOS_Add_PeriodicEvent(Update_Joystick, 50, 1);

    G8RTOS_Launch();
    while (1);
}

/************************************MAIN*******************************************/
