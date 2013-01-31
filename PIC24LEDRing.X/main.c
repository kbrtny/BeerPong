/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

//#include <p24F04KA201.h>
#include <xc.h>
#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */


/*
 RA0:  PGC
 RA1:  PGD
 RA2:  AN2
 RA3:  AN3
 RA4:  G
 RA5:  MCLR
 RA6:  D0
 RB0:  AN0
 RB1:  AN1
 RB2:  RX
 RB4:  R
 RB7:  TX
 RB8:  XLAT
 RB9:  BLANK
 RB12: SCLK
 RB13: SIN
 RB14: GSCLK
 RB15: B
 */
/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

#define REDF LATBbits.LATB4
#define BLUEF LATBbits.LATB15
#define GREENF LATAbits.LATA4
#define XLAT LATBbits.LATB8
#define BLANK LATBbits.LATB9

#define RED 1
#define GREEN 2
#define BLUE 3

/* i.e. uint16_t <variable_name>; */
uint8_t LatchUpdate,DataUpdate,SequenceUpdate,i,Color;
uint16_t PackedData[36];


void Init(void);
void SetLEDRGB(unsigned char index,unsigned char R,unsigned char G,unsigned char B);
void SetLEDtoHSL(unsigned char index,unsigned short Hue,unsigned char Sat,unsigned char Light);
unsigned char ScaleSixty(unsigned char inval);
/******************************************************************************/
/* Initialization                                                              */
/******************************************************************************/
void Init(void)
{
    //IO Pins
    TRISA=0x000C;
    Nop();
    TRISB=0x0007;
    Nop();
    AD1PCFG=0x000F;
    REDF=1;
    GREENF=1;
    BLUEF=1;

    //SPI Port 8Mhz, CKE=1 CKP=0 with 32Mhz Fcy, 16 bits at a time.
    SPI1CON1=0x051B;
    SPI1STATbits.SPIROV=0;
    SPI1STATbits.SPIEN=1;

    //GSCLK Setup
    OC1RS=5;
    OC1R=5;
    OC1CONbits.OCM=6;
    PR2=9;
    T2CONbits.TON=1;

    //XLat timer to go off every 2.56mS and interrupt
    PR1=40960;
    T1CONbits.TON=1;

}

/******************************************************************************/
/* Interrupts                                                                 */
/******************************************************************************/

//XLAT Timer
void __attribute__ ((__interrupt__)) _T1Interrupt(void)
{
    IFS0bits.T1IF = 0;
    BLANK=1;
    if(LatchUpdate)
    {
        XLAT=1;
        Nop();
        XLAT=0;
        Nop();
        LatchUpdate=0;
    }
    BLANK=0;
    if(DataUpdate)
    {
        switch(Color)
        {
            case RED:
                BLUEF=1;
                Nop();
                REDF=0;
                for(i=0;i<12;i++)
                {
                    SPI1BUF=PackedData[i];
                }
                Color=GREEN;
                break;
            case GREEN:
                REDF=1;
                Nop();
                GREENF=0;
                for(i=12;i<24;i++)
                {
                    SPI1BUF=PackedData[i];
                }
                Color=BLUE;
                break;
            case BLUE:
                GREENF=1;
                Nop();
                BLUEF=0;
                for(i=24;i<36;i++)
                {
                    SPI1BUF=PackedData[i];
                }
                SequenceUpdate=1;
                Color=RED;
                break;
            default:
                GREENF=1;
                Nop();
                BLUEF=1;
                Nop();
                REDF=1;
                Color=RED;
                break;
        }
        DataUpdate=0;
        LatchUpdate=1;
    }
    if(SequenceUpdate)
    {
        //NextFrame();

        //SetLED(ind,0);

        //if(ind<1)ind=15;
        //ind--;
        //SetLED(ind,50);
        //CycleDown(1,1);
    }
}

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

int16_t main(void)
{

    /* Configure the oscillator for the device */
    //__builtin_write_OSCCONH(0x01);  /* Set OSCCONH for clock switch */
    //__builtin_write_OSCCONL(0x01);  /* Start clock switching */
    //while(OSCCONbits.COSC != 0b001);

    /* Initialize IO ports and peripherals */
    Init();

    /* TODO <INSERT USER APPLICATION CODE HERE> */
    //while(OSCCONbits.COSC != 0b001);
    while(1)
    {
        BLUEF=0;
        REDF=0;
        BLUEF=1;
        REDF=1;
    }
}

void SetLEDRGB(unsigned char index,unsigned char R,unsigned char G,unsigned char B)
{
    unsigned char offset,d,i;
    offset=index>>2;
    offset*=3;
    index-=offset;
    for(i=0;i<3;i++)
    {
        if(i==0)
        {
            d=R;
        }else if(i==1)
        {
            d=G;
            offset+=12;
        }else if(i==2)
        {
            d=B;
            offset+=12;
        }
        if(index==0)
        {
            PackedData[offset]&=0x000F;
            PackedData[offset]|=d<<8;
        }else if(index==1)
        {
            PackedData[offset]&=0xFFF0;
            PackedData[offset]|=d>>4;
            PackedData[offset+1]&=0x00FF;
            PackedData[offset]|=d<<12;
        }else if(index==2)
        {
            PackedData[offset+1]&=0xFF00;
            PackedData[offset+1]|=d;
            PackedData[offset+2]&=0x0FFF;
        }else if(index==3)
        {
            PackedData[offset+2]&=0xF000;
            PackedData[offset+2]|=d<<4;
        }
    }
}

void SetLEDtoHSL(unsigned char index,unsigned short Hue,unsigned char Sat,unsigned char Light)
{
    unsigned short C,R,G,B;
    R=0;
    G=0;
    B=0;
    if(Light>128)
    {
        C=((Light-128)*2)^255;
    }else if(Light&0x80)
    {
        C=255;
    }else
    {
        C=Light<<1;
    }
    if(C==0xff)
    {
        C=Sat;
    }else if(Sat!=0xff)
    {
        C=(C*Sat)>>8;
    }
    if(C!=0xff)
    {
        if(Hue<60)
        {
            R=C;
            G=(C*ScaleSixty(Hue))>>8;
        }else if(Hue<120)
        {
            R=(C*(255-ScaleSixty(Hue-60)))>>8;
            G=C;
        }else if(Hue<180)
        {
            G=C;
            B=(C*ScaleSixty(Hue-120))>>8;
        }else if(Hue<240)
        {
            G=(C*(255-ScaleSixty(Hue-180)))>>8;
            B=C;
        }else if(Hue<300)
        {
            R=(C*ScaleSixty(Hue-240))>>8;
            B=C;
        }else if(Hue<360)
        {
            R=C;
            B=(C*(255-ScaleSixty(Hue-240)))>>8;
        }
    }else
    {
        if(Hue<60)
        {
            R=C;
            G=ScaleSixty(Hue);
        }else if(Hue<120)
        {
            R=(255-ScaleSixty(Hue-60));
            G=C;
        }else if(Hue<180)
        {
            G=C;
            B=ScaleSixty(Hue-120);
        }else if(Hue<240)
        {
            G=(255-ScaleSixty(Hue-180));
            B=C;
        }else if(Hue<300)
        {
            R=ScaleSixty(Hue-240);
            B=C;
        }else if(Hue<360)
        {
            R=C;
            B=(255-ScaleSixty(Hue-240));
        }
    }
    SetLEDRGB(index,R,G,B);
}

unsigned char ScaleSixty(unsigned char inval)
{
    unsigned short tmp;
    tmp=(inval<<4)+inval;
    tmp>>=2;
    return tmp;
}
