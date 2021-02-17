#include <OLED_I2C.h>

/*----------------------------------VARIABLES---------------------------------*/
extern uint8_t SmallFont[];
extern uint8_t TinyFont[];
const int led = 13u;
OLED  myOLED(SDA, SCL);

/*---WiFi User Name and Password---*/
String ssid = "TestWiFi";
String pswd = "12345678";

/*---Buffer and Index to process response from ESP8266 Module---*/
uint8_t buff_idx = 0;
uint8_t buff[200] = { 0 };

uint8_t IP_ADDRESS[17u] = { 0 };    /*--Store IP Address--*/
uint8_t MAC_ADDRESS[18u] = { 0 };   /*--Store MAC Address--*/

#define OLED_SMALL_LEN    22u
char oled_buff_s[OLED_SMALL_LEN] = { 0 };     /*--OLED Data to write on a line with Small Font--*/

/*------------------------------FUNCTION PROTOTYPES---------------------------*/
uint8_t send_echo_off( void );
uint8_t send_disconnect_ap( void );
uint8_t send_at( uint32_t timeout );
uint8_t send_mode( uint8_t mode, uint32_t timeout );
uint8_t send_join_ap( String ssid, String pswd, uint32_t timeout );
uint8_t get_ip_mac_address( uint8_t *ip, uint8_t *mac, uint32_t timeout );

uint8_t check_for_ok( uint32_t timeout );
uint8_t check_for_join_ap( uint32_t timeout );
uint8_t check_get_ip_mac_address( uint8_t ip, uint8_t mac, uint32_t timeout );

void flush_serial_data( void );
void flush_buffer( void );

void setup() 
{
  /*--Initialize the Serial Module, ESP8266 is connected with this--*/
  /*--Make sure to change the baud rate of ESP8266 module to 9600 using AT+UART_DEF command--*/
  Serial.begin( 9600 );
  /*--Initialize OLED--*/
  if(!myOLED.begin(SSD1306_128X64))
  {
    while(1);
  }
  
  myOLED.setFont(SmallFont);
  myOLED.clrScr();
  myOLED.print("EMBEDDED LABORATORY", CENTER, 0u);
  myOLED.print("OPEN WEATHER API", CENTER, 16u);
  myOLED.print("Weather Station", CENTER, 24u);
  myOLED.update();
  
  /*--Send ECHO Off Command--*/
  send_echo_off();
  myOLED.print("...                  ", LEFT, 56u);
  myOLED.update();
  /*--Check if module is responding with OK response--*/
  while ( !send_at(2000) )
  {
    delay(1000);
  }
  myOLED.print(".........            ", LEFT, 56u);
  myOLED.update();

  delay(500);
  /*--Set module in Station Mode--*/
  while( !send_mode(1,2000) )
  {
    /*--if control comes here, this means problem in setting mode--*/
    delay(500);
  }
  myOLED.print(".............        ", LEFT, 56u);
  myOLED.update();

  delay(500);
  /*--Join with Access Points--*/
  while( !send_join_ap(ssid, pswd, 5000) )
  {
    /*--unable to connect, so retry once again--*/
    delay(500);
  }
  myOLED.print("................     ", LEFT, 56u);
  myOLED.update();
  
  /*--Get IP Address and MAC Address from the Module--*/
  while( !get_ip_mac_address( IP_ADDRESS, MAC_ADDRESS, 2000) )
  {
  
    /*--unable to get, try again--*/
    delay(500);
  }
  myOLED.print(".....................", LEFT, 56u);
  snprintf( oled_buff_s, OLED_SMALL_LEN, "IP:%s", (char*)IP_ADDRESS);
  myOLED.print( oled_buff_s, LEFT, 40u);
  snprintf( oled_buff_s, OLED_SMALL_LEN, "MAC:%s", (char*)MAC_ADDRESS);
  myOLED.print( oled_buff_s, LEFT, 48u);
  myOLED.update();
  /*--display information for 1 second and then move to looping state--*/
  delay(1000);
}

void loop()
{
  // TODO:
}

/*-----------------------------FUNCTION DEFINITIONS---------------------------*/
/**
 * @brief Send Echo Off Command
 * 
 * This function sends the ECO Off Command to ESP Module and then flushes 
 * the serial data.
 * @return Status of the function, true or false TODO:
 */
uint8_t send_echo_off( void )
{
  Serial.println("ATE0");
  delay(2000);
  // Flush the Serial Data
  flush_serial_data();
  return true;
}

/**
 * @brief Disconnect from Access Points
 * 
 * This function sends the command to disconnect from connect access points
 * @return Status of the function, true or false TODO:
 */
uint8_t send_disconnect_ap( void )
{
  Serial.println("AT+CWQAP");
  delay(2000);
  // Flush the Serial Data
  flush_serial_data();
  return true;
}

/**
 * @brief Send AT Command
 * 
 * This function sends the AT command to module and check if the response is
 * valid.
* @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t send_at( uint32_t timeout )
{
  uint8_t status = false;
  flush_buffer();
  Serial.println("AT");
  status = check_for_ok( timeout );
  return status;
}

/**
 * @brief Disconnect from Access Points
 * 
 * This function sends the command to disconnect from connect access points
 * @param mode TODO: not used
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t send_mode( uint8_t mode, uint32_t timeout )
{
  uint8_t status = false;
  flush_buffer();
  Serial.println("AT+CWMODE=1");
  // Serial.print(mode);
  // Serial.println();
  status = check_for_ok( timeout );
  return status;
}

const char WIFI_CONNECTED[] = "WIFI CONNECTED";
const char WIFI_GOT_IP[] = "WIFI GOT IP";

/**
 * @brief Send Command to Join Access Point
 * 
 * This function sends the command to join the access point.
 * @param ssid WiFi User Name
 * @param pswd WiFi Password
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t send_join_ap( String ssid, String pswd, uint32_t timeout )
{
  uint8_t status = false;
  // before connecting send the disconnect command
  send_disconnect_ap();
  // clear the buffer
  flush_buffer();
  String packet = "AT+CWJAP=\"" + ssid + "\",\"" + pswd + "\"";
  Serial.println(packet);
  status = check_for_join_ap(timeout);
  return status;
}

/**
 * @brief Get IP and MAC Address
 * 
 * This function sends the command to get the IP and MACS Address from the WiFi
 * Module
 * @param ip pointer to IP Address Buffer
 * @param mac pointer to MAC Address Buffer
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t get_ip_mac_address( uint8_t *ip, uint8_t *mac, uint32_t timeout )
{
  uint8_t status = false;
  flush_buffer();
  Serial.println("AT+CIFSR");
  status = check_get_ip_mac_address(ip, mac, timeout);
  return status;
}

/**
 * @brief Check the OK Response
 * 
 * This function checks for the OK response from the module
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_for_ok( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t status = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      buff[buff_idx] = Serial.read();
      buff_idx++;
    }
    // Check for \r\nOK\r\n Response
    if( buff[0] == '\r' && buff[1] == '\n' && buff[2] == 'O' && buff[3] == 'K' && buff[4] == '\r' && buff[5] == '\n' )
    {
      status = true;
    }
  }
  return status;
}

/**
 * @brief Check the Response of Access Point Join Command
 * 
 * This function checks the response of CWJAP command, and if joining is 
 * successfull, it returns true
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_for_join_ap( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t status = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      buff[buff_idx] = Serial.read();
      buff_idx++;
    }
    // Check for \r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n Response
    if( strncmp( (char*)buff, WIFI_CONNECTED, sizeof(WIFI_CONNECTED)-1 ) == 0 )
    {
      if( strncmp( (char*)(buff+16), WIFI_GOT_IP, sizeof(WIFI_GOT_IP)-1 ) == 0 )
      {
        if( buff[33] == '\r' && buff[34] == '\n' )
        {
          status = true;
        }
      }
    }
  }
  return status;
}

/**
 * @brief Check the Response of CIFSR Command
 * 
 * This function checks the response of CIFSR command, and then parse the data
 * into IP Address and MAC Address Array
 * @param ip pointer to IP Address Buffer
 * @param mac pointer to MAC Address Buffer
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_get_ip_mac_address( uint8_t *ip, uint8_t *mac, uint32_t timeout )
{
  /*The data format is as below
  +CIFSR:STAIP,"192.168.43.160"{0D}{0A}
  +CIFSR:STAMAC,"5c:cf:7f:ae:3f:09"{0D}{0A}{0D}{0A}OK{0D}{0A}
  */
  uint32_t timestamp;
  uint8_t status = false;
  uint8_t idx = 0u;
  uint8_t index = 0;
  uint8_t found = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      buff[buff_idx] = Serial.read();
      // Check if data is received
      if( buff_idx > 4u && buff[buff_idx] == '\n' && buff[buff_idx-1] == '\r' && buff[buff_idx-2] == 'K' && buff[buff_idx-3] == 'O' )
      {
        status = true;
      }
      buff_idx++;
    }
  }
  
  // Data is Received Completely, Now
  // Look for IP Address
  while( (idx < sizeof(buff)) && found == false && (millis()-timestamp <= timeout) )
  {
    if( buff[idx] == '\"' )
    {
      // the value of idx points to IP Address Information
      found = true;
    }
    idx++;
  }
  if( found == true )
  {
    // Copy IP Address Data
    index = 0;
    while( buff[idx] != '\"' )
    {
      *(ip+index) = buff[idx];
      index++;
      idx++;
    }
    idx++;
    *(ip+index) = 0;  // Added Null Character
  }
  // Look for MAC Address
  found = false;
  while( (idx < sizeof(buff)) && found == false && (millis()-timestamp <= timeout) )
  {
    if( buff[idx] == '\"' )
    {
      // the value of idx points to IP Address Information
      found = true;
    }
    idx++;
  }
  if( found == true )
  {
    // COPY MAC Address
    index = 0;
    while( buff[idx] != '\"' )
    {
      *(mac+index) = buff[idx];
      index++;
      idx++;
    }
    *(mac+index) = 0;  // Added Null Character
  }
  return status;
}

/**
 * @brief Clear the Buffer
 * 
 * This function resets the buffer index and buffer content
 */
void flush_buffer( void )
{
  buff_idx = 0x00;
  memset( buff, 0x00, sizeof(buff) );
}

/**
 * @brief Flush the Serial Data
 * 
 * This function flushes the Serial Data from the internal buffers
 */
void flush_serial_data( void )
{
  uint8_t temp = 0;
  while( Serial.available() > 0 )
  {
    temp = Serial.read();
  }
}
