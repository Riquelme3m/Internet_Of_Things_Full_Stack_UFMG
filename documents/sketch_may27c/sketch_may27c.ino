
#include<DHT.h>

DHT tempHumiSensor(25,DHT11);



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(2000);
  tempHumiSensor.begin();
 
  

}

void loop() {
  // put your main code here, to run repeatedly:
  float temp = tempHumiSensor.readTemperature();
  float humidity = tempHumiSensor.readHumidity();
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("°C");
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print(" % ");
  delay(2000);

}
