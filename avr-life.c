#define F_CPU 1200000UL
#include <avr/io.h>
#include <util/delay.h>

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

uint8_t preseed[COLS] = {
	0b00100000,
	0b00010000,
	0b01110000,
	0b00000000,
	0b00000000,
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

void init(void) {
	for (int i=0; i<COLS; i++) {
		set_output(DDRD, COL_PIN[i]);
		output_high(PORTD, COL_PIN[i]);
	}
	for (int i=0; i<ROWS; i++) {
		set_output(DDRB, ROW_PIN[i]);
		output_low(PORTB, ROW_PIN[i]);
	}
}

void seed(void) {
	for (int8_t i=0; i<COLS; i++) {
		field[current][i] = preseed[i];
	}
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

int main(void) {
	init();
	seed();
	int n = 0;
	uint8_t active_col = 0;

	while(1) {
		for (int x=0; x<ROWS; x++) {
			if (alive(active_col,x)) {
				output_low(PORTB, ROW_PIN[x]);
			} else {
				output_high(PORTB, ROW_PIN[x]);
			}
		}
		output_high(PORTD,COL_PIN[active_col]);
		n++;
		if (n > 150) {
			uint8_t changes = update_field();
			n = 0;
			if (changes == 0) {
				seed();
			}
		} else {
			_delay_ms(2);
		}
		output_low(PORTD,COL_PIN[active_col]);
		active_col = (active_col+1)%COLS;
	}
	return 0;
}
