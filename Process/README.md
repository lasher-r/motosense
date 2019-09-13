# Process

 *  Multiplexing the IR sensors.


## Multiplexing the IR sensors.

The [ir sensor](https://www.adafruit.com/product/1748) I got is an I2C device which makes things easy to wire up.  Just power the sensor, connect the I2C pins and add pull up resistors and your done.  The only hard part is making sure you know what pin is what, there's a little tab to help orientate the sensor.

The code is straightforward:

```C
#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 irTempSensor = Adafruit_MLX90614();

void setup() {
  irTempSensor.begin();
}

void loop() {
  double objC = irTempSensor.readObjectTempC();
  double ambC = irTempSensor.readAmbientTempC();
  double objF = irTempSensor.readObjectTempF(); //this is the call that I'm intersted in.
  double ambF = irTempSensor.readAmbientTempF();
}
```

Except it has a fixed address of 0x5A and I need two.  Notice that using the library I never set the address; It's hard coded in `Adafruit_MLX90614.h`.  So I got an [I2C multiplexer](https://www.adafruit.com/product/2717).

The wiring was still relatively simple, just add the multiplexer between the sensors and the arduino:

![tca](20190601_153602.jpg "tca")

The code needs a few changes:


Add the wire lib:
```C
#include <Wire.h>
extern "C" { 
#include "utility/twi.h"  // from Wire library, so we can do bus scanning
}
```

Add a tca select helper function to select what sensor connected to the multiplexer we want:
```C
void tcaselect(uint8_t i) {
  if (i > 7) return;
  
  //0x70 is default address for multiplexer
  // You can change it on the multiplexer by shorting the 
  // A0-A2 pins
  Wire.beginTransmission(0x70); 
  Wire.write(1 << i);
  Wire.endTransmission();  
}
```

Then when you want to call one of the sensors you need to use tcaselect() with what pins that sensor is on.

example:
```C
tcaselect(2);
irTemp1.begin(); // start the sensor on pins sda2/scl2
tcaselect(7);
irTemp2.begin(); // start the sensor on pins sda7/scl7

// ...

//  get temp of sensor1 from pins 2 then 7
tcaselect(2);
double temp1 = irTemp1.readObjectTempF();
tcaselect(7);
double temp2 = irTemp2.readObjectTempF();
```