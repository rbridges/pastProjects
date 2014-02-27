RECIEVER MAIN


#include "lm3s1968.h"
#include "ADC.h"
#include "SysTick8.h"
#include "PLL.h"
#include "LCD.h"
#include "FIFOheaderlab9.h"
#include "UARTrecieve.h"


int datmail;
extern int sample;
extern int ADCmail;
int oldmail;
void UART_Init(void);
extern int flag;
extern int getmail;


//debug code
//
// This program periodically samples ADC channel 0 and stores the
// result to a global variable that can be accessed with the JTAG
// debugger and viewed with the variable watch feature.
#define PG2   (*((volatile unsigned long *)0x40026010))
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode


extern int Fifo[16];
extern int *fifopt;
extern int *frontpt;
extern int *backpt;
extern int fifocount;
extern int getmail;
extern int errorcount;


void ADC_InitSWTriggerSeq3(unsigned char channelNum){
  // channelNum must be 0-3 (inclusive) corresponding to ADC0 through ADC3
  if(channelNum > 3){
    return;                                 // invalid input, do nothing
  }
  // **** general initialization ****
  SYSCTL_RCGC0_R |= SYSCTL_RCGC0_ADC;       // activate ADC
  SYSCTL_RCGC0_R &= ~SYSCTL_RCGC0_ADCSPD_M; // clear ADC sample speed field
  SYSCTL_RCGC0_R += SYSCTL_RCGC0_ADCSPD125K;// configure for 125K ADC max sample rate (default setting)
  // **** ADC initialization ****
                                            // sequencer 0 is highest priority (default setting)
                                            // sequencer 1 is second-highest priority (default setting)
                                            // sequencer 2 is third-highest priority (default setting)
                                            // sequencer 3 is lowest priority (default setting)
  ADC0_SSPRI_R = (ADC_SSPRI_SS0_1ST|ADC_SSPRI_SS1_2ND|ADC_SSPRI_SS2_3RD|ADC_SSPRI_SS3_4TH);
  ADC_ACTSS_R &= ~ADC_ACTSS_ASEN3;          // disable sample sequencer 3
  ADC0_EMUX_R &= ~ADC_EMUX_EM3_M;           // clear SS3 trigger select field
  ADC0_EMUX_R += ADC_EMUX_EM3_PROCESSOR;    // configure for software trigger event (default setting)
  ADC0_SSMUX3_R &= ~ADC_SSMUX3_MUX0_M;      // clear SS3 1st sample input select field
                                            // configure for 'channelNum' as first sample input
  ADC0_SSMUX3_R += (channelNum<<ADC_SSMUX3_MUX0_S);
  ADC0_SSCTL3_R = (0                        // settings for 1st sample:
                   & ~ADC_SSCTL3_TS0        // read pin specified by ADC0_SSMUX3_R (default setting)
                   | ADC_SSCTL3_IE0         // raw interrupt asserted here
                   | ADC_SSCTL3_END0        // sample is end of sequence (default setting, hardwired)
                   & ~ADC_SSCTL3_D0);       // differential mode not used (default setting)
  ADC0_IM_R &= ~ADC_IM_MASK3;               // disable SS3 interrupts (default setting)
  ADC_ACTSS_R |= ADC_ACTSS_ASEN3;           // enable sample sequencer 3
}




 int digconvert =0;
 int n;
 int sum;
unsigned char* cm = " cm";		// hardfault register - 0xE000ED28





 int Convert (int digconvert_)				//   y = ax + b  ... a = .001833   ;  b=-.12079
 
{
		double result=0;
		result = (digconvert_)*(1833000) - (120790000) ;
	if(result <= 0)
	{ return 0;}
		return result/1000000;
}

int main(void){
	int useless =0;
	int a,b,c;
	
	DisableInterrupts(); 
	PLL_Init();      // 50 MHz I think?
	LCD_Open();
	ADC_InitSWTriggerSeq3(3);
	SysTick_Init();
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOG; // activate port G
	useless = 1;
	useless =0;
  GPIO_PORTG_DIR_R |= 0x04;             // make PG2 out (built-in LED)
  GPIO_PORTG_AFSEL_R &= ~0x04;          // disable alt func on PG2
  GPIO_PORTG_DEN_R |= 0x04;             // enable digital I/O on PG2
  PG2 = 0;                              // turn off LED
	UART_Init();
	ADC_Init();
	LCD_Clear();
	EnableInterrupts();
	
	


while(1)
{
	

	if((FIFO_Get() !=1) && getmail==0x2)
	{
		
		
	int i;
	int sendme;
		flag =0;
		//FIFO_Get();
		LCD_GoTo(0);
	for(i = 0; i <5; i++)
		{
			FIFO_Get();
			sendme = getmail;
			LCD_OutChar( sendme);
		}
		LCD_GoTo(20);
		LCD_OutChar(99);
		LCD_OutChar(109);
		FIFO_Get();
		FIFO_Get();	// these two gets just take the blank space and the Stop 0x03 off the FIFO
		
	}	
}
	
}


SYSTICK HANDLER
	

void SysTick_Handler()
{
	  GPIO_PORTG_DATA_R ^= 0x4;
		sample = ADC_In();
		GPIO_PORTG_DATA_R ^= 0x4;
		sample = Convert(sample);
		thousands = sample%1000;
		hundreds = (sample%100)-(thousands*10);
		tens = (sample%10) - ( (hundreds*10) + (thousands*100) );
		ones = ( sample - (tens*10 + hundreds*100 + thousands*1000 ) );
	
	
		UART_OutChar(0x2);									// STx
		UART_OutChar((thousands)+0x30);		// first number ASCII
	  UART_OutChar(0x2E);									// dot ASCII
		UART_OutChar( (hundreds) + 0x30);		// second number ASCII
	  UART_OutChar( tens +0x30);					// third
		UART_OutChar(ones +0x30);					// last number ASCII
		UART_OutChar(0x0D);								// CR, whatever that is
		UART_OutChar(0x3);			 					// ETx
	
		samplecount++;
	
		GPIO_PORTG_DATA_R ^= 0x4;
		
}












UART 

#include "lm3s1968.h"
#include "ADClab9.h"
#include "FIFOheaderlab9.h"
//#include "SysTickInts.h"


int errorcount;
int flag;


void UART_Init(void){
// 	int i;
//   SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART1; // activate UART1
// 	i=i;
//   SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD; // activate port D
// 	i = i;
//   UART1_CTL_R &= ~0x1;      // disable UART
// 	i=i;
//   UART1_IBRD_R = 31;                    // IBRD = int(50,000,000 / (16 * 100000)) = int(31.25)
//   UART1_FBRD_R = 16;                     // FBRD = int(0.1267 * 64 + 0.5) = 16.5
//                                         // 8 bit word length (no parity bits, one stop bit, FIFOs)
// 	//UART1_CTL_R &= UART_CTL_UARTEN;	
	// I put this in. Reenable UART?
//   UART1_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
//   UART1_CTL_R |= UART_CTL_UARTEN;       // enable UART
// 	i=i;
// 	UART1_LCRH_R |= 0x0070;
// 	UART1_CTL_R |= 0x0301;		// enables RXE, TXE, and UART
//   GPIO_PORTD_AFSEL_R |= 0x03;           // enable alt funct on PD1-0
//   GPIO_PORTD_DEN_R |= 0x03;             // enable digital I/O on PD1-0
	int i;

	SYSCTL_RCGC1_R |= 0x2;  //activate UART1
	i=0;
	i=1;
	SYSCTL_RCGC2_R |= 0x08;	//activate PortD
	i=0;
	i=1;

	UART1_CTL_R &= ~0x01;	//disable UART
	UART1_IBRD_R = 31; //15.625    31
	UART1_FBRD_R = 6;  //0.625*64 = 40      16
	UART1_LCRH_R = 0x0070;
	UART1_CTL_R = 0x0301;		// enables RXE, TXE, and UART
	UART1_IM_R |= 0x10;		// something from slides I'm not sure the effect of
	NVIC_EN0_R |= 0x40; 		// enable "interrupt 6"
	
	UART1_IFLS_R = (UART1_IFLS_R & ~0x38)|0x12;  // enables interrupts for 1/2 full, etc?
	GPIO_PORTD_AFSEL_R |= 0x0C;	//enable alt function on PD2,3
	GPIO_PORTD_DEN_R |= 0x0C;	//enable digital I/O on PD2,3
	
		FIFO_Init();
		errorcount =0;
	
}

void UART_OutChar(int stuff)
{
	
	while( (UART1_FR_R&0x0020) != 0){     }
		UART1_DR_R = stuff;

}

// void UART1_Init(void)
// {
// 	SYSCTL_RCGC1_R |= 0x1;  //activate UART1
// 	SYSCTL_RCGC2_R |= 0x08;	//activate PortD
// 	UART1_CTL_R &= ~0x02;	//disable UART
// 	UART1_IBRD_R = 31; //15.625
// 	UART1_FBRD_R = 16;  //0.625*64 = 40
// 	UART1_LCRH_R = 0x0070;
// 	UART1_CTL_R = 0x0302;		// enables RXE, TXE, and UART
// 	
// 		NVIC_PRI1_R |= 0xE00000;; // off of slide 20, set bits 21-23
// 	UART1_IM_R |= 0x10;		// something from slides I'm not sure the effect of
// 	NVIC_EN0_R |= 0x20; 		// enable "interrupt 6"
// 	UART1_IFLS_R = (UART1_IFLS_R & ~0x38)|0x10;  // enables interrupts for 1/2 full, etc?
// 	
// 	GPIO_PORTD_AFSEL_R |= 0x0C;	//enable alt function on PD2,3
// 	GPIO_PORTD_DEN_R |= 0x0C;	//enable digital I/O on PD2,3
// 	counter =0;
// }

unsigned char UART_InChar(void)
{
while((UART1_FR_R&0x001) != 0);  //wait unitl RXFE is 0
return ((unsigned char)(UART1_DR_R&0xFF));
}

void UART1_Handler()
{
	int i;
	GPIO_PORTG_DATA_R ^= 0x2;
	GPIO_PORTG_DATA_R ^= 0x2;
	while(UART1_FR_R& (0x8))
	{}				//while RXFE is 1, do nothing
		
		for( i =0; i<8; i++)
		{
			if (FIFO_Put(UART1_DR_R) == 1)
			{ errorcount++;}
		}
		UART1_ICR_R = 0x10;			// acknowledges the interrupt happened
		flag =1;
		
	
	
}





FIFO 


int Fifo[16];
int *fifopt;
int *frontpt;
int *backpt;
int fifocount;
int getmail;

void FIFO_Init()
{
	fifopt = &Fifo[0];
	frontpt = &Fifo[0];
	backpt = &Fifo[0];
	fifocount = 0;
	getmail =0;
}


 int FIFO_Put(int datum)
{
	
	if (fifocount>=16)
	{return 1;}				// if FIFO is full, you're trying to put onto a full fifo! return error!

	
	
	if(frontpt <= fifopt+15)
	{
		
		frontpt++;
		*frontpt = datum;
	}
	else
	{
		frontpt = fifopt;
		*frontpt = datum;
	}
	
	fifocount++;
	return 0;
}

int FIFO_Get()
{
	if(fifocount == 0)
		{ return 1; } // fifo empty, and trying to get, give error 

	if(backpt <= fifopt+15)
	{
		getmail = *backpt;
		backpt++;
	}
	else
	{
		getmail = *backpt;
		backpt = fifopt;
	}
	
	fifocount--;
	return 0;
		


}
