#include <p30fxxxx.h>
#include "uart1.h"

void initUART1(void)
{
	// U1BRG registar se koristi za odredjivanje baudrate-a pri serijskoj komunikaciji
	// U1BRG = [ (10 000 000 / 3) / (16 * baudrate) ] - 1
    U1BRG = 0x0040; // BAUDRATE = 9600
	
	// U1MODEbits.ALTIO registar se koristi za biranje alternativnog porta za serijsku komunikaciju
	// Ima smisla samo kod UART1, UART2 nema alternativni port
	// UART1 - RF2, RF3
	// UART1 alt - RC13, RC14
	// UART2 - RF4, RF5
    U1MODEbits.ALTIO = 0; // koriste se RF2 i RF3
	
	// IEC0bits.U1RXIE registar omogucava prekid (interupt) pri prijemu podataka
    IEC0bits.U1RXIE = 1; // omogucavamo rx1 interupt
	
	// Na ovaj nacin resetujemo donja 2 (LSB) bita na 0 pri inicijalizaciji
	// Ta dva bita su bit flag-a i bit greske pri slanju podataka
    U1STA &= 0xfffc; // inicijalno resetujemo bit za gresku i flag
	
	// U1MODEbits.UARTEN registar ukljucuje UART1 modul
    U1MODEbits.UARTEN = 1; // ukljucujemo ovaj modul
	
	// U1STAbits.UTXEN registar omogucava slanje podataka preko UART1
    U1STAbits.UTXEN = 1; // ukljucujemo predaju
}



void WriteUART1(unsigned int data)
{
	// While ceka sve dok predajni registar ne postane slobodan
	while(!U1STAbits.TRMT);
	
    if(U1MODEbits.PDSEL == 3)	// Provjera da li su podaci 9-bitni
        U1TXREG = data;
    else
        U1TXREG = data & 0xFF;
}



void RS232_putst(register const char *str)
{
	while((*str) != 0)
	{
		WriteUART1(*str);
		if(*str == 13) WriteUART1(10);
		if(*str == 10) WriteUART1(13);
		str++;
	}
}


void WriteUART1dec2string(unsigned int data) 
{
	unsigned char temp;

	temp = data/1000;
	WriteUART1(temp+'0');
	data = data - temp*1000;
	temp = data/100;
	WriteUART1(temp+'0');
	data = data - temp*100;
	temp = data/10;
	WriteUART1(temp+'0');
	data = data - temp*10;
	WriteUART1(data+'0');
}