#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

#define BUTTONPIN  2   // Pin where the push button is connected
#define PIN        8   // Pin where LED is connected
#define NUMPIXELS  1   // Number of LEDs on the strip

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// BLE service & characteristics
BLEService buttonService("1e03ce00-b8bc-4152-85e2-f096236d2833");
BLEByteCharacteristic buttonStateChar(
  "1e03ce02-b8bc-4152-85e2-f096236d2833",
  BLERead | BLENotify
);

// New: RGB characteristic for LED color (3 bytes: R, G, B)
BLECharacteristic ledColorChar(
  "2a56f0a0-1234-4321-9abc-998877665544",
  BLERead | BLEWrite | BLENotify,
  3
);

bool buttonPressed = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(BUTTONPIN, INPUT_PULLUP);

  // init NeoPixel
  pixels.begin();
  pixels.setBrightness(50);
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();

  // start BLE
  if (!BLE.begin()) {
    Serial.println("BLE init failed!");
    while (1);
  }

  // register services & characteristics
  buttonService.addCharacteristic(buttonStateChar);
  buttonService.addCharacteristic(ledColorChar);
  BLE.addService(buttonService);

  // initial values
  buttonStateChar.writeValue((byte)0);
  uint8_t initColor[3] = {0, 0, 0};
  ledColorChar.writeValue(initColor, 3);

  // advertise
  BLE.setLocalName("BLEWebAPI-Sample");
  BLE.setAdvertisedService(buttonService);
  BLE.advertise();
  Serial.println("BLE ready, waiting for central...");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected: ");
    Serial.println(central.address());

    while (central.connected()) {
      // pump BLE stack
      BLE.poll();

      // ——— handle button notifications ———
      bool currentState = (digitalRead(BUTTONPIN) == LOW);
      if (currentState && !buttonPressed) {
        buttonStateChar.writeValue((byte)1);
        buttonPressed = true;
      }
      else if (!currentState && buttonPressed) {
        buttonStateChar.writeValue((byte)0);
        buttonPressed = false;
      }

      // ——— handle LED color writes ———
      if (ledColorChar.written()) {
        uint8_t rgb[3];
        ledColorChar.readValue(rgb, 3);
        Serial.print("Set LED to R=");
        Serial.print(rgb[0]);
        Serial.print(" G=");
        Serial.print(rgb[1]);
        Serial.print(" B=");
        Serial.println(rgb[2]);

        pixels.setPixelColor(0, pixels.Color(rgb[0], rgb[1], rgb[2]));
        pixels.show();

        // echo back current color
        ledColorChar.writeValue(rgb, 3);
      }

      delay(10);
    }

    Serial.println("Central disconnected");
  }
}