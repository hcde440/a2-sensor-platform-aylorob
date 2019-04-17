/*ICE #3.1
 * 
   DHT22 temperature and humidity sensor demo.
    
   brc 2018
*/

// URL for dashboard: https://io.adafruit.com/aylorob/dashboards/hcde-440-a2

// Adafruit IO Temperature & Humidity Example
// Tutorial Link: https://learn.adafruit.com/adafruit-io-basics-temperature-and-humidity
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016-2017 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

/************************ Example Starts Here *******************************/
#include <ESP8266WiFi.h>                                  
#include <ESP8266HTTPClient.h>                            
#include <ArduinoJson.h>                                  
#include <Adafruit_Sensor.h>                              
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_MPL115A2.h>    //Include these libraries
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
                                                          
typedef struct {                                          //Its another new data structure
  String temp;                                            //Temperature
  String humid;                                           //Humidity
  String windS;                                           //Wind speed
  String windD;                                           //Wind direction, which is in degrees it seems
  String cloud;                                           //Cloud patterns, which is really weather description
  String pr;                                              //Pressure because it was in the example code and why not
} MetData;                                                //The name of the newfangled data structure

MetData weather;                                          //Creating an instance of MetData

// pin connected to DH22 data line
#define DATA_PIN 12

// create DHT22 instance
DHT_Unified dht(DATA_PIN, DHT22);

// set up the 'temperature', 'humidity', 'pressure', 'IO Output', and button feeds
AdafruitIO_Feed *temperature = io.feed("temperature");
AdafruitIO_Feed *humidity = io.feed("humidity");
AdafruitIO_Feed *pressure = io.feed("pressure");
AdafruitIO_Feed *digital = io.feed("IO Output");
AdafruitIO_Feed *magnet = io.feed("button");

//Set a variable for the IO button to change
int go = 0;

void setup() {

  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while(! Serial);

  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));                            //These four lines give description of of file name and date 
  Serial.print("Complied: ");
  Serial.println(F(__DATE__ " " __TIME__));
  
  // initialize dht22
  dht.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();                     // Clear any existing image
  display.setTextSize(1);                     // Set text size
  display.setTextColor(WHITE);                // Set text color
  display.setCursor(0, 0);                    // Put cursor back on top left
  display.println("Starting up.");            // Test and write up
  display.display();                          // Display the display

  pinMode(16,INPUT);
  
  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();
  
  // set up a message handler for the 'digital' feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  digital->onMessage(handleMessage);
  
  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

}

void loop() {

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

  Serial.println(go);                                     //Print state of IO button                             
  if (go == 1) {                                          //Check if can run

    int value = digitalRead(16);                          //Read the hall effect
    int outputValue = map(value,0,1,1,0);                 //Switch the value of a magnet from 0 to 1
    Serial.println(outputValue);                          //Print that value

    magnet->save(outputValue);                            //Send the value to IO
    
    sensors_event_t event;                                //An event is a grabbing of data, so prep a instance
    dht.temperature().getEvent(&event);                   //Actually grab the data
  
    float celsius = event.temperature;                    //Take temperature from the event
    float fahrenheit = (celsius * 1.8) + 32;              //Convert to Celcius

    Serial.print("The instrumentation says it is ");      //Print results
    Serial.print(celsius);                        
    Serial.println("C inside.");

    getMet("Seattle");                                    //Use getMet to find the outside temperature
    
    Serial.print("The OpenWeather API says it is ");      //Print results
    Serial.print(weather.temp);
    Serial.println("C outside");
    
    display.clearDisplay();                               //Clear test display off
  
    display.setCursor(0, 0);                              //Reset cursor
    display.print("Temperature inside in C:.");           //Print that to display
    display.println(celsius);                             //Print that to display too
    display.display();                                    //Display the display
    
    // save fahrenheit (or celsius) to Adafruit IO
    temperature->save(celsius);
  
    dht.humidity().getEvent(&event);                      //Grab humidity from event
  
    Serial.print("humidity: ");                           //Print the humidity
    Serial.print(event.relative_humidity);
    Serial.println("%");
  
    // save humidity to Adafruit IO
    humidity->save(event.relative_humidity);
  
    // wait 5 seconds (5000 milliseconds == 5 seconds)
    delay(5000);
    
  }
}

void handleMessage(AdafruitIO_Data *data) {     //Deal with any incoming data from IO
 
  Serial.print("received <- ");                 //Print that
 
  if(data->toPinLevel() == 1) {                 //Make it not a pointer so it can be used, then see if pressed or not
    Serial.println("Go");                       //Print that
    go = 1;                                     //If pressed set go to 1
  } 
  else {                                        //else
    Serial.println("Stop");                     //print that
    go = 0;                                     //When button let go set go to 0
  }
}



void getMet(String city) {                                                    //Takes string input, outputs nothing
  HTTPClient theClient;                                                       //Make a client
  String apiCall = "http://api.openweathermap.org/data/2.5/weather?q=";       //Start forming the api call
  apiCall += city;                                                            //Add the city
  apiCall += "&units=metric&appid=";                                          //Make sure imperial units and prep the key
  apiCall += weaKey;                                                          //Add the key itself
  theClient.begin(apiCall);                                                   //Make the call on the client
  int httpCode = theClient.GET();                                             //Get the response
  if (httpCode > 0) {                                                         //If it did connect

    if (httpCode == HTTP_CODE_OK) {                                           //If you got a good connection
      String payload = theClient.getString();                                 //Get json as string
      DynamicJsonBuffer jsonBuffer;                                           //Make json buffer of adaptable size
      JsonObject& root = jsonBuffer.parseObject(payload);                     //Parse string and set it as root
      if (!root.success()) {                                                  //If that didnt work
        Serial.println("parseObject() failed in getMet().");                  //Print that
        return;                                                               //Exit
      }
      weather.temp = root["main"]["temp"].as<String>();                       //Assigning json to MetData as strings cause thats what it expects
      weather.pr = root["main"]["pressure"].as<String>();
      weather.humid = root["main"]["humidity"].as<String>();
      weather.cloud = root["weather"][0]["description"].as<String>();
      weather.windS = root["wind"]["speed"].as<String>();
      weather.windD = root["wind"]["deg"].as<int>();
    }
  }
  else {                                                                                  //If all else fails
    Serial.println("Something went wrong with connecting to the endpoint in getMet().");   //Print that
  }
}
