// Timer library, ATmega328P Version
// Simon Walker, NAIT
// Revision History:
// March 18 2022 - Initial Build

// model of timer output compare (channel A) ISR
/*
// output compare A interrupt
ISR(TIMER1_COMPA_vect)
{
	// rearm the output compare operation
	OCR1A += _Timer_OC_Offset; // 1ms intervals @ prescale 64
	
	// up the global tick count
	++_Ticks;
}
*/

typedef enum Timer_Prescale
{
	Timer_Prescale_1 = 1,
	Timer_Prescale_8 = 2,
	Timer_Prescale_64 = 3,
	Timer_Prescale_256 = 4,
	Timer_Prescale_1024 = 5
} Timer_Prescale;

typedef enum Timer_PWM_Channel
{
   Timer_PWM_Channel_OC0A,
   Timer_PWM_Channel_OC0B
} Timer_PWM_Channel;

typedef enum Timer_PWM_ClockSel
{
  Timer_PWM_ClockSel_STOP = 0,
  Timer_PWM_ClockSel_Div1 = 1,
  Timer_PWM_ClockSel_Div8 = 2,
  Timer_PWM_ClockSel_Div64 = 3,
  Timer_PWM_ClockSel_Div256 = 4,
  Timer_PWM_ClockSel_Div1024 = 5
  
} Timer_PWM_ClockSel;

typedef enum Timer_PWM_Pol
{
  Timer_PWM_Pol_NonInverting,
  Timer_PWM_Pol_Inverting
} Timer_PWM_Pol;

// bring the timer up with basic OCA functionality enabled
void Timer_Init (Timer_Prescale pre, unsigned int uiInitialOffset);

// bring up timer 0 in fast PWM mode
void Timer_F_PWM0 (Timer_PWM_Channel chan, Timer_PWM_ClockSel clksel, Timer_PWM_Pol pol);
