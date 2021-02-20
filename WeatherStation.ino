#include <OLED_I2C.h>

/*----------------------------------VARIABLES---------------------------------*/
extern uint8_t SmallFont[];
extern uint8_t TinyFont[];
const int led = 13u;
OLED  myOLED(SDA, SCL);

/*---WiFi User Name and Password---*/
const String ssid = "TestWiFi";         /*--Replace with your Network Name--*/
const String pswd = "12345678";         /*--Replace with your Network Password--*/

const String GET_REQ_PRE = "GET /data/2.5/weather?q=";
const String GET_REQ_POST = "&APPID=ENTERYOURAPIKEYHERE&units=metric";   /*--Replace with your API Key--*/

const char WIFI_CONNECTED[] = "WIFI CONNECTED";
const char WIFI_GOT_IP[] = "WIFI GOT IP";
const char TCP_CONNECT[] = "CONNECT";
const char SEND_OK[] = "SEND OK";

/*---Buffer and Index to process response from ESP8266 Module---*/
uint8_t buff_idx = 0;
uint8_t buff[50u] = { 0 };
/*---Separate Buffer to Process the GET Response which contains weather data--*/
uint16_t weather_idx = 0u;
char weather_buff[1000u] = { 0 };

uint8_t IP_ADDRESS[17u] = { 0 };    /*--Store IP Address--*/
uint8_t MAC_ADDRESS[18u] = { 0 };   /*--Store MAC Address--*/

#define NUM_OF_CITIES           4u
#define WEATHER_DATA_SIZE       8u

typedef struct _city_info_s
{
  String city_name;
  uint8_t len;
} city_info_s;

/*--Weather Data Structure--*/
typedef struct _Weather_Data_s
{
  uint8_t temp_normal[WEATHER_DATA_SIZE];
  uint8_t temp_real_feel[WEATHER_DATA_SIZE];
  uint8_t temp_minimum[WEATHER_DATA_SIZE];
  uint8_t temp_maximum[WEATHER_DATA_SIZE];
  int8_t temp_n;        // TODO: Future
  int8_t temp_r_f;      // TODO: Future
  int8_t temp_min;      // TODO: Future
  int8_t temp_max;      // TODO: Future
} Weather_Data_s;

Weather_Data_s s_Weather_Data = { 0 };

/*--Total Number of cities--*/
const city_info_s s_city_info[NUM_OF_CITIES] = 
{
  { "London",     84u },
  { "Shimla",     84u },
  { "Delhi",      83u },
  { "New York",   86u }
};

#define OLED_SMALL_LEN    22u
char oled_buff_s[OLED_SMALL_LEN] = { 0 };     /*--OLED Data to write on a line with Small Font--*/

/*------------------------------FUNCTION PROTOTYPES---------------------------*/
uint8_t send_echo_off( void );
uint8_t send_disconnect_ap( void );
uint8_t send_at( uint32_t timeout );
uint8_t send_mode( uint8_t mode, uint32_t timeout );
uint8_t send_join_ap( String ssid, String pswd, uint32_t timeout );
uint8_t get_ip_mac_address( uint8_t *ip, uint8_t *mac, uint32_t timeout );
uint8_t send_connect_cmd( uint32_t timeout );
uint8_t send_close_cmd( void );
uint8_t send_num_of_bytes( uint8_t no_bytes, uint32_t timeout);
uint8_t send_get_req( String city_name, uint32_t timeout );

uint8_t check_for_ok( uint32_t timeout );
uint8_t check_for_join_ap( uint32_t timeout );
uint8_t check_get_ip_mac_address( uint8_t ip, uint8_t mac, uint32_t timeout );
uint8_t check_connect_cmd( uint32_t timeout );
uint8_t check_for_num_of_bytes( uint32_t timeout );
uint8_t check_get_req( uint32_t timeout );

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
  while( !send_join_ap(ssid, pswd, 6000) )
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
  delay(100);
}

void loop()
{
  uint8_t idx = 0;
  for( idx=0u; idx<NUM_OF_CITIES; idx++)
  {
    myOLED.setFont(TinyFont);
    /*--Connect the Open Weather Map--*/
    while( !send_connect_cmd( 2000 ) )
    {
      myOLED.clrScr();
      myOLED.print( "Connection: FAIL    ", LEFT, 56u);
      myOLED.update();
    }
    myOLED.clrScr();
    myOLED.print( "Connection: OK      ", LEFT, 56u);
    myOLED.setFont(SmallFont);
    delay(500);
    /*--Connection is OK, send Number of Bytes to Send--*/
    while( !send_num_of_bytes( s_city_info[idx].len, 3000) )
    {
    }
    delay(500);
    /*--Send GET Request and Receive Data--*/
    if( send_get_req( s_city_info[idx].city_name, 5000u ) == true )
    {
      myOLED.print("City Name:" + s_city_info[idx].city_name, CENTER, 0u);
      snprintf( oled_buff_s, OLED_SMALL_LEN, "Temp:%s C", (char*)s_Weather_Data.temp_normal);
      myOLED.print( oled_buff_s, LEFT, 16u);
      snprintf( oled_buff_s, OLED_SMALL_LEN, "Real Feel:%s C", (char*)s_Weather_Data.temp_real_feel);
      myOLED.print( oled_buff_s, LEFT, 24u);
      snprintf( oled_buff_s, OLED_SMALL_LEN, "Min.Temp:%s C", (char*)s_Weather_Data.temp_minimum);
      myOLED.print( oled_buff_s, LEFT, 32u);
      snprintf( oled_buff_s, OLED_SMALL_LEN, "Max.Temp:%s C", (char*)s_Weather_Data.temp_maximum);
      myOLED.print( oled_buff_s, LEFT, 40u);
      myOLED.update();
    }
    delay(2000);
  }
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
  delay(500);
  /*--Flush the Serial Data--*/
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
  delay(1000);
  /*--Flush the Serial Data--*/
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
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.println("AT");
  status = check_for_ok( timeout );
  /*--Flush the Serial Data--*/
  flush_serial_data();
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
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.println("AT+CWMODE=1");
  // Serial.print(mode);
  // Serial.println();
  status = check_for_ok( timeout );
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;
}

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
  /*--before connecting send the disconnect command--*/
  send_disconnect_ap();
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  String packet = "AT+CWJAP=\"" + ssid + "\",\"" + pswd + "\"";
  Serial.println(packet);
  status = check_for_join_ap(timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
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
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.println("AT+CIFSR");
  status = check_get_ip_mac_address(ip, mac, timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;
}

/**
 * @brief Connect with TCP Server
 * 
 * This function sends the command to connect with TCP Server
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t send_connect_cmd( uint32_t timeout )
{
  uint8_t status = false;
  /*--Before connecting send close the connection--*/
  send_close_cmd();
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.println("AT+CIPSTART=\"TCP\",\"api.openweathermap.org\",80");
  status = check_connect_cmd( timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;  
}

/**
 * @brief Disconnect with the Server
 * 
 * This function sends the command to disconnect from the server
 * @return Status of the function, true or false TODO:
 */
uint8_t send_close_cmd( void )
{
  /*--For Single Connections--*/
  Serial.println("AT+CIPCLOSE");
  delay(1000);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return true;
}

/**
 * @brief Send Number of Bytes
 * 
 * This function sends the CIPSEND command with no. of bytes
 * @param no_bytes length of GET Request
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false TODO:
 */
uint8_t send_num_of_bytes( uint8_t no_bytes, uint32_t timeout)
{
  uint8_t status = false;
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.print("AT+CIPSEND=");
  Serial.println(no_bytes);
  status = check_for_num_of_bytes( timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;  
}

/**
 * @brief Send GET Request
 * 
 * This function sends the GET request to OpenWeather website
 * @param String city name
 * @param timeout Timeout value to exit the function with failure
 * @param Weather Data Pointer
 * @return Status of the function, true or false TODO:
 */
uint8_t send_get_req( String city_name, uint32_t timeout )
{
  uint8_t status = false;
  String get_req = GET_REQ_PRE + city_name + GET_REQ_POST;
  /*--Flush Weather Response Buffer--*/
  flush_weather_buffer();
  Serial.println( get_req );
  status = check_get_req(timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
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
    /*--Check for \r\nOK\r\n Response--*/
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
    /*--heck for \r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n Response--*/
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
 * @brief Check the Response of CIPSTART Command
 * 
 * This function checks the response of CIPSTART command.
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_connect_cmd( uint32_t timeout )
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
    /*--valid response from module is CONNECT{0D}{0A}{0D}{0A}OK{0D}{0A} --*/
    if( strncmp( (char*)buff, TCP_CONNECT, sizeof(TCP_CONNECT)-1 ) == 0 )
    {
      if( buff[7] == '\r' && buff[8] == '\n' && buff[9] == '\r' && buff[10] == '\n' && \
          buff[11] == 'O' && buff[12] == 'K' && buff[13] == '\r' && buff[14] == '\n' )
      {
        status = true;
      }
    }
  }
  return status;
}

/**
 * @brief Check the Response of CIPSEND Command
 * 
 * This function checks the response of CIPSEND command.
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_for_num_of_bytes( uint32_t timeout )
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
    /*--Check for \r\nOK\r\n>Response--*/
    if( buff[0] == '\r' && buff[1] == '\n' && buff[2] == 'O' && buff[3] == 'K' && buff[4] == '\r' && buff[5] == '\n' && \
        buff[6] == '>' )
    {
      status = true;
    }
  }
  return status;
}

#define SEARCH_COUNTER      20u
/**
 * @brief Check the Response of GET Request Command
 * 
 * This function checks the response of the GET Request and parse the data and update the buffers
 * @param timeout Timeout value to exit the function with failure
 * @param Weather Data Pointer
 * @return Status of the function, true or false
 */
uint8_t check_get_req( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t counter = 0u;
  uint8_t status = false;
  uint8_t idx = 0u;
  char *pointer;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      weather_buff[weather_idx] = Serial.read();
      weather_idx++;
    }
    /*--we will get a lot of data from the module, and we have to first wait till data is completely received--*/
    /*--we receive this packet at last CLOSED\r\n, so we will wait till we get this data--*/
    /*Example of Data Received
    Recv 86 bytes{0D}{0A}{0D}{0A}SEND OK{0D}{0A}{0D}{0A}+IPD,554:{"coord":{"lon":-74.006,"lat":40.7143},"
    weather":[{"id":600,"main":"Snow","description":"light snow","icon":"13n"},
    {"id":701,"main":"Mist","description":"mist","icon":"50n"}],"base":"stations",
    "main":{"temp":-1.67,"feels_like":-9.42,"temp_min":-3,"temp_max":-0.56,"pressure":1021,"humidity":93},"visibility":9656,
    "wind":{"speed":7.72,"deg":30,"gust":10.29},"snow":{"1h":0.27},"clouds":{"all":90},"dt":1613730994,
    "sys":{"type":1,"id":4610,"country":"US","sunrise":1613735067,"sunset":1613774132},"timezone":-18000,"id":5128581,"name":"New York","cod":200}
    CLOSED{0D}{0A}--*/
    /*--TODO: poor way of doing, need to improve this--*/
    if( weather_idx > 10u && weather_buff[weather_idx-1] == '\n' && weather_buff[weather_idx-2] == '\r' && weather_buff[weather_idx-3] == 'D' && \
        weather_buff[weather_idx-4] == 'E' && weather_buff[weather_idx-5] == 'S' && weather_buff[weather_idx-6] == 'O' && weather_buff[weather_idx-7] == 'L' && weather_buff[weather_idx-8] == 'C' )
    {
      /*--this means that data has been received correctly and now we can proceed with parsing of the data--*/
      status = true;
    }
  }
  /*--parse data if status is true--*/
  if( status )
  {
    /*--search for "temp", data is like "temp":-1.67--*/
    pointer = strstr( (char*)weather_buff, "temp" );
    while( *pointer != '\"' && counter < SEARCH_COUNTER )
    {
      pointer++;
      counter++;
    }
    /*--If found copy data into buffer--*/
    if( counter < SEARCH_COUNTER )
    {
      counter = 0u;
      /*--SW has found quote of "temp":-1.67, and now copy data till we get ,--*/
      pointer++;  // reached til colon :
      pointer++;  // now reached -minus
      idx = 0u;
      while( *pointer != ',' && counter < SEARCH_COUNTER )
      {
        s_Weather_Data.temp_normal[idx] = *pointer;
        idx++;
        pointer++;
        counter++;
      }
      s_Weather_Data.temp_normal[idx] = 0;    // Added NULL Character
    }
    else
    {
      status = false;
    }
    
    /*--Search for Real Feel Temperature--*/
    if( counter < SEARCH_COUNTER )
    {
      counter = 0u;
      pointer = strstr( (char*)weather_buff, "feels_like" );
      while( *pointer != '\"' && counter < SEARCH_COUNTER )
      {
        pointer++;
        counter++;
      }
      /*--If found copy data into buffer--*/
      if( counter < SEARCH_COUNTER )
      {
        counter = 0u;
        /*--SW has found quote of feels_like":-1.67, and now copy data till we get ,--*/
        pointer++;  // reached til colon :
        pointer++;  // now reached -minus
        idx = 0u;
        while( *pointer != ',' && counter < SEARCH_COUNTER )
        {
          s_Weather_Data.temp_real_feel[idx] = *pointer;
          idx++;
          pointer++;
          counter++;
        }
        s_Weather_Data.temp_real_feel[idx] = 0;    // Added NULL Character
      }
    }
    else
    {
      status = false;
    }
    
    /*--Search for Minimum Temperature--*/
    if( counter < SEARCH_COUNTER )
    {
      counter = 0u;
      pointer = strstr( (char*)weather_buff, "temp_min" );
      while( *pointer != '\"' && counter < SEARCH_COUNTER )
      {
        pointer++;
        counter++;
      }
      /*--If found copy data into buffer--*/
      if( counter < SEARCH_COUNTER )
      {
        counter = 0u;
        /*--SW has found quote of feels_like":-1.67, and now copy data till we get ,--*/
        pointer++;  // reached til colon :
        pointer++;  // now reached -minus
        idx = 0u;
        while( *pointer != ',' && counter < SEARCH_COUNTER )
        {
          s_Weather_Data.temp_minimum[idx] = *pointer;
          idx++;
          pointer++;
          counter++;
        }
        s_Weather_Data.temp_minimum[idx] = 0;    // Added NULL Character
      }
    }
    else
    {
      status = false;
    }
    
    /*--Search for Maximum Temperature--*/
    if( counter < SEARCH_COUNTER )
    {
      counter = 0u;
      pointer = strstr( (char*)weather_buff, "temp_max" );
      while( *pointer != '\"' && counter < SEARCH_COUNTER )
      {
        pointer++;
        counter++;
      }
      /*--If found copy data into buffer--*/
      if( counter < SEARCH_COUNTER )
      {
        counter = 0u;
        /*--SW has found quote of feels_like":-1.67, and now copy data till we get ,--*/
        pointer++;  // reached til colon :
        pointer++;  // now reached -minus
        idx = 0u;
        while( *pointer != ',' && counter < SEARCH_COUNTER )
        {
          s_Weather_Data.temp_maximum[idx] = *pointer;
          idx++;
          pointer++;
          counter++;
        }
        s_Weather_Data.temp_maximum[idx] = 0;    // Added NULL Character
      }
    }
    else
    {
      status = false;
    }
    
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
 * @brief Clear the Weather Buffer
 * 
 * This function resets the buffer index and buffer content
 */
void flush_weather_buffer( void )
{
  weather_idx = 0x00;
  memset( weather_buff, 0x00, sizeof(weather_buff) );
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
