#include <YunClient.h>
#include <PubSubClient.h>
#include <Console.h>


YunClient yunClient;
PubSubClient mqtt("messaging.quickstart.internetofthings.ibmcloud.com", 1883, 0, yunClient);
unsigned long time;
// String pubString;
char pubChars[50];
char connectChars[50];
String macAddrStr;

void setup()
{
  Bridge.begin();
  Console.begin();
  
  // wait for the console to connect so we can see what's happening
  // useful for debugging
  //  while (!Console){
  //    ; // wait for Console port to connect.
  //  }
  
  // get the mac address from the linux half of the Yun board

  String s = getMacAddressString();
  s.replace(":","");
  s.toLowerCase();
  macAddrStr = s.substring(0,12);
    
  // connect to node-RED quickstart to receive messages
    
  String connectStr="d:quickstart:yun:"+macAddrStr;
  connectStr.toCharArray(connectChars, connectStr.length() + 1);
  
  Console.println(connectStr);
  
  if (!mqtt.connect(connectChars)) {
    Console.println("error connecting");
  }
  
  // set up the ADC to read the internal temperature sensor
  setupTempSensor();
  
}

void loop()
{
  if (millis() > (time + 5000))
  {
    time = millis();
    
    float temp = getTemp();
    
    String pubString = "{\"d\":{\"temp\":" + String(temp) + "}}";
    
    Console.println(pubString+"->"+macAddrStr);

    pubString.toCharArray(pubChars, pubString.length() + 1);
    if (!mqtt.publish("iot-2/evt/status/fmt/json", pubChars)) {
      Console.println("publish error");
    }
  }
  
  if (!mqtt.loop()) {
      Console.println("loop error");
  }
}

void setupTempSensor() {
  setupADCFortempSensorReading();
  // throw away the first reading and wait a while -- as per the recommendations on the Atmel docs
  getTemp();
  delay(1);
}

void setupADCFortempSensorReading() {
 
  //ADC Multiplexer Selection Register
  ADMUX = 0;
  ADMUX |= (1 << REFS1);  //Internal 2.56V Voltage Reference with external capacitor on AREF pin
  ADMUX |= (1 << REFS0);  //Internal 2.56V Voltage Reference with external capacitor on AREF pin
  ADMUX |= (0 << MUX4);  //Temperature Sensor - 100111
  ADMUX |= (0 << MUX3);  //Temperature Sensor - 100111
  ADMUX |= (1 << MUX2);  //Temperature Sensor - 100111
  ADMUX |= (1 << MUX1);  //Temperature Sensor - 100111
  ADMUX |= (1 << MUX0);  //Temperature Sensor - 100111
 
  //ADC Control and Status Register A 
  ADCSRA = 0;
  ADCSRA |= (1 << ADEN);  //Enable the ADC
  ADCSRA |= (1 << ADPS2);  //ADC Prescaler - 16 (16MHz -> 1MHz)
 
  //ADC Control and Status Register B 
  ADCSRB = 0;
  ADCSRB |= (1 << MUX5);  //Temperature Sensor - 100111
}

double getTemp(){

  ADCSRA |= (1 << ADSC);  //Start temperature conversion
  while (bit_is_set(ADCSRA, ADSC));  //Wait for conversion to finish

  // We could report an precise number but accuracy is only +/-2% at best.
  // ADCW combines ADCL and ADCH into single 16 bit number
  //  double temperature = ADCW;
  //  return temperature - 273.4;
  
  // take an honest approach to reportimg the temp as we know it */
  byte low  = ADCL;
  byte high = ADCH;
  int temperature = (high << 8) | low;  //Result is in kelvin
  return temperature - 273;
}

/**
 * Get a mac address for this Yun board. Note the mac address of the wireless and the ethernet can be different
 * this code just gets a unique address for this device.
 */
String getMacAddressString() {
  Process p;
  
  // use the ethernet port. Could use the wireless port too.
  // the grep command simply looks for a string of the type DD:DD:DD:DD:DD:DD
  // the -o prints only the matched (non-empty) parts of matching lines, with each such part on a separate output line. 
  // we are passing the grep the output of the ifconfig for eth0 so we only expect one mac address to be found
 
  p.runShellCommand("ifconfig eth0 | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'");

  // do nothing until the process finishes, so you get the whole output:
  while (p.running());
  
  // if the process has finished and return no data then we have busted Yun.
  if (p.available() )
  { 
    return p.readString();
  }
  
  return "";
}

