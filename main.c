#pragma config MCLRE=ON            // Master Clear desabilitado
#pragma config FOSC=INTOSC_XT      // Oscilador c/ cristal externo HS
#pragma config WDT=OFF             // Watchdog controlado por software
#pragma config LVP=OFF             // Sem programação em baixa tensão
#pragma config DEBUG=OFF           // Desabilita debug
#pragma config XINST=OFF


//funções de bit
#define BitSet(arg,bit) ((arg) |= (1<<bit))
#define BitClr(arg,bit) ((arg) &= ~(1<<bit)) 
#define BitFlp(arg,bit) ((arg) ^= (1<<bit)) 
#define BitTst(arg,bit) ((arg) & (1<<bit)) 


//defines para registros especiais
#define PORTC	(*(volatile __near unsigned char*)0xF82)
#define PORTD	(*(volatile __near unsigned char*)0xF83)

#define TRISC	(*(volatile __near unsigned char*)0xF94)
#define TRISD	(*(volatile __near unsigned char*)0xF95)

#define TRISA	(*(volatile __near unsigned char*)0xF92)
#define ADCON2	(*(volatile __near unsigned char*)0xFC0)
#define ADCON1	(*(volatile __near unsigned char*)0xFC1)
#define ADCON0	(*(volatile __near unsigned char*)0xFC2)
//#define ADRESL	(*(volatile __near unsigned char*)0xFC3)
#define ADRESH	(*(volatile __near unsigned char*)0xFC4)


//also disp1
#define RS  6
#define EN  7

void delayMS(unsigned char ms) {
    unsigned char t;
    for (;ms>0; ms--){
        for(t=0;t<30;t++);
    }
}

//pulse the Enable pin high (for a microsecond).
//This clocks whatever command or data is in DB4~7 into the LCD controller.

void pulseEnablePin() {
    BitSet(PORTC, EN);
    BitClr(PORTC, EN);
}

//push a nibble of data through the the LCD's DB4~7 pins, clocking with the Enable pin.
//We don't care what RS and RW are, here.

void pushNibble(int value, int rs) {
    PORTD = value;
    if (rs) {
        BitSet(PORTC, RS);
    } else {
        BitClr(PORTC, RS);
    }
    pulseEnablePin();
}

//push a byte of data through the LCD's DB4~7 pins, in two steps, clocking each with the enable pin.

void pushByte(int value, int rs) {
    int val_lower = (value << 4) &0xf0;
    int val_upper = value & 0xF0;
    pushNibble(val_upper, rs);
    pushNibble(val_lower, rs);
}

void lcdCommand4bits(int nibble) {
    pushNibble(nibble<<4, 0);
}

void lcdCommand8bits(int value) {
    pushByte(value, 0);
    delayMS(1);
}

//print the given character at the current cursor position. overwrites, doesn't insert.

void lcdChar(int value) {
    //let pushByte worry about the intricacies of Enable, nibble order.
    pushByte(value, 1);
}


//print the given string to the LCD at the current cursor position.  overwrites, doesn't insert.
//While I don't understand why this was named printIn (PRINT IN?) in the original LiquidCrystal library, I've preserved it here to maintain the interchangeability of the two libraries.

void lcdString(char msg[]) {
    unsigned char i = 0; //fancy int.  avoids compiler warning when comparing i with strlen()'s uint8_t
    while (msg[i]) {
        lcdChar(msg[i]);
        i++;
    }
}

void lcdDefconLogo(void) {
    int i;
    unsigned char defcon[] = {
        0x0, 0x1, 0x3, 0x3, 0x3, 0x3, 0x1, 0x4,
        0xe, 0x1f, 0x4, 0x4, 0x1f, 0xe, 0x11, 0x1f,
        0x0, 0x10, 0x18, 0x18, 0x18, 0x18, 0x10, 0x4,
        0xc, 0x3, 0x0, 0x0, 0x0, 0x3, 0xc, 0x4,
        0x0, 0x0, 0x1b, 0x4, 0x1b, 0x0, 0x0, 0x0,
        0x6, 0x18, 0x0, 0x0, 0x0, 0x18, 0x6, 0x2
    };
    unsigned char USP[] = {
        0x1f,0x12,0x12,0x12,0x12,0x12,0x10,0x0f,
        0x17,0x08,0x08,0x04,0x02,0x01,0x80,0x17,
        0x1d,0x02,0x10,0x08,0x04,0x02,0x02,0x1d,
        0x17,0x01,0x09,0x09,0x09,0x0e,0x80,0x18
    };
    lcdCommand8bits(0x40);
    for (i = 0; i < 8 * 6; i++) {
        lcdChar(~defcon[i]);
    }

}

// initiatize lcd after a short pause
//while there are hard-coded details here of lines, cursor and blink settings, you can override these original settings after calling .init()

void lcdInit() {
    BitClr(TRISC, EN);
    BitClr(TRISC, RS);
    TRISD &= 0x0f;

    delayMS(15);//15ms

    //The first 4 nibbles and timings are not in my DEM16217 SYH datasheet, but apparently are HD44780 standard...
    lcdCommand4bits(0x03);
    delayMS(1);
    lcdCommand4bits(0x03);
    delayMS(1);//100us
    lcdCommand4bits(0x03);
    delayMS(5);

    // needed by the LCDs controller
    //this being 2 sets up 4-bit mode.
    lcdCommand4bits(0x02);
    delayMS(1);

    //configura o display
    lcdCommand8bits(0x28); //8bits, 2 linhas, 5x8
    lcdCommand8bits(0x06); //modo incremental
    lcdCommand8bits(0x0c); //display e cursor on, com blink
    lcdCommand8bits(0x03); //zera tudo
    lcdCommand8bits(0x80); //posição inicial
    lcdCommand8bits(0x01); //limpar display
}

void adInit(void) {
    BitSet(TRISA, 0); //pin setup
    ADCON0 = 0b00000001; //channel select AN0, stop, ADC on
    ADCON1 = 0b00001110; //ref = source, AN0 is analog
    ADCON2 = 0b00001001; //left justified, t_conv = 2 TAD, f_ad = f_osc/2
}

unsigned int adRead(void) {
    unsigned int ADvalue;
    BitSet(ADCON0, 1); //start conversion
    while (BitTst(ADCON0, 1)); //wait
    return ADRESH; //return result
}
char *lkt = "0123456789ABCDEF";
void main(void) {
    unsigned char i;
    unsigned char ad;
    
    adInit();

    lcdInit();
    lcdDefconLogo();
    lcdCommand8bits(0x80);
    lcdChar(0);
    lcdChar(1);
    lcdChar(2);
    lcdString(" Defcon");
    lcdCommand8bits(0xC0);
    lcdChar(3);
    lcdChar(4);
    lcdChar(5);
    lcdString("mBed workshop");
    adInit();
    TRISD = 0x00;
    for (;;) {
        ad = adRead();
        lcdCommand8bits(0x8B);
        //lcdChar(lkt[ad>>4]);
        //lcdChar(lkt[ad&0x0f]);
        lcdChar((ad / 100) % 10 + 48);
        lcdChar((ad / 10) % 10 + 48);
        lcdChar((ad / 1) % 10 + 48);
        
        
        for (i = 0; i < ad; i++){
            PORTD = 0xff;
        }
        
        for (i = ad; i < 255;i ++){
            PORTD = 0x00;
        }
    }
}
