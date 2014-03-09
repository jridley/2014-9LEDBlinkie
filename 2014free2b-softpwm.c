/***********************************************************************************/
// this is the code for the 9 LED, Tiny85 blinkie developed for Duckon's Free For All build-a-blinkie
/***********************************************************************************/
// John Ridley
// 2014-02-05
// DragonflyDIY.com
/***********************************************************************************/


// this tunes the accuracy of _delay_ms
// I've screwed with the normal timing to the point where I have to play with this from standard values to get it right.
#define F_CPU 270270

// common modules that we use in this program
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// button tracking - this counts up every 1/xxx of a second that the button is down.
// when the count reaches defined values, things happen.
// or for some functions, when the button is released, different things happen based on how long it was down
int button_down_count = 0;


/***********************************************************************************/
// update this to the max mode when you add more modes
// should be one more than actual modes - the biggest one runs all patterns
#define max_mode 12
/***********************************************************************************/


// These indicate which mode we should be running
// the ISR changes button_mode immediately when the button is clicked
// the main program notices that the two no longer match and switches modes
volatile int button_mode = 1; // this is changed by the button within the ISR
volatile int current_mode = 1; // this follows button_mode when the main code reacts
volatile int shutdown_now = 0;






/***********************************************************************************/
// pattern subroutines here
// 
// rules for writing pattern functions:
// - call ExitCheck often, leave when it returns 1.  Putting large delays between ExitCheck()
//   calls will result in bad button performance.
// - you must set loops=0 at the beginning of your function so ExitCheck() knows how
//   long you have been running.
// - you can use loops yourself if you need to
// - use _delay_ms() for timing

// this is used for timing patterns.  It gets incremented every time the display driver
// completes a cycle.  Current value for 1 second defined below.
// this is the number of hertz that the display is updating at also.
#define ONESECOND 125
volatile int loops = 0;


// display buffer
volatile unsigned char displaybuffer[9];


/**********************************************************************/
// use for millisecond delay. This is not terribly accurate, but probably good enough
// accuracy to whatever the loop counter is
// this will exit as soon as an exit condition is met.
// if this returns 1, leave immediately.
#define DELAY(x) if (Delay(x)) return
int Delay(int ms)
{
	int mscount = 0;
	while (mscount < ms)
	{
		_delay_ms(1);
		if (button_mode != current_mode)
			return 1;
		if (shutdown_now)
			return 1;
		mscount++;
	}
	return 0;
}

// set loops to 0 before beginning a function that will call this
// mostly depricated, probably want to just use Delay() instead.
int ExitCheck()
{
	if (button_mode != current_mode)
		return 1;
	if (loops > (ONESECOND*60))
		return 1;
	return 0;
}

/**********************************************************************/
void ClearDisplay()
{
	for (int x=0; x<9; x++)
		displaybuffer[x] = 0;
}

/**********************************************************************/
// call when mode switches, it will indicate the new mode for 1 second
void ModeSwitchPattern()
{
	for (int x=0; x<9; x++)
	{
		// max_mode is "rotate all modes" - display all the LEDs
		if ((current_mode == max_mode) || (current_mode & (1<<x)) > 0)
			displaybuffer[x] = 15;
		else
			displaybuffer[x] = 0;
	}
	loops = 0;
	while (loops < ONESECOND)
	{
		if (ExitCheck())
			return;
	}
}


/**********************************************************************/
void PopPopPopBuzz()
{
	loops = 0;
	while (loops < ONESECOND*60)
	{
		for (char y=0; y<4; y++)
		{
			DELAY(80);
			ClearDisplay();
			DELAY(80);
			for (int x=0; x<9; x++)
				displaybuffer[x] = 15;
		}
		for (int y=15; y>0; y--)
		{
			DELAY(30);
			for (int x=0; x<9; x++)
				displaybuffer[x] = y-1;
		}
		DELAY(250);
	}
}

/**********************************************************************/
void SlowFadingSequence()
{
	loops = 0;
	ClearDisplay();
	int cur = 0;
	int dir = 1;
	while (loops < ONESECOND*60)
	{
		// work with current bit first
		if (displaybuffer[cur] == 15)
		{
			// time to move to the next bit
			if (dir == 1)
			{
				if (cur == 8)
				{
					cur = 7;
					dir = -1;
				}
				else
					cur++;
			}
			else
			{
				if (cur == 0)
				{
					cur = 1;
					dir = 1;
				}
				else
					cur--;
			}
		}
		displaybuffer[cur]++;
		int last = cur-dir;
		if ((last >= 0) && (last <= 8))
			displaybuffer[last]--;
		DELAY(100);
	}
}


/**********************************************************************/
#define KROVERSHOOT 5
void KnightRider()
{
	int dotpos = 0;
	int direction = 1;
	int x;

	loops = 0;
	ClearDisplay();

	// run for 1 minute
    while (loops < ONESECOND*60)
    {
		DELAY(50);
		// diminish every LED every cycle
		for (x=0; x<9; x++)
			if (displaybuffer[x] > 3)
				displaybuffer[x] -=4;
			else
				displaybuffer[x] = 0;

		// we let the "scan" overshoot the end a bit for realism
		if ((dotpos >= 0) && (dotpos <= 8))
			displaybuffer[dotpos] = 15;

		if (direction == 1)
			if (dotpos == 8+KROVERSHOOT)
			{
				dotpos = 8;
				direction = -1;
			}
			else
				dotpos++;
		else
			if (dotpos == 0-KROVERSHOOT)
			{
				dotpos = 0;
				direction = 1;
			}
			else
				dotpos--;
    }
}



void MovingIrritation()
{
	loops = 0;
	int cur = 0;
	ClearDisplay();
	while (loops < ONESECOND*60)
	{
		int curplus = cur+1;
		if (curplus > 8)
			curplus = 0;

		for (char x=0; x<4; x++)
		{
			displaybuffer[cur] = 15;
			displaybuffer[curplus] = 15;
			DELAY(40);
			ClearDisplay();
			DELAY(40);
		}
		cur = curplus;
	}
}


// called when we're shutting down.  Old skool TV dying thing
// no escapes, just delay and stop.
void shutdownPattern()
{
	for (char x=0; x<9; x++)
		displaybuffer[x] = 1; // start with all on low
	_delay_ms(100);

	// fade to center, center getting brighter as they all blink out
	for (char x=4; x>0; x--)
	{
		_delay_ms(100);
		displaybuffer[4-x] = 0;
		displaybuffer[4+x] = 0;
		displaybuffer[4] = 15-(x*3);
	}
	_delay_ms(100);
	for (char x=15; x>0; x--)
	{
		_delay_ms(50);
		displaybuffer[4] = x-1;
	}
}


/**********************************************************************/
// simple chase
void chase()
{
	loops = 0;
	ClearDisplay();
	// run for 1 minute
    while (loops < ONESECOND*60)
	{
		for (char x=0; x<9; x++)
		{
			displaybuffer[x] = 15;
			DELAY(100);
			displaybuffer[x] = 0;
		}
	}
}

/**********************************************************************/
void Surge()
{
	loops = 0;
	ClearDisplay();
	// run for 1 minute
    while (loops < ONESECOND*60)
	{
		for (char x=0; x<9; x++)
			displaybuffer[x] = 15;
		for (char y=15; y>0; y--)
		{
			DELAY(20);
			for (char x=0; x<9; x++)
				displaybuffer[x] = y-1;
		}
		DELAY(200);
	}
}

/**********************************************************************/
void EMTFlash()
{
	loops = 0;
	char side;
	side = 0;
	ClearDisplay();
	// run for 1 minute
    while (loops < ONESECOND*60)
	{
		for (char x=0; x<3; x++)
		{
			if (side == 0)
			{
				displaybuffer[0] = 15;
				displaybuffer[1] = 15;
				displaybuffer[2] = 15;
				displaybuffer[3] = 15;
			}
			else
			{
				displaybuffer[5] = 15;
				displaybuffer[6] = 15;
				displaybuffer[7] = 15;
				displaybuffer[8] = 15;
			}
			DELAY(30);
			ClearDisplay();
			DELAY(30);
		}
		DELAY(300);
		if (side == 0)
			side = 1;
		else
			side = 0;
	}
}


void gbfill(char pos, char size)
{
	for (char x=0; x<9; x++)
		if ((x < pos) || (x >= (pos + size)))
			displaybuffer[x] = 0;
		else
			displaybuffer[x] = 15;
}
void growingbounce()
{
	loops = 0;
	char direction = 1;
	char curpos = 0;
	char size = 1;
	char fooloop = 0;
	ClearDisplay();
	while (loops < ONESECOND*60)
	{
gbtop:
		gbfill(curpos, size);
		DELAY(80);
		if (size == 9)
		{
			DELAY(300);
			for (char x=15; x>0; x--)
			{
				for (char y=0; y<9; y++)
					displaybuffer[y] = x-1;
				DELAY(50);
			}
			DELAY(100);
			curpos = 0;
			size = 1;
			direction = 1;
			goto gbtop;
		}
gbrecalc:
		if (direction == 1)
			if (curpos+size < 9)
				curpos++;
			else
			{
				direction = 0;
				goto gbrecalc;
			}
		else
			if (curpos > 0)
				curpos--;
			else
			{
				direction = 1;
				if (fooloop++ == 3)
				{
					fooloop = 0;
					size++;
				}
				else
					goto gbrecalc;
			}
	}
}

void Cinema()
{
	loops = 0;
	while (loops < ONESECOND*60)
	{
		for (char x=0; x<3; x++)
		{
			ClearDisplay();
			for (char y=x; y<9; y+=3)
				displaybuffer[y] = 15;
			if (Delay(60))
				return;
		}
	}
}


/**********************************************************************/
// center weighted throb
void ctrDisplay(int value)
{
	int curval, maxval;
	for (char x=0; x<5; x++)
	{
		curval = value;
		if (curval > 15)
		{
			value = curval-15;
			curval = 15;
		}
		else
			value = 0;

		displaybuffer[4-x] = curval;
		if (x>0)
			displaybuffer[4+x] = curval;
	}
}
void centerThrob()
{
	loops = 0;

	// run for 1 minute
    while (loops < ONESECOND*60)
	{
		for (int x=0; x<70; x+=2)
		{
			ctrDisplay(x);
			DELAY(4);
		}
		for (int x=70; x>=0; x-=2)
		{
			ctrDisplay(x);
			DELAY(4);
		}
		DELAY(200);
	}
}

/**********************************************************************/
void LEDTEST()
{
	loops = 0;
#define LEDTESTDELAY 60
	int x,y;
	for (x=0; x<9; x++)
	{
		displaybuffer[x] = 15;
		DELAY(LEDTESTDELAY);
		displaybuffer[x] = 0;
	}

	for (y=1; y<15; y++)
	{
		for (x=0; x<9; x++)
			displaybuffer[x] = y;
		DELAY(LEDTESTDELAY);
	}

	for (y=0;y<3; y++)
	{
		for (x=0; x<9; x++)
			displaybuffer[x] = 15;
		DELAY(LEDTESTDELAY);
		for (x=0; x<9; x++)
			displaybuffer[x] = 0;
		DELAY(LEDTESTDELAY);
	}
}

void accelerator()
{
	
}

/**********************************************************************/
void LAZAR()
{
	loops = 0;
	ClearDisplay();
	// run for 1 minute
    while (loops < ONESECOND*60)
	{
		for (short x=0; x<9; x++)
		{
			displaybuffer[x] = 15;
			DELAY(20);
			displaybuffer[x] = 0;
		}
		DELAY(100);
	}
}


// VU Meter utility function: set level
void vulevel(short level)
{
	loops = 0;
	for (short x=0; x<9; x++)
	{
		if (x <= level)
			displaybuffer[x] = 255;
		else
			displaybuffer[x] = 0;
	}
}
void vumeter()
{
	loops = 0;

	short currentlevel = 0;
	while (1)
	{
	}
}

/**********************************************************************/



void SetupDisplayInterrupt()
{
   // Setup Timer 0
   TCCR0A = 0b00000000;   // Normal Mode
   TCCR0B = 0b00000001;   // No Prescaler
   TCNT0 = 0;        	// Initial value

// enable interrupts
   TIMSK = (1<<TOIE0);  	// Timer Mask: Enable interrupt on Timer 0 overflow
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
// this is the main function


int main()
{
	int rotator = 0; // used for rotating all modes when in demo mode

    SetupDisplayInterrupt();
    sei();           	// Global enable Interrupts

	ClearDisplay();

//	LEDTEST(); // need a way to call this except on initial power-up


	while (1)
	{
		if (shutdown_now)
		{
			shutdown_now = 0;
			shutdownPattern();
			button_down_count = 101;
			power_off();
		}
		if (button_mode != current_mode)
		{
			current_mode = button_mode;
			ModeSwitchPattern(); // this displays the current mode in binary for 1 second
		}
		else
		{
			int runmode;
			if (current_mode == max_mode)
			{
				runmode = rotator;
				rotator++;
			}
			else
			{
				runmode = current_mode;
			}
			switch (runmode)
			{
			case 1:
				KnightRider();
				break;
			case 2:
				chase();
				break;
			case 3:
				EMTFlash();
				break;
			case 4:
				LAZAR();
				break;
			case 5:
				centerThrob();
				break;
			case 6:
				Surge();
				break;
			case 7:
				growingbounce();
				break;
			case 8:
				Cinema();
				break;
			case 9:
				SlowFadingSequence();
				break;
			case 10:
				MovingIrritation();
				break;
			case 11:
				PopPopPopBuzz();
				break;
			}
		}
	}
	return 0;
}





// **********************************************************
// *** BUTTON PRESS HANDLING
// **********************************************************
//Setup pin change interrupt used to wake from sleep
void init_pcint(void)
{
  GIMSK |= 1<<PCIE;   // General Interrupt Mask: Enable Pin Change Interrupt
  PCMSK |= 1<<PCINT0; // Pin Change Mask: Watch for Pin Change on Pin5 (PB0)
}

//Pin Change Interrupt
ISR(PCINT0_vect)
{
  sleep_disable();
  GIMSK &= ~(1<<PCIE); //Disable the interrupt so it doesn't keep flagging
  PCMSK &= ~(1<<PCINT0);
}




// turn off and wait for button press
void power_off(void)
{
  cli();
  TIMSK &= ~(1<<TOIE0);  //Timer Interrupt Mask: Turn off the Timer Overflow Interrupt bit

  // don't proceed until the button is released
  DDRB = 0x0;   // PB0 to input mode
  PORTB = 0x01;   // pull-up active
  while ((PINB & 0x01) == 0);
  _delay_ms(50); // Wait a moment to make sure the button isn't bouncing

  // go into power down mode until the button gets pressed again
gotosleep:
  init_pcint();     	// set up pin change interrupt
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  sei(); // enable interrupts (so we can come back alive again when the button is pressed)

  // this statement puts the CPU into power down mode until the pin change interrupt awakens it.
  sleep_mode();

  // when we hit here, we've awakened from sleep
  // interrupts will be disabled when we get back from here but RETI at the end of this function will enable interrupts
 
  // detect double click
  _delay_ms(50); // debounce button press
 
  while ((PINB & 0x01) == 0); // wait for button release
  _delay_ms(50); // debounce button release


  // button must be double clicked in 1/2 second, otherwise it was just something
  // randomly hitting the button and we go back to sleep.
  for (int x=0; x<500; x++) // 1/2 second to press button again
  {
      if ((PINB & 0x01) == 0) // button was pressed again
   	 goto wakeup;
    _delay_ms(1);
  }
  // no subsequent button press, false alarm
  goto gotosleep;
 
wakeup:
    button_down_count = 101; // don't switch modes upon release.
    SetupDisplayInterrupt(); // put interrupts back to run mode
}





// **********************************************************
// *** DISPLAY HANDLER
// **********************************************************
// LED channel 0 = PB0
#define CLEAR0 (pinlevelB &= ~(1<<0))
// LED channel 1 = PB1
#define CLEAR1 (pinlevelB &= ~(1<<1))
// LED channel 2 = PB4
#define CLEAR2 (pinlevelB &= ~(1<<4))

// to drive bank 0 (0, 3, 6)
// PB2 - low, PB3 - tristate
// to drive bank 1 (1, 4, 7)
// PB2 - high, PB3 - tristate, compare values are inverted
// to drive bank 2 (2, 5, 8)
// PB2 - tristate, PB3 - low

// even though the max value for luminance is 15, it helps to count a bit past that
// so even at full brightness there's some off time.  Otherwise the last 4 or 5 values
// are equally as bright.
#define cyclecount 20

// This gets called on timer interrupt
// this also detects button presses
ISR(TIM0_OVF_vect)
{
	static short loopcount=0;

	static unsigned char PWMCount = 0;
	static short bufferOffset=0; // points to the set of 3 in displaybuffer that we're on right now
	static unsigned char compare[3];
	static unsigned char defaultPinLevel = 1<<0 | 1<<1 | 1<<4; // for testing, bank 0
	static unsigned char pinlevelB = 1<<0 | 1<<1 | 1<<4; // all 3 pins on
	static unsigned char DDRval = 0;


	// Set values calculated during the last interrupt
	// doing them here at the beginning eliminates timing differences due to varying calculation times in the function
	// so we get accurate timing without worrying about how much time the ISR takes.
   	DDRB = DDRval;
	PORTB = pinlevelB;

	// the rest of the function is concerned with setting up those values for
	
	// we do PWM manually every entry, then we switch banks occasionally.
	// we have 3 channels to PWM on
	// 16 level greyscale is fine
	if (PWMCount == 0)
	{
		// reload local values only at the beginning of a cycle to avoid weird flicker
		if (bufferOffset == 3)
		{
			// bank 2 needs pwm values inverted since the LEDs are running reversed
			compare[0] = cyclecount - displaybuffer[0+bufferOffset]-1;
			compare[1] = cyclecount - displaybuffer[1+bufferOffset]-1;
			compare[2] = cyclecount - displaybuffer[2+bufferOffset]-1;
		}
		else
		{
			compare[0] = displaybuffer[0+bufferOffset];
			compare[1] = displaybuffer[1+bufferOffset];
			compare[2] = displaybuffer[2+bufferOffset];
		}

		pinlevelB = defaultPinLevel; // all 3 pins on along with whatever bank driver bit needs to be set.
	}

	if (compare[0] == PWMCount) CLEAR0;
	if (compare[1] == PWMCount) CLEAR1;
	if (compare[2] == PWMCount) CLEAR2;
	++PWMCount;

	TCNT0 = 192;        	// Initial value

	if (PWMCount == cyclecount) // giving a little off time even at full on gives a little more dynamic range control
		PWMCount = 0;
	else
		return;


	// switch banks every full PWM cycle
	// every 16, we drop down here.  PWMcount will be zero and values will get reloaded next round.

	// read the switch on a 4th cycle
	if (loopcount == 3)
	{
		// this is a good place to read the switch
		DDRB = 0x00; // all pins to input
		PORTB = 1<<0; // PB0 pull-up on
		if ((PINB & 1) == 0) // button pressed = 0 on PB0
   	 	{
   		 	// if button has been down for a long time, power off
	   		 if (button_down_count++ == 100)
   			 {
#if 0
				// this is to run the power down immediately
   				 button_down_count = 0;
   				 power_off(); // then shut off until the button is pressed
#else
				// this is if the main program will do the shutdown, we just tell it that it should be done.
				// this allows something to be done before actually  shutting down like a special pattern that
				// announces shutdown
				 shutdown_now = 1;
#endif
   		 	}
	   	 } else {
   			 // if button down count is > 100 then we're returning from power down, ignore button activity on first release.
	   		 // if button is now up and was down long enough for it to not be a bounce but not long enough to be power off, switch modes
   			 if ((button_down_count > 5) && (button_down_count < 101))
   			 {
	   			 if (button_mode == max_mode)
   					 button_mode = 1;
   				 else
	   				 button_mode++;
   			 }
   			 button_down_count = 0;
	   	 }
   		 PORTB = 0; // don't leave the pull-up on


		// recycle back to beginning of display cycle
		loopcount = 0;
		loops++;
	}

	if (loopcount == 0)
	{
		// To run bank 0, tristate PB3, turn PB2 on as output
		// (plus the usual 3 for PWM on the LEDs)
		DDRval = 1<<DDB2  | 1<<DDB0 | 1<<DDB1 | 1<<DDB4;

		bufferOffset = 0;
		// switch back to bank 0
		// tristate PB3, turn PB2 on and set it low
		defaultPinLevel = 1<<0 | 1<<1 | 1<<4; // the four LED lines on, the common low
	}
	if (loopcount == 1)
	{
		bufferOffset = 3;
		// To display bank 1:
		// tristate PB3 (set as input for switch read), turn PB2 on as output (same as bank 0) but PB2 is high
		DDRval = 1<<DDB2  | 1<<DDB0 | 1<<DDB1 | 1<<DDB4;

		defaultPinLevel = 1<<0 | 1<<1 | 1<<4   | 1<<2 | 1<<3; // the four LED lines on, the common (PB2) high, PB3 high to set the pullup for the switch read
	}
	if (loopcount == 2)
	{
		bufferOffset = 6;
		// To display bank 1:
		// tristate PB2, turn PB3 on as output with PB3 low
		DDRval = 1<<DDB3  | 1<<DDB0 | 1<<DDB1 | 1<<DDB4;
		defaultPinLevel = 1<<0 | 1<<1 | 1<<4; // the four LED lines on, the common low
	}

	loopcount++;
}
