/*
 * sleeper.c
 *
 * Created: 03.02.2018 14:27:16
 * Author : Wojcik98
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define PERIOD_MS 20
#define SECOND (1000/PERIOD_MS)
#define MINUTE (60*SECOND)
#define MIN_BPM (6)
#define MAX_BPM (11)
#define WAITING_PERIOD (4*SECOND)
#define PRESSED(x) (!(PINB & (1<<x)))

void sleep(void){
	TCCR0B &= ~(1<<CS00) & ~(1<<CS01) & ~(1<<CS02);		// turn off timer
	GIMSK |= (1<<PCIE);					// enable PCINT
	PCMSK |= (1<<2) | (1<<3) | (1<<4);	// ...on pins PB2..4
	SREG |= (1<<7);			// enable interrupts
	MCUCR |= (1<<SE);	// enable sleep
	asm("sleep");
}

int main(void)
{
	uint8_t maxTimeMins = 8, maxBright = 100, prevState = 0, bright = 0;
	uint8_t bpm = MAX_BPM, mode = 0;
	uint16_t time = 0, last = 0, period = MINUTE/bpm, halfPeriod = ((period+1)>>1) - (bpm<<2);

	// SETUP
	DDRB = (1<<1) | (1<<0);	// pb0 and 1 as outputs (led pwm), rest as inputs
	TCCR0A |= (1<<WGM02) | (1<<WGM01) | (1<<WGM00);	// fast PWM
	TCCR0A |= (1<<COM0A1) | (1<<COM0B1);		// non-inverted PWM
	TCCR0B |= (1<<CS00);				// set internal clock as timer clock
	MCUCR |= (1<<SM1);	// sleep will enter power-down mode
	PRR |= (1<<PRADC);	// turn off adc

	while(1){
		if(PRESSED(2)){			// changing mode
			mode = (prevState&(1<<2)) ? (mode) : (mode ^ 1);
			prevState |= (1<<2);
		}else{
			prevState &= ~(1<<2);
		}

		if(time < WAITING_PERIOD){	// setting brightness and time
			if(PRESSED(3)){		// increase time or brightness
				if(mode == 0)	// add if button wasn't pressed in last cycle
					maxBright = (maxBright < 245) ? (maxBright + 5*(!(prevState & (1<<3)))) : 255;	// bad steering near 255, but it won't be visible anyway
				else
					maxTimeMins = (maxTimeMins < 21) ? (maxTimeMins + !(prevState & (1<<3))) : maxTimeMins;
				prevState |= (1<<3);
				time = 0;
			}else{
				prevState &= ~(1<<3);
			}
			if(PRESSED(4)){		// decrase...
				uint8_t tmp = maxBright - 5*(!(prevState & (1<<4)));
				if(mode == 0)
					maxBright = (tmp >= 0) ? (tmp) : 0;
				else
					maxTimeMins = (maxTimeMins > 1) ? (maxTimeMins - !(prevState & (1<<4))) : maxTimeMins;
				prevState |= (1<<4);
				time = 0;
			}else{
				prevState &= ~(1<<4);
			}
			OCR0A = maxTimeMins*5;
			OCR0B = maxBright;
		}
		else if(time == WAITING_PERIOD){	// blinking
			OCR0A = 150;
			OCR0B = 150;
			_delay_ms(255);
			OCR0A = 0;
			OCR0B = 0;
			_delay_ms(255);
			OCR0A = 150;
			OCR0B = 150;
			_delay_ms(255);
			OCR0A = 0;
			OCR0B = 0;
			_delay_ms(255);
			last = time;
		}
		else{					// pulsing light
			if(time%(MINUTE) == 0){				// decrease bpm
				bpm = (bpm>MIN_BPM) ? (bpm - 1) : (bpm);
			}
			uint8_t pwm;
			if(last + halfPeriod > time){	// lighten up
				pwm = ((time - last) * (maxBright)) / halfPeriod;
				bright = (pwm > maxBright) ? maxBright : pwm;
			}else{								// darken down
				pwm = ((last + (halfPeriod<<1) - time) * (maxBright)) / halfPeriod;
				bright = (pwm > maxBright) ? maxBright : pwm;
			}

			bright = (last + (halfPeriod<<1) <= time) ? 0 : bright;

			if(last + period <= time){			// start new cycle
				period = (60*SECOND)/bpm;
				halfPeriod = ((period+1)>>1) - (bpm<<2);
				last = time;
				bright = 0;
			}

			if(!(mode&1)){			// light proper LED
				DDRB &= ~(1<<1);
				DDRB |= (1<<0);
				OCR0A = bright;
			}else{
				DDRB |= (1<<1);
				DDRB &= ~(1<<0);
				OCR0B = bright;
			}
		}

		if(time >= maxTimeMins*MINUTE || (PRESSED(3) && PRESSED(4))){	// sleep after maximum time is achieved or two buttons are pressed
			DDRB &= ~(1<<0) & ~(1<<1);	// turn off outputs
			while(PRESSED(3) || PRESSED(4));
			_delay_ms(255);
			time = 0;
			bpm = MAX_BPM;
			prevState = 255;
			sleep();
		}

		_delay_ms(PERIOD_MS);	// loop roughly 50Hz, but we don't care that much for precision and we don't have more timers
		time++;
	}
}

ISR(PCINT0_vect){			// interrupt vector
	GIMSK &= ~(1<<PCIE);	// disable PCINT
	PCMSK = 0;				// disable interrupts inputs
	MCUCR &= ~(1<<SE);		// disable sleep
	TCCR0B |= (1<<CS00);	// turn on timer
	DDRB = (1<<1) | (1<<0);	// set as outputs
}
