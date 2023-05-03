#include <p30fxxxx.h>
#include "uart2.h"

// UART1 se u ovom projektu koristi za Bluetooth komunikaciju

void initUART2(void)
{
	// U2BRG registar se koristi za odredjivanje baudrate-a pri serijskoj komunikaciji
	// U2BRG = [ (10 000 000) / (16 * baudrate) ] - 1
    U2BRG = 0x0040; // BAUDRATE = 9600
	
	// U1MODEbits.ALTIO registar se koristi za biranje alternativnog porta za serijsku komunikaciju
	// Ima smisla samo kod UART1, UART2 nema alternativni port
	// UART1 - RF2, RF3
	// UART1 alt - RC13, RC14
	// UART2 - RF4, RF5
    //U1MODEbits.ALTIO = 0; // koriste se RF2 i RF3
	
	// IEC0bits.U2RXIE registar omogucava prekid (interupt) pri prijemu podataka
    IEC1bits.U2RXIE = 1; // omogucavamo rx1 interupt
	
	// Na ovaj nacin resetujemo donja 2 (LSB) bita na 0 pri inicijalizaciji
	// Ta dva bita su bit flag-a i bit greske pri slanju podataka
    U2STA &= 0xfffc; // inicijalno resetujemo bit za gresku i flag
	
	// U2MODEbits.UARTEN registar ukljucuje UART2 modul
    U2MODEbits.UARTEN = 1; // ukljucujemo ovaj modul
	
	// U2STAbits.UTXEN registar omogucava slanje podataka preko UART2
    U2STAbits.UTXEN = 1; // ukljucujemo predaju
}



void WriteUART2(unsigned int data)
{
	// While ceka sve dok predajni registar ne postane slobodan
	while(!U2STAbits.TRMT);
	
    if(U2MODEbits.PDSEL == 3)	// Provjera da li su podaci 9-bitni
        U2TXREG = data;
    else
        U2TXREG = data & 0xFF;
}



void print_DEBUG(register const char *str)
{
	while((*str) != 0)
	{
		WriteUART2(*str);
		if(*str == 13) WriteUART2(10);
		if(*str == 10) WriteUART2(13);
		str++;
	}
}


void WriteUART2dec2string(unsigned int data) 
{
	unsigned char temp;

	temp = data/1000;
	WriteUART2(temp+'0');
	data = data - temp*1000;
	temp = data/100;
	WriteUART2(temp+'0');
	data = data - temp*100;
	temp = data/10;
	WriteUART2(temp+'0');
	data = data - temp*10;
	WriteUART2(data+'0');
}