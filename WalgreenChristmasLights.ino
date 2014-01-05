// code to bit bang the protocol for the Walgreen's Christmas lights

class WalgreenLights
{
  private:
    int  pin;  // the pin to write to
    int  numLights;

    volatile uint8_t *pinPort;
    int pinMask;
    
    void SendPacket(unsigned char val)
    {
      register volatile uint8_t *port = pinPort;
      register int mask = pinMask;
      
      /////////////////////////////////////////////
      //  **********   WARNING  **********
      // This code runs with INTERRUPTS DISABLED
      ////////////////////////////////////////////
      uint8_t oldSREG = SREG;  // get interrupt register state
      cli();    // disable interrupts

      // 3 pulses of H 6.25us + L 6.67us 
      for (int i = 0; i < 3; i++)
      {
        *port |= mask;  // HIGH
        delayMicroseconds(6);
        *port &= ~mask;  // LOW
        delayMicroseconds(6);
      }
      
      // guess at LSB first
      for (int i = 0; i < 4; i++)
      {
        *port |= mask;  // HIGH
        delayMicroseconds((val & 1) ? 3 : 1);
        *port &= ~mask;  // LOW
        delayMicroseconds(3);
        val >>= 1;
      }

      SREG = oldSREG;  // restore interrupts

      // hold for at least one full packet time
      // this allows the downstream neighbor to forward on
      // his state to the next guy
      // could be 60us if we wanted to get really tight
      // 100us seems like plenty
      // default system waits 1.13ms between packets
      delayMicroseconds(100);
    }

    /*
// Macros To Speed Up Read/Writes
// from http://skpang.co.uk/blog/archives/323
#define Macro_SetPin( pin, state ) ( state == LOW ) ? Macro_SetPinLow( (pin) ) : Macro_SetPinHigh( (pin) )
#define Macro_SetPinLow( pin )  ( (pin) < 8 ) ? PORTD &= ~( 1 << (pin) ) : PORTB &= ~( 1 << ( (pin) - 8 ) )
#define Macro_SetPinHigh( pin ) ( (pin) < 8 ) ? PORTD |=  ( 1 << (pin) ) : PORTB |=  ( 1 << ( (pin) - 8 ) )

#define PIN 13

    void OldSendPacket(unsigned char val)
    {
      /////////////////////////////////////////////
      //  **********   WARNING  **********
      // This code runs with INTERRUPTS DISABLED
      ////////////////////////////////////////////
      uint8_t oldSREG = SREG;  // get interrupt register state
      cli();    // disable interrupts

      // 3 pulses of H 6.25us + L 6.67us 
      for (int i = 0; i < 3; i++)
      {
        Macro_SetPin(PIN, HIGH);
        delayMicroseconds(6);
        Macro_SetPin(PIN, LOW);
        delayMicroseconds(6);
      }
      
      // guess at LSB first
      for (int i = 0; i < 4; i++)
      {
        Macro_SetPin(PIN, HIGH);
        delayMicroseconds((val & 1) ? 3 : 1);
        Macro_SetPin(PIN, LOW);
        delayMicroseconds(2);
        val >>= 1;
      }

      SREG = oldSREG;  // restore interrupts

      // hold for at least one full packet time
      // this allows the downstream neighbor to forward on
      // his state to the next guy
      // could be 60us if we wanted to get really tight
      // 100us seems like plenty
      // default system waits 1.13ms between packets
      delayMicroseconds(100);
    }
*/

    void InitString(void)
    {
      // these delay values were determined by monitoring a set of lights
      digitalWrite(pin, HIGH);
      delay(150);
      digitalWrite(pin, LOW);
      delay(225);
      
      for (int i = 0; i < numLights; i++)
      {
        SendPacket(0);
      }
    }

    // set the value on one color of lights only
    // all others at 'other' (default 0 = off)
    // offsets: 0=White, 1=Orange, 2=Blue, 3=Green, 4=Red
    void OneColor(int offset, unsigned char val, unsigned char other = 0)
    {
      for (int i = 0; i < numLights; i++)
      {
        if (i % 5 == offset)
          SendPacket(val);
        else
          SendPacket(other);
      }
    }

  public:
    WalgreenLights(int _pin, int _numLights)
    {
      pin = _pin;
      numLights = _numLights;

      pinPort = (pin < 8) ? &PORTD : &PORTB;
      pinMask = (pin < 8) ? 1 << pin : 1 << (pin - 8);
      
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
      
      InitString();
    }
    
    ~WalgreenLights(void)
    {
      digitalWrite(pin, LOW);
      pinMode(pin, INPUT);
    }
    
    // valPtr points to an array of char
    // each char holds the value (0-15) for
    // the bulbs in the string
    void SendValue(unsigned char *valPtr)
    {
      for (int i = 0; i < numLights; i++)
      {
        SendPacket(*valPtr++);
      }
    }
    
    // set the value on the red lights only
    // all others at 'other' (default 0 = off)
    void RedOnly(unsigned char val, unsigned char other = 0)
    {
      OneColor(4, val, other);
    }
    void GreenOnly(unsigned char val, unsigned char other = 0)
    {
      OneColor(3, val, other);
    }
    void BlueOnly(unsigned char val, unsigned char other = 0)
    {
      OneColor(2, val, other);
    }
    void OrangeOnly(unsigned char val, unsigned char other = 0)
    {
      OneColor(1, val, other);
    }
    void WhiteOnly(unsigned char val, unsigned char other = 0)
    {
      OneColor(0, val, other);
    }
    
    void AllTo(unsigned char val)
    {
      OneColor(0, val, val);
    }
    
    void AllOn(void)
    {
      AllTo(15);
    }
    
    void AllOff(void)
    {
      AllTo(0);
    }
};

class WalgreenLights *lights, *lights2;

void setup()
{
  lights = new WalgreenLights(13, 15);
  lights2 = new WalgreenLights(12, 15);
}

unsigned char state[15];

void loop()
{
  // all on
  lights->AllOn();
  lights2->AllOff();
  delay(1000);

  // all off
  lights->AllOff();
  lights2->AllOn();
  delay(1000);

  // around the string
  for (int i = 0; i < sizeof(state); i++)
    state[i] = 0;
  for (int k = 0; k < 10; k++)
  {
    for (int i = 0; i < sizeof(state); i++)
    {
      if (i == 0)
        state[sizeof(state)-1] = 0;
      else
        state[i-1] = 0;
      state[i] = 15;
      
      lights->SendValue(state);
      lights2->SendValue(state);
      delay(15-k);
    }
  }

  // sparkle
  for (int k=0; k < 400; k++)
  {
    for (int i = 0; i < sizeof(state); i++)
      state[i] = random(0, 100) < 80 ? 1 : 15;
      
    lights->SendValue(state);
    lights2->AllTo(random(0,15));
    delay(1);
  }

  lights->AllOff();
  delay(25);
  
  // ramp up / down
  for (int v = 0; v < 16; v++)
  {
    lights->AllTo(v);
    lights2->AllTo(15-v);
    delay(50);
  }
  delay(250);
  for (int v = 15; v >= 0; v--)
  {
    lights->AllTo(v);
    lights2->AllTo(15-v);
    delay(50);
  }
  delay(1000);

  lights2->WhiteOnly(15, 2);
  lights->RedOnly(15, 2); delay(500);
  lights2->OrangeOnly(15, 2);
  lights->GreenOnly(15, 2); delay(500);
  lights2->BlueOnly(15, 2);
  lights->BlueOnly(15, 2); delay(500);
  lights2->GreenOnly(15, 2);
  lights->OrangeOnly(15, 2); delay(500);
  lights2->RedOnly(15, 2);
  lights->WhiteOnly(15, 2);
  delay(1000);
}



