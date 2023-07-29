#include <Adafruit_Fingerprint.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MLX90614.h>

// SD card
#define SD_CS 10

File entryFile;

// temperature variables
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
double temp_reading;

SoftwareSerial mySerial(2,3);
Adafruit_Fingerprint finger(&mySerial);



void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(100);
  Serial.println("\n\n=== Check Fingerprint ===");

  finger.begin(57600);
  delay(5);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Fingerprint sensor not ready");
    while(1) { delay(1); } // nonsense
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  // Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  // Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  // Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  // Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  // Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print(F("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example."));
  }
  else {
    Serial.println(F("Waiting for valid finger..."));
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }

  if (!SD.begin()) {
    Serial.println("Missing SD card");
    while(true);
  }

  // temperature sensor
  if (!mlx.begin()) {
    Serial.println("Error starting mlx sensor");
    // while(true);
  }
}

void loop() {
  // Ask if checking for ID or enrolling new ID
  // If checking for ID:
  //  kggGetFingerprintID and then search existing.
  //  if existing:
  //    get temperature, and add entry to SD card with temperature
  //  if not existing:
  //    return notification of rejected fingerprint, and advise to
  //      enroll or sign physically

  uint8_t fingerprint_status = FINGERPRINT_NOFINGER;

  Serial.println(F("Place your fingerprint to confirm entry"));
  while (kggGetFingerprintID() != FINGERPRINT_OK) {
    delay(1000); // no need to run too fast
  }
  Serial.println(F("Fingerprint taken"));

  if (kggSearchFinger() == -1) {
    Serial.print("Fingerprint found: ID #");
    Serial.println(finger.fingerID);
    // 1. Get temperature
    delay(20000);
    // temp_reading = mlx.readObjectTempC();
    // temp_reading = 32.2;

    // 2. Enter entry into sd card
    entryFile = SD.open("entry.csv", FILE_WRITE);
    entryFile.seek(EOF);

    if (entryFile) {
      entryFile.print(finger.fingerID);
      entryFile.print(",");
      entryFile.println(temp_reading);
      entryFile.close();  // Done storing entry
    } else {
      Serial.println("Error opening entry file");
    }

    Serial.println("Entry Registered...");
  } else {
    Serial.println("Fingerprint ID not found");
  }
  delay(10000);
}

uint8_t kggGetFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image taken"));
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(F("No finger detected"));
      delay(2000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image converted"));
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println(F("Image too messy"));
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println(F("Communication error"));
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println(F("Could not find fingerprint features"));
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!

  return p;
}

uint8_t kggSearchFinger() {
  uint8_t p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return -1;
}
