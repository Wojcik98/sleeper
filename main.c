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
#define MIN_BPM (4)
#define MAX_BPM (9)
#define WAITING_PERIOD (4*SECOND)
#define PRESSED(x) (!(PINB & (1<<x)))

#define PLUS_BTN 3
#define MINUS_BTN 4
#define MODE_BTN 2

void sleep(void){
	TCCR0B &= ~(1<<CS00) & ~(1<<CS01) & ~(1<<CS02);		// turn off timer
	GIMSK |= (1<<PCIE);					// enable PCINT
	PCMSK |= (1<<PLUS_BTN) | (1<<MINUS_BTN) | (1<<MODE_BTN);	// ...on buttons
	SREG |= (1<<7);			// enable interrupts
	MCUCR |= (1<<SE);	// enable sleep
	asm("sleep");
}

int main(void)
{
	uint8_t maxTimeMins = 8, maxBright = 100, prevState = 0, bright = 0;
	uint8_t bpm = MAX_BPM, mode = 0;
	uint16_t time = 0, last = 0, period = MINUTE/bpm, halfPeriod = ((period+1)>>1);//- (bpm<<3);
	uint8_t plus_msk = (1<<PLUS_BTN), minus_msk = (1<<MINUS_BTN), mode_msk = (1<<MODE_BTN);

	// SETUP
	DDRB = (1<<1) | (1<<0);	// pb0 and 1 as outputs (led pwm), rest as inputs
	TCCR0A |= (1<<WGM02) | (1<<WGM01) | (1<<WGM00);	// fast PWM
	TCCR0A |= (1<<COM0A1) | (1<<COM0B1);		// non-inverted PWM
	TCCR0B |= (1<<CS00);				// set internal clock as timer clock
	MCUCR |= (1<<SM1);	// sleep will enter power-down mode
	PRR |= (1<<PRADC);	// turn off adc

	while(1){
		if(PRESSED(MODE_BTN)){			// changing mode
			mode = (prevState&mode_msk) ? (mode) : (mode + 1);
			prevState |= mode_msk;
		}else{
			prevState &= ~mode_msk;
		}

		if(time < WAITING_PERIOD){	// setting brightness and time
			if(PRESSED(PLUS_BTN)){		// increase time or brightness
				if(!(mode&1))	// add if button wasn't pressed in last cycle
					maxBright = (maxBright < 245) ? (maxBright + 5*(!(prevState & plus_msk))) : 255;	// bad steering near 255, but it won't be visible anyway
				else
					maxTimeMins = (maxTimeMins < 21) ? (maxTimeMins + !(prevState & plus_msk)) : maxTimeMins;
				prevState |= plus_msk;
				time = 0;
			}else{
				prevState &= ~plus_msk;
			}
			if(PRESSED(MINUS_BTN)){		// decrase...
				uint8_t tmp = 5*(!(prevState & minus_msk));
				if(!(mode&1))
					maxBright = (maxBright >= tmp) ? (maxBright - tmp) : 0;
				else
					maxTimeMins = (maxTimeMins > 1) ? (maxTimeMins - !(prevState & minus_msk)) : maxTimeMins;
				prevState |= minus_msk;
				time = 0;
			}else{
				prevState &= ~minus_msk;
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
				pwm = (pwm*4)/3;
				bright = (pwm > maxBright) ? maxBright : pwm;
			}else{								// darken down
				pwm = ((time - last - halfPeriod) * maxBright) / halfPeriod;
				pwm = (pwm*4)/3;
				pwm = (pwm > maxBright) ? maxBright : pwm;
				bright = maxBright - pwm;
			}

			if(last + period <= time){			// start new cycle
				period = MINUTE/bpm;
				halfPeriod = ((period+1)>>1);// - (bpm<<3);
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

		if(time >= maxTimeMins*MINUTE || (PRESSED(PLUS_BTN) && PRESSED(MINUS_BTN))){	// sleep after maximum time is achieved or two buttons are pressed
			DDRB &= ~(1<<0) & ~(1<<1);	// turn off outputs
			while(PRESSED(PLUS_BTN) || PRESSED(MINUS_BTN));
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
