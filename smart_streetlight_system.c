#include <lpc214x.h>
#include <stdio.h>

/************************************************
   LCD CONNECTIONS (8-bit mode)
   RS -> P0.0
   RW -> P0.1
   EN -> P0.2
   D0-D7 -> P0.8 - P0.15
************************************************/

/************************************************
   LED CONNECTIONS
   LEDs connected to P1.16 - P1.23
************************************************/

/************************************************
   KEYPAD CONNECTIONS (4x4 Matrix)
   ROWS -> P0.16 - P0.19 (output)
   COLS -> P0.20 - P0.23 (input, pull-up)
************************************************/

/************************************************
   FUNCTION PROTOTYPES
************************************************/
void InitLCD(void);
void cmdLCD(unsigned char);
void charLCD(unsigned char);
void strLCD(char *);
void delay_ms(unsigned int);

void RTC_Init(void);
void RTC_read(void);
void display_time_date(void);
void check_light_and_control_leds(void);
unsigned int adc_read(unsigned char channel);

char keypad_getkey(void);
unsigned int get_number_from_keypad(unsigned int digits);
void edit_rtc_menu(void);
void edit_rtc_with_keypad(unsigned char field);
unsigned char isLeapYear(unsigned int year);
unsigned char daysInMonth(unsigned int month, unsigned int year);

void EINT1_ISR(void) __irq;

/************************************************
   GLOBAL VARIABLES
************************************************/
unsigned int hour, minute, second, date, month, year, dow;
unsigned int ldr_value = 0;
unsigned int LDR_THRESHOLD = 200;   // ADC threshold for darkness

const char *days[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

/************************************************
   MAIN FUNCTION
************************************************/
int main(void)
{
    char choice; // must declare at top

    unsigned int temp; // for any temporary use

    // LCD pins as output
    IODIR0 |= 0x0000FF07;   // P0.0, P0.1, P0.2, P0.8-P0.15 as output
    // LED pins as output
    IODIR1 |= 0x00FF0000;   // P1.16 - P1.23 as output
    // Keypad pins
    IODIR0 |= 0x000F0000;   // P0.16-P0.19 as output (rows)
    IODIR0 &= ~(0x00F00000);// P0.20-P0.23 as input (cols)

    // Configure EINT1 for RTC edit switch
    PINSEL0 |= (1<<29);     // P0.14 as EINT1
    EXTMODE = (1<<1);       // EINT1 edge-sensitive
    EXTPOLAR &= ~(1<<1);    // Falling edge
    VICIntEnable = (1<<15); // Enable EINT1 interrupt
    VICVectCntl0 = 0x20 | 15;
    VICVectAddr0 = (unsigned long)EINT1_ISR;

    // Init LCD and RTC
    InitLCD();
    RTC_Init();

    // Startup menu
    while(1)
    {
        cmdLCD(0x01);
        strLCD("1.EDIT RTC INFO");
        cmdLCD(0xC0);
        strLCD("2.EXIT");

        choice = keypad_getkey();

        if(choice == '1')
            edit_rtc_menu();
        else if(choice == '2')
            break;
    }

    cmdLCD(0x01);
    strLCD("SMART STREETLIGHT");
    delay_ms(2000);
    cmdLCD(0x01);

    while(1)
    {
        RTC_read();
        display_time_date();
        check_light_and_control_leds();
        delay_ms(1000);
    }
}

/************************************************
   LCD FUNCTIONS
************************************************/
void InitLCD(void)
{
    cmdLCD(0x38);  // 8-bit, 2 line
    cmdLCD(0x0C);  // Display ON, Cursor OFF
    cmdLCD(0x06);  // Auto increment
    cmdLCD(0x01);  // Clear
    delay_ms(2);
}

void cmdLCD(unsigned char cmd)
{
    IOPIN0 = (IOPIN0 & 0xFFFF00FF) | (cmd << 8);
    IOCLR0 = (1<<0);  // RS=0
    IOCLR0 = (1<<1);  // RW=0
    IOSET0 = (1<<2);  // EN=1
    delay_ms(2);
    IOCLR0 = (1<<2);  // EN=0
}

void charLCD(unsigned char data)
{
    IOPIN0 = (IOPIN0 & 0xFFFF00FF) | (data << 8);
    IOSET0 = (1<<0);  // RS=1
    IOCLR0 = (1<<1);  // RW=0
    IOSET0 = (1<<2);  // EN=1
    delay_ms(2);
    IOCLR0 = (1<<2);  // EN=0
}

void strLCD(char *str)
{
    while(*str) charLCD(*str++);
}

/************************************************
   RTC FUNCTIONS
************************************************/
void RTC_Init(void)
{
    CCR = 0x11;   // Enable RTC, reset clock
    HOUR = 0; MIN = 0; SEC = 0;
    DOM = 1; MONTH = 1; YEAR = 2025; DOW = 0;
}

void RTC_read(void)
{
    hour = HOUR;
    minute = MIN;
    second = SEC;
    date = DOM;
    month = MONTH;
    year = YEAR;
    dow = DOW;
}

void display_time_date(void)
{
    char buf[20];
    cmdLCD(0x80);
    sprintf(buf,"%02d:%02d:%02d %s",hour,minute,second,days[dow]);
    strLCD(buf);
    cmdLCD(0xC0);
    sprintf(buf,"%02d/%02d/%04d",date,month,year);
    strLCD(buf);
}

/************************************************
   LDR + LED CONTROL
************************************************/
void check_light_and_control_leds(void)
{
    if(hour >= 18 || hour < 6)
    {
        ldr_value = adc_read(1);
        if(ldr_value < LDR_THRESHOLD)
            IOSET1 = 0x00FF0000;  // LEDs ON
        else
            IOCLR1 = 0x00FF0000;  // LEDs OFF
    }
    else
        IOCLR1 = 0x00FF0000;      // LEDs OFF in daytime
}

/************************************************
   ADC FUNCTION (for LDR)
************************************************/
unsigned int adc_read(unsigned char channel)
{
    unsigned int result;
    PINSEL1 |= (1<<24);        // P0.28 as AD0.1
    AD0CR = (1<<channel) | (4<<8) | (1<<21);
    AD0CR |= (1<<24);
    while((AD0GDR & (1U<<31)) == 0);
    result = (AD0GDR >> 6) & 0x3FF;
    return result;
}

/************************************************
   KEYPAD FUNCTIONS
************************************************/
char keypad_getkey(void)
{
    const char keypad[4][4]={{'1','2','3','A'},
                             {'4','5','6','B'},
                             {'7','8','9','C'},
                             {'*','0','#','D'}};
    int row,col;

    while(1)
    {
        for(row=0;row<4;row++)
        {
            IOSET0 = 0x000F0000;
            IOCLR0 = (1<<(16+row));
            for(col=0;col<4;col++)
            {
                if(!(IOPIN0 & (1<<(20+col))))
                {
                    delay_ms(20);
                    while(!(IOPIN0 & (1<<(20+col))));
                    return keypad[row][col];
                }
            }
        }
    }
}

unsigned int get_number_from_keypad(unsigned int digits)
{
    unsigned int num=0,i;
    char key;

    for(i=0;i<digits;i++)
    {
        key = keypad_getkey();
        if(key>='0' && key<='9')
        {
            charLCD(key);
            num = num*10 + (key-'0');
        }
        else i--;
    }
    return num;
}

/************************************************
   RTC EDIT MENU SYSTEM
************************************************/
void edit_rtc_menu(void)
{
    char choice; // must declare at top
    unsigned int val;

    while(1)
    {
        cmdLCD(0x01);
        strLCD("1.h 2.m 3.s 4.d");
        cmdLCD(0xC0);
        strLCD("5.D 6.M 7.Y 8.E");

        choice = keypad_getkey();

        if(choice >= '1' && choice <= '7')
            edit_rtc_with_keypad(choice);
        else if(choice == '8')
        {
            cmdLCD(0x01);
            strLCD("RTC UPDATED!");
            delay_ms(1000);
            cmdLCD(0x01);
            break;
        }
    }
}

void edit_rtc_with_keypad(unsigned char field)
{
    unsigned int val;
    cmdLCD(0x01);

    switch(field)
    {
        case '1':
            strLCD("EDIT HOURS:");
            cmdLCD(0xC0);
            val = get_number_from_keypad(2);
            if(val <= 23) HOUR = val;
            else { strLCD("INVALID"); delay_ms(1000); }
            break;
        case '2':
            strLCD("EDIT MIN:");
            cmdLCD(0xC0);
            val = get_number_from_keypad(2);
            if(val <= 59) MIN = val;
            else { strLCD("INVALID"); delay_ms(1000); }
            break;
        case '3':
            strLCD("EDIT SEC:");
            cmdLCD(0xC0);
            val = get_number_from_keypad(2);
            if(val <= 59) SEC = val;
            else { strLCD("INVALID"); delay_ms(1000); }
            break;
        case '4':
            strLCD("EDIT DAY:");
            cmdLCD(0xC0);
            val = get_number_from_keypad(1);
            if(val <= 6) DOW = val;
            else { strLCD("INVALID"); delay_ms(1000); }
            break;
        case '5':
            strLCD("EDIT DATE:");
            cmdLCD(0xC0);
            val = get_number_from_keypad(2);
            if(val >=1 && val <= daysInMonth(MONTH, YEAR)) DOM = val;
            else { strLCD("INVALID"); delay_ms(1000); }
            break;
        case '6':
            strLCD("EDIT MONTH:");
            cmdLCD(0xC0);
            val = get_number_from_keypad(2);
            if(val>=1 && val<=12) MONTH = val;
            else { strLCD("INVALID"); delay_ms(1000); }
            break;
        case '7':
            strLCD("EDIT YEAR:");
            cmdLCD(0xC0);
            val = get_number_from_keypad(4);
            if(val>=2000 && val<=2099) YEAR = val;
            else { strLCD("INVALID"); delay_ms(1000); }
            break;
    }
}

/************************************************
   DATE VALIDATION HELPERS
************************************************/
unsigned char isLeapYear(unsigned int y)
{
    if((y%400==0) || (y%4==0 && y%100!=0)) return 1;
    else return 0;
}

unsigned char daysInMonth(unsigned int m, unsigned int y)
{
    switch(m)
    {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12: return 31;
        case 4: case 6: case 9: case 11: return 30;
        case 2: return isLeapYear(y)?29:28;
        default: return 31;
    }
}

/************************************************
   EXTERNAL INTERRUPT HANDLER
************************************************/
void EINT1_ISR(void) __irq
{
    edit_rtc_menu();
    EXTINT |= (1<<1);
    VICVectAddr = 0;
}

/************************************************
   SIMPLE DELAY
************************************************/
void delay_ms(unsigned int ms)
{
    unsigned int i,j;
    for(i=0;i<ms;i++)
        for(j=0;j<2000;j++);
}
