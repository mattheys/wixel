/* wireless_serial app:
 * This app allows you to connect two Wixels together to make a wireless,
 * bidirectional, lossless serial link.  
 * See description.txt or the Wixel User's Guide for more information.
 */

/** Dependencies **************************************************************/
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <radio_com.h>
#include <radio_link.h>

/** Parameters ****************************************************************/
int32 CODE param_baud_rate = 9600;
int32 CODE param_Tranciever = 0;

/** Global Variables **********************************************************/
uint16 lastTransmit;
uint16 sendChar = 0;
/** Functions *****************************************************************/

void updateLeds()
{
    static BIT dimYellowLed = 0;
    static uint16 lastRadioActivityTime;
    uint16 now;

    usbShowStatusWithGreenLed();

    now = (uint16)getMs();

    if (!radioLinkConnected())
    {
        // We have not connected to another device wirelessly yet, so do a
        // 50% blink with a period of 1024 ms.
        LED_YELLOW(now & 0x200 ? 1 : 0);
    }
    else
    {
        // We have connected.

        if ((now & 0x3FF) <= 20)
        {
            // Do a heartbeat every 1024ms for 21ms.
            LED_YELLOW(1);
        }
        else if (dimYellowLed)
        {
            static uint8 DATA count;
            count++;
            LED_YELLOW((count & 0x7)==0);
        }
        else
        {
            LED_YELLOW(0);
        }
    }

    if (radioLinkActivityOccurred)
    {
        radioLinkActivityOccurred = 0;
        dimYellowLed ^= 1;
        lastRadioActivityTime = now;
    }

    if ((uint16)(now - lastRadioActivityTime) > 32)
    {
        dimYellowLed = 0;
    }
}

void usbToRadioService()
{
    uint8 signals;

    if(param_Tranciever == 1)
    {
        if (getMs() > lastTransmit + 1000)
        {
            if(radioComTxAvailable())
            {
                lastTransmit = getMs();
                if (sendChar == 0)
                {
                    radioComTxSendByte(97);
                }
                else
                {
                    radioComTxSendByte(98);
                }
                sendChar ^= 1;
            }
            // Do a heartbeat every 1024ms for 21ms.
        }
    }
    
    if (param_Tranciever == 0)
    {
        while(radioComRxAvailable())
        {
            uint8 getByte = radioComRxReceiveByte();
            if(getByte == 97)
            {
                LED_RED(1);
            }
            if(getByte == 98)
            {
                LED_RED(0);
            }
        }
    }
    // Control Signals

    radioComTxControlSignals(usbComRxControlSignals() & 3);

    // Need to switch bits 0 and 1 so that DTR pairs up with DSR.
    signals = radioComRxControlSignals();
    usbComTxControlSignals( ((signals & 1) ? 2 : 0) | ((signals & 2) ? 1 : 0));
}

void main()
{
    systemInit();

    usbInit();

    radioComRxEnforceOrdering = 1;
    radioComInit();

    // Set up P1_5 to be the radio's TX debug signal.
    P1DIR |= (1<<5);
    IOCFG0 = 0b011011; // P1_5 = PA_PD (TX mode)

    while(1)
    {
        boardService();
        updateLeds();

        radioComTxService();

        usbComService();
        usbToRadioService();  
    }
}
