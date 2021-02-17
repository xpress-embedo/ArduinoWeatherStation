#include <OLED_I2C.h>

const int led = 13u;
OLED  myOLED(SDA, SCL);

extern uint8_t SmallFont[];

uint8_t buff[10u] = { 0 };
uint8_t buff_idx = 0;

// Funtion Prototype
uint8_t send_echo_off( void );
uint8_t send_at( uint32_t timeout );
uint8_t send_mode( uint8_t mode, uint32_t timeout );

uint8_t check_for_ok( uint32_t timeout );

void setup() 
{
  Serial.begin( 9600 );
  pinMode( led, OUTPUT );
  if(!myOLED.begin(SSD1306_128X64))
  {
    while(1);
  }
  myOLED.setFont(SmallFont);
  myOLED.clrScr();
  myOLED.print("Embedded Laboratory", CENTER, 0u);
  myOLED.print("Open Weather Map", CENTER, 16u);
  myOLED.print("Weather Station", CENTER, 32u);
  myOLED.update();
  delay(2000);
  send_echo_off();
  while ( !send_at(2000) )
  {
    myOLED.print("Connecting....       ", LEFT, 56u);
    myOLED.update();
  }
  myOLED.print("ESP8266 Connection OK", LEFT, 56u);
  myOLED.update();

  delay(2000);
  while( !send_mode(1,2000) )
  {
    myOLED.print("Unable to Set Mode   ", LEFT, 56u);
    myOLED.update();
  }
  myOLED.print("MODE SET OK          ", LEFT, 56u);
  myOLED.update();
}

void loop()
{
//  myOLED.clrScr();
//  myOLED.print("Upper case:", LEFT, 0);
//  myOLED.print("ABCDEFGHIJKLM", CENTER, 16);
//  myOLED.print("NOPQRSTUVWXYZ", CENTER, 24);
//  myOLED.update();
  delay (1000);
}

// Function Definition
uint8_t send_echo_off( void )
{
  Serial.println("ATE0");
  delay(2000);
}

uint8_t send_at( uint32_t timeout )
{
  uint8_t status = false;
  Serial.println("AT");
  status = check_for_ok( timeout );
  return status;
}

uint8_t send_mode( uint8_t mode, uint32_t timeout )
{
  uint8_t status = false;
  Serial.println("AT+CWMODE=1");
  // Serial.print(mode);
  // Serial.println();
  status = check_for_ok( timeout );
  return status;
}

uint8_t check_for_ok( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t status = false;
  buff_idx = 0;
  memset( buff, 0u, sizeof(buff) );
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0u )
    {
      buff[buff_idx] = Serial.read();
      buff_idx++;
    }
    // Check for \r\n Response
    if( buff[0] == '\r' && buff[1] == '\n' && buff[2] == 'O' && buff[3] == 'K' && buff[4] == '\r' && buff[5] == '\n' )
    {
      status = true;
    }
  }
  return status;
}
