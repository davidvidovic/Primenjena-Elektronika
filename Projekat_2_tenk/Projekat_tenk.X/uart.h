#include <p30fxxxx.h>

// UART1 se u ovom projektu koristi za Debugging

// Funkcija za inicijalizaciju UART1
void InitUART1(void);

// Funkcija za ispis karaktera preko UART1
void WriteUART1(unsigned int data);

// Funkcija za slanje stringa preko UART1
void print_DEBUG(register const char *str);

// Funkcija za slanje 4-cifrenog broja preko UART1
void WriteUART1dec2string(unsigned int data);


// UART2 se u ovom projektu koristi za Bluetooth komunkaciju

// Funkcija za inicijalizaciju UART1
void InitUART2(void);

// Funkcija za ispis karaktera preko UART1
void WriteUART2(unsigned int data);

// Funkcija za slanje stringa preko UART1
void print_BLE(register const char *str);

// Funkcija za slanje 4-cifrenog broja preko UART1
void WriteUART2dec2string(unsigned int data);
