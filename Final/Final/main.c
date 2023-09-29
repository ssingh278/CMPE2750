/********************************************************************/
// Program:  Stopwatch
// Processor:     Atmega328p
// Bus Speed:     8 MHz
// Author:        Sharry SIngh
// Details:       making stopwatch with atmega328p using various concept
// Date:          Sept 28, 2023


/********************************************************************/
// Library includes
/********************************************************************/
// your includes go here
#define F_CPU 8E6 // with external xtal enabled, and clock div/8, bus == 2MHz
#include <avr/io.h>
#include <util/delay.h>
#include "I2C.h"
#include "PCF8574A.h"
#include "timer.h"
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <stdio.h>


/********************************************************************/
// Constant Defines
/********************************************************************/
#define  BUTTON1 PB1//button switch connected to port B pin1
#define  BUTTON2 PB2//button switch connected to port B pin2


//To model switch states
typedef enum SwState
{
	Idle,
	Pressed,
	Held,
	Released,
}SwState;
// enum to describe mc switch positions
// tag values represent actual bit positions
typedef enum SwitchPos
{
	SWL_RIGHT= BUTTON2,
	SWL_LEFT = BUTTON1
} SWL_SwitchPos;

enum States  //enum for stopwatch states
{
	IDLE,
	RUN,
	STOP,
	RESET
};

/********************************************************************/
// Local Prototypes
/********************************************************************/
void UpdateLCD();//function to update LCD
void sw_Run();//function when state is run
void sw_Stop();//function when state is stop
void sw_Reset();//function hen state is reset
SwState Sw_Process(SwState*, SWL_SwitchPos);
int SWL_Pushed (SWL_SwitchPos);

void init_portB()
{
	
	DDRB = 0xFFu;//all pin of port b as output
	
	// Set PB1 and PB2 as input pins
	DDRB &= ~(1 << BUTTON1);
	DDRB &= ~(1 << BUTTON2);

	// Enable pull-up resistors on PB1 and PB2
	PORTB |= (1 << BUTTON1);
	PORTB |= (1 << BUTTON2);
}


/********************************************************************/
// Global Variables
/********************************************************************/

const unsigned int _Timer_OC_Offset = 12500;// constant for timer output compare offset, init and ISR rearm

// global isr-modified 100ms tick counter
volatile unsigned long _Update=0;
volatile unsigned long _secondCounter=0;

// global isr-modfied  tick counts
unsigned int _seconds=00, _minutes = 00, _hours =00;

enum States _state= IDLE;//current state of STOPWATCH

char rxTime[80] = {0}; // array to diplay time
	
SwState _leftButton = Idle;
SwState _rightButton = Idle;
	

int main(void)
{
/********************************************************************/
// initializations
/********************************************************************/
	init_portB();//buttons
	
	Timer_Init(Timer_Prescale_64, _Timer_OC_Offset); // 100ms intervals, ISR
	
	// jump up to 8MHz, as 2MHz is a little slow
	// 11.11.2 ATmega328PB Full
	CLKPR = 0b10000000; // enable changes
	CLKPR = 0b00000001; // set to div by 2 (16 / 2 = 8MHz)
	
	
	DDRD |= 1 << PORTD7; // make portd pin 7 an output (PD7)
	
	I2C_Init(F_CPU,I2CBus100);
	sleep_enable();
	
	LCD_Init(F_CPU);
	sei();
	
	
	LCD_Clear();
	(void)sprintf(rxTime,"Time : %02d:%02d:%02d",_hours,_minutes,_seconds);
	LCD_StringXY(0,0,rxTime);
	LCD_StringXY(0,1,"State : Idle");
	
	
/********************************************************************/
// main program loop
/********************************************************************/
	while(1)
	{
		sleep_cpu();//sleeping CPU
		
		if(_state == IDLE && (Sw_Process(&_leftButton, SWL_LEFT) == Pressed))// if state is idle then we will only accept button to start only 
		{
			_state = RUN;
		}
		
		//Switch case for all states
		switch (_state)
		{
			case RUN:
				sw_Run(); // running the stopwatch
				if(Sw_Process(&_rightButton, SWL_RIGHT) == Pressed)//we could only stop , no any other button response required
				{
					_state = STOP;
				}
				break;
			
			case STOP:
				if(Sw_Process(&_leftButton, SWL_LEFT) == Pressed) // if user want to start again
				{
					_state = RUN;
				}
				if(Sw_Process(&_rightButton, SWL_RIGHT) == Pressed) // if user want to reset
				{
					_state = RESET;
				}
				sw_Stop();
				break;
			
			case RESET:
				sw_Reset();
				if(Sw_Process(&_leftButton, SWL_LEFT) == Pressed)// after reset user can only start the SW
				{
					_state = RUN;
				}
				break;
			
			default:
			break;
		}
	}
}//****************************************************************************************** **
// void sw_Run()
//Purpose: This function will increments all the constraints 
//Parameters: no
//Returns: nothing
//****************************************************************************************** **void sw_Run(){	if(_secondCounter >=10)//counter is incrementing after every 100 ms
	{
		_secondCounter =0;
		PORTD |= 0b10000000; // turning on LED after every one second
		
		++_seconds;// incrementing counter
		
		if(_seconds >=60)//if time is = 60 then reseting second and increasing minute
		{
			_seconds=0;
			++_minutes;
		}
		if(_minutes>=60)
		{
			_minutes=0;
			++_hours;
		}
	}
	else
	{
		PORTD &= ~(0b10000000); // turning off LED
	}
	UpdateLCD();//updating LCD
}//****************************************************************************************** **
// void sw_Stop()
//Purpose: This function will only work when state is stop
//Parameters: no
//Returns: nothing
//****************************************************************************************** **void sw_Stop(){	UpdateLCD();}//****************************************************************************************** **
// void sw_Reset()
//Purpose: This function will reset all the constraint 
//Parameters: no
//Returns: nothing
//****************************************************************************************** **void sw_Reset(){	_seconds=00, _minutes = 00, _hours =00;	UpdateLCD();}//****************************************************************************************** **
// void UpdateLCD()
//Purpose: This function will update LCD 
//Parameters: no
//Returns: nothing
//****************************************************************************************** **void UpdateLCD(){	//updating LCD after every 500 ms
	if(_Update>=5)
	{
		_Update = 0; //reseting count
		char rxTime[80] = {0};
		(void)sprintf(rxTime,"Time : %02d:%02d:%02d",_hours,_minutes,_seconds);
		LCD_StringXY(0,0,rxTime);
		
		// displaying the state on LCD  
		if(_state == IDLE)
			LCD_StringXY(0,1,"State : Idle   ");
		if(_state == RUN)
			LCD_StringXY(0,1,"State : Running");
		if(_state == STOP)
			LCD_StringXY(0,1,"State : Stop   ");
		if(_state == RESET)
			LCD_StringXY(0,1,"State : Reset  ");
		
	}}// is a specific switch being pushed (T/F)
int SWL_Pushed(SWL_SwitchPos button)
{
	return (!(PINB & (1<<button))) ? 1 : 0;
}// Optional - Using
SwState Sw_Process(SwState* state, SWL_SwitchPos sw)
{
	if(SWL_Pushed(sw))
	{//Switch active
		if(*state == Idle)
		{
			*state = Pressed;
		}
		else
		{
			*state = Held;
		}
	}
	else
	{//Switch not-active
		if(*state == Held)
		{
			*state = Released;
		}
		else
		{
			*state = Idle;
		}
	}
	return *state;
}// output compare A interrupt
ISR(TIMER1_COMPA_vect)
{
	// rearm the output compare operation
	OCR1A += _Timer_OC_Offset; // 100ms intervals @ Prescale 64
	
	// count 100ms ticks!
	++_Update;
	
	if(_state == RUN)//for accuracy we will only increment seconds if state is run
		++_secondCounter;
}
