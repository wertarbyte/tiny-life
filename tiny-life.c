#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

#define alive(i,j) ( field[current][i] & (1<<(j)) )

#define set_alive(f,i,j) ( f[i] |= (1<<(j)) )
#define set_dead(f,i,j) ( f[i] &= ~(1<<(j)) )

#define mod(x,y) (x>=y ? x-y : (x<0 ? x+y : x))

#define ROWS 7
#define COLS 5

uint8_t current = 0;

#define N_PRESEEDS 2
uint8_t i_preseed = 0;
uint8_t preseed[N_PRESEEDS][COLS] = {
	{
	0b00100000,
	0b00010000,
	0b01110000,
	0b00000000,
	0b00000000,
	},
	{
	0b00000000,
	0b00001000,
	0b00111000,
	0b00001000,
	0b00000000,
	},
};

uint8_t field[2][COLS];

const uint8_t COL_PIN[COLS] = {
	PD0,
	PD1,
	PD2,
	PD3,
	PD4,
};

const uint8_t ROW_PIN[ROWS] = {
	PB0,
	PB1,
	PB2,
	PB4,
	PB3,
	PB5,
	PB6
};

void inline seed(void) {
	for (uint8_t i=0; i<COLS; i++) {
		field[current][i] = preseed[i_preseed][i];
	}
}

void init(void) {
	for (int i=0; i<COLS; i++) {
		set_output(DDRD, COL_PIN[i]);
		output_high(PORTD, COL_PIN[i]);
	}
	for (int i=0; i<ROWS; i++) {
		set_output(DDRB, ROW_PIN[i]);
		output_low(PORTB, ROW_PIN[i]);
	}
	set_input(DDRA, PA0);
	set_input(DDRA, PA1);
	seed();
	OCR1A = 2;
	TCCR1A = 0x00;
	// WGM1=4, prescale at 1024
	TCCR1B = (0 << WGM13)|(1 << WGM12)|(1 << CS12)|(0 << CS11)|(1 << CS10);
	//Set bit 6 in TIMSK to enable Timer 1 compare interrupt
	TIMSK |= (1 << OCIE1A);
	sei();
}

uint8_t neighbours(int i, int j) {
        uint8_t n = 0;
        for (int8_t di=-1; di<=+1; di++) {
                for (int8_t dj=-1; dj<=+1; dj++) {
                        if (di == 0 && dj == 0) continue;
#if 0
			if (i+di >= COLS || i+di < 0 || j+dj >= ROWS || j+dj < 0) {
				continue;
			} else {
				if (alive(i+di, j+dj)) n++;
			}
#else
			if (alive( mod(i+di, COLS), mod(j+dj, ROWS) ) ) {
				n++;
			}
#endif
                }
        }
        return n;
}

int update_field(void) {
	uint8_t next = 1-current;
        int changes = 0;

	for (int i=0; i<COLS; i++) {
		for (int j=0; j<ROWS; j++) {
			// calculate number of neihgbours
			uint8_t n = neighbours(i,j);
			// a living cell dies if it has less than 2 neighbours
			if (n<2 && alive(i,j)) {
				set_dead(field[next], i, j);
				changes++;
			// a living cell dies if it has more than 3 neighbours
			} else if (n>3 && alive(i,j)) {
				set_dead(field[next], i, j);
				changes++;
			// a dead cell comes alive if it has exactly 3 neighbours
			} else if (n == 3 && ! alive(i,j)) {
				set_alive(field[next], i, j);
				changes++;
			} else {
				if (alive(i,j)) {
					set_alive(field[next], i, j);
				} else {
					set_dead(field[next], i, j);
				}
			}
		}
        }
	current = next;
        return changes;
}

volatile uint8_t active_col = 0;
void draw_screen(void) {
	output_low(PORTD,COL_PIN[ mod(active_col-1, COLS) ]);
	for (int x=0; x<ROWS; x++) {
		if (alive(active_col,x)) {
			output_low(PORTB, ROW_PIN[x]);
		} else {
			output_high(PORTB, ROW_PIN[x]);
		}
	}
	output_high(PORTD,COL_PIN[active_col]);
	active_col = (active_col+1)%COLS;
}

SIGNAL(SIG_TIMER1_COMPA) {
	draw_screen();
}

int main(void) {
	init();

	while(1) {
		uint8_t changes = 0;
		if (PINA & (1<<PA0)) {
			i_preseed = mod(i_preseed+1, N_PRESEEDS);
		} else {
			changes = update_field();
		}
		if (changes == 0) {
			seed();
		}
		_delay_ms(500);
	}
	return 0;
}
