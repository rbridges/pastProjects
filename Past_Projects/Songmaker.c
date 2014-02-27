#include "SysTick.h"
#include "lm3s1968.h"


const int size = 67;
int tempo = 120000;

typedef struct
{
int tone;
double duration;
int staccato;
}sound;

//The Tetris Song(tm)
sound music[size] = { {0,20,0}, {-5, 20,1}, {-4,20, 1}, {-2,10,0}, {0,10,1}, {-2,10,1}, {-4,10,0}, {-5,10,0}, {-7,20,0}, {-7,10,0}, 
										{-4,10,0}, {0,20,0}, {-2, 20,1}, {-4,20, 1}, {-5,20,0}, {-4,20, 0}, {-2,20, 0}, {0,20,0},{-4,20,0},{-7,20,0}, 
										{-7,30,0}, {100,10,0}, {-2, 20,0}, {-2, 20,1}, {1, 20,1}, {5, 20,0}, {3, 20,1}, {1, 20,1}, {0, 30,0 }, {-4, 20,1}, {0, 20,0 }, 
										{-2, 20,1 }, {-4, 20,1 }, {-5, 20,0 }, {-4, 20,0 }, {-2, 20,0 }, {0, 20,0 }, {-4, 20,0 }, {-7, 20,0 },
										{-7, 30,0 }, {100, 20, 0}, {0, 40,0}, {-4, 40,0}, {-2, 40,0}, {-5, 40,0}, {-4, 40,0}, {-7, 40,0}, {-8, 40,0}, {-5, 20, 0},
										{100, 20, 0}, {0, 40,0}, {-4, 40,0}, {-2, 40,0}, {-5, 40,0}, {-4, 20,0}, {0, 20,0}, {5, 40,0}, {4, 50,0}, {100, 45, 0}  };
										

void Delay(double duration)	
{
	int d;
	for(d = duration*tempo; d != 0; d--)
		{
			d = d;
		}
}

int ConvertNote (int changeme)
{
	switch (changeme)
	{
	case 0:
		changeme = 5972;
		break; 
	case -8:
		changeme = 9481;
		break;
	case -7:
		changeme = 8949;
		break;
	case -6:
		changeme = 8446;
		break;
	case -5:
		changeme = 7972;
		break;
	case -4:
		changeme = 7525;
		break;
	case -3:
		changeme = 7102;
		break;
	case -2:
		changeme = 6704;
		break;
	case -1:
		changeme = 6327;
		break;
	case 1:
		changeme =  5637;
		break;
	case 3:
		changeme = 5022;
		break;
	case 4:
		changeme = 4740;
		break;
	case 5:
		changeme = 4474;
		break;
	case 100:
		NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC; // disable interrupts for some silence
		break;
	default:
		changeme = 0xFFFFFF;
	}
		
	return changeme;
}


void PlayNote(int note, double duration, int staccato)
{
	if ( ((GPIO_PORTG_DATA_R&0x8)==8) && (tempo > 10000) ) // that's a button
	{
		tempo -= 10000;
	}
	
	if (staccato == 0)
	{
	NVIC_ST_RELOAD_R = note;
	Delay(duration);
	}
	
	if (((GPIO_PORTG_DATA_R&64)==64) && (tempo < 500000) )
	{
		tempo += 10000;
	}
	
	else
		NVIC_ST_RELOAD_R = note;
		Delay(duration*.85);
		NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC; // disable interrupts for some silence
		Delay(duration*.15);
		NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC+NVIC_ST_CTRL_INTEN; // reenable ints because we neeeeeed theeeeemmm
	
		if(note == 100)
			NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC+NVIC_ST_CTRL_INTEN;
}


int PlaySong(void)
{
	int x;
	for( x = 0; x < size; x++)
		{
			int note = music[x].tone;
			double duration = music[x].duration;
			int staccato = music[x].staccato;
			note = ConvertNote(note);
			 if ( (GPIO_PORTG_DATA_R&0x10) == 0x10)
			 {
				 return 0;
			 }
			PlayNote(note, duration,staccato);
			duration=0;
			
		}
		PlaySong();
	
}












	
