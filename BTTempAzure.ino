/*
The Circuit
*/

// BT Temp Azure project
// read data from the sensor
// send it to RPi2 via BT
// send to BT device
// save to SD if network is not avail

// using #define to reduce storage

#include "Wire.h"		// I2C = clock
#include "DHT.h"
#include "SoftwareSerial.h"

#include "SPI.h"
#include "SD.h"

// my clock specific functions
#include "Clock.h"

// is this needed here? this should be in the clock driver
#define DS1307_ADDRESS 0x68		// RTC

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN 4     // what digital pin we're connected to for the temp/humid sensor

#define bluetoothTx 2	// TX-O pin of bluetooth mate, Arduino D2 (TX = data that leaves the module)
#define bluetoothRx 3	// RX-I pin of bluetooth mate, Arduino D3 (RX = data that enters the module)

#define LED 6			// general use LED
#define DATA_LED 7		// data send LED, flash when data is sent.

#define LOG_NAME "log.txt"	// log file name
#define CHIP_SELECT 10		// SD - chip select pin
#define CARD_DETECT_PIN 5	// CD - card detect pin

// SD read/write objects
char *logFileName = LOG_NAME;
bool isCardDetected = false;	// is there a SD card insered, pin shorts to ground if card is inserted
bool sdAvail = false;			// is the SD reader available.
File myFile;

// temp data
unsigned long lastMillis = 0;		// millisecond counter since power on
const int pollingSeconds = 5000;	// polling interval
int lastMills = 0;

/*
Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT21   // DHT 21 (AM2301)

Connect pin 1 (on the left) of the sensor to +5V
NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
to 3.3V instead of 5V!
Connect pin 2 of the sensor to whatever your DHTPIN is
Connect pin 4 (on the right) of the sensor to GROUND
Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

Initialize DHT sensor.
Note that older versions of this library took an optional third parameter to
tweak the timings for faster processors.  This parameter is no longer needed
as the current DHT reading algorithm adjusts itself to work on faster procs.
*/

/*
SD card connect to arduino UNO via SPI (MOSI, MISO, SCK and CS)
CD - Card Detect pin. It shorts to ground when a card is inserted.
Connect a pull up resistor (10K or so) and wire this to another pin if you want to detect when a card is inserted.

Connect the 5V pin to the 5V pin on the Arduino
Connect the GND pin to the GND pin on the Arduino
Connect CLK to pin 13	(SPI Clock)
Connect DO to pin 12	(MISO)
Connect DI to pin 11	(MOSI)
Connect CS to pin 10	(Chip Select)
*/

// init the temp sensor
DHT dht(DHTPIN, DHTTYPE);

// init the serial (bluetooth) modem
SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

// init the clock
Clock *clock = new Clock();

// data structure for temp reading
struct TempReadings
{
	float tempF;
	float tempC;
	float humidity;
	float heatIndexF;
	float heatIndexC;
};

// clock returns an int for the weekday: 0 - 6
char *weekdayTable[] = { "Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat" };

TempReadings *tmpData = new TempReadings();

void setup()
{
	Wire.begin();

	pinMode(LED, OUTPUT);
	pinMode(DATA_LED, OUTPUT);
	pinMode(CARD_DETECT_PIN, INPUT_PULLUP);

	digitalWrite(LED, LOW);
	digitalWrite(DATA_LED, LOW);

	// sparkfun defaults to 115200 change it to 9600 no parity
	bluetooth.begin(115200);

	bluetooth.print("$");
	bluetooth.print("$");
	bluetooth.print("$");

	delay(2000);  // delay for ACK from BT

	bluetooth.println("U,9600,N"); // temp change to 9600 no parity

	// all this waiting, not good... undeterministic...
	delay(500);
	bluetooth.begin(9600);
	delay(8000);

	// check if the SD reader is avail TODO: move this to a function
	if (SD.begin(CHIP_SELECT))
	{
		// check if there is an the SD card inserted
		isCardDetected = digitalRead(CARD_DETECT_PIN);
		if (isCardDetected)
		{
			// routine for SD card file
			bluetooth.println("Checking SD card...");
			if (!doesLogExist())
			{
				bluetooth.println("Creating new log...");
				createFile();
			}
		}
	}
}

void loop()
{
	if (bluetooth.available())
	{
		//Serial.print((char)bluetooth.read());
		byte c = (char)bluetooth.read();

		if (c == 'h')
		{
			digitalWrite(LED, HIGH);
			bluetooth.println("On.");
		}
		if (c == 'l')
		{
			digitalWrite(LED, LOW);
			bluetooth.println("Off.");
		}
	}

	if (millis() - lastMillis > pollingSeconds)
	{
		// take environment readings and fetch time
		takeTempReading();
		lastMillis = millis();

		// get time
		clock->GetDateTime();

		// send data over bluetooth
		printDate();

		// save to SD card
		isCardDetected = digitalRead(CARD_DETECT_PIN);
		if (isCardDetected)
		{
			myFile = SD.open(logFileName, FILE_WRITE);
			if (myFile)
			{
				bluetooth.println("Writing data to SD card.");

				myFile.print(clock->hour);
				myFile.print(",");
				myFile.print(clock->minute);
				myFile.print(",");
				myFile.print(clock->second);
				myFile.print(",");
				myFile.print(clock->weekDay);
				myFile.print(",");
				myFile.print(clock->month);
				myFile.print(",");
				myFile.print(clock->monthDay);
				myFile.print(",");
				myFile.print(clock->year);
				myFile.print(",");
				myFile.print(tmpData->tempF);
				myFile.print(",");
				myFile.print(tmpData->tempC);
				myFile.print(",");
				myFile.print(tmpData->humidity);
				myFile.print(",");
				myFile.print(tmpData->heatIndexF);
				myFile.print(",");
				myFile.print(tmpData->heatIndexC);

				myFile.close();
			}
			else
			{
				bluetooth.println("File Not Avail.");
				// call routine to flash Data LED
			}
		}
		else
		{
			// retry this: myFile = SD.open(logFileName, FILE_WRITE);
			bluetooth.println("No SD Card Detected.");
		}
	}
}

void takeTempReading()
{
	// take humidity reading
	tmpData->humidity = dht.readHumidity();

	// Read temperature as Celsius (the default)
	tmpData->tempC = dht.readTemperature();

	// Read temperature as Fahrenheit (isFahrenheit = true)
	tmpData->tempF = dht.readTemperature(true);

	if (isnan(tmpData->humidity) || isnan(tmpData->tempC) || isnan(tmpData->tempF))   // isnan = is not a number from the math library
	{
		bluetooth.println("Failed to read from DHT sensor!");
		return;
	}

	// Compute heat index in Fahrenheit (the default)
	tmpData->heatIndexF = dht.computeHeatIndex(tmpData->tempF, tmpData->humidity);

	// Compute heat index in Celsius (isFahreheit = false)
	tmpData->heatIndexC = dht.computeHeatIndex(tmpData->tempC, tmpData->humidity, false);

	bluetooth.print("Humidity:");
	bluetooth.print(tmpData->humidity);
	bluetooth.print(" %\t");

	bluetooth.print("Temp: ");
	bluetooth.print(tmpData->tempC);
	bluetooth.print(" *C ");

	bluetooth.print(tmpData->tempF);
	bluetooth.print(" *F\t");
	bluetooth.print("Heat Index: ");

	bluetooth.print(tmpData->heatIndexC);
	bluetooth.print(" *C ");

	bluetooth.print(tmpData->heatIndexF);
	bluetooth.print(" *F\r\n");

	// blink the LED
	flashLed();
}

void flashLed()
{
	digitalWrite(DATA_LED, HIGH);
	delay(200);
	digitalWrite(DATA_LED, LOW);
}

bool doesLogExist()
{
	if (SD.exists(logFileName))
	{
		bluetooth.println("Log exists.");
		sdAvail = true;
		return true;
	}
	else
	{
		bluetooth.println("Log file not found.");
		return false;
	}
}

void createFile()
{
	myFile = SD.open(logFileName, FILE_WRITE);
	myFile.close();

	bluetooth.println("Verifying log is created...");
	if (doesLogExist())
	{
		bluetooth.println("Log created.");
		sdAvail = true;
	}
	else
	{
		bluetooth.println("FAIL - Log not created.");
	}
}

// clock methods
void printDate()
{
	// this is for console print. Data should be serialized as it is stored on SD
	bluetooth.print(clock->month);
	bluetooth.print("/");
	bluetooth.print(clock->monthDay);
	bluetooth.print("/");
	bluetooth.print(clock->year);
	bluetooth.print(" ");
	bluetooth.print(clock->hour);
	bluetooth.print(":");
	bluetooth.print(clock->minute);
	bluetooth.print(":");
	bluetooth.print(clock->second);
	bluetooth.print(" ");
	bluetooth.println(weekdayTable[clock->weekDay]);
}

void sendData()
{
	/*
	char buf[2];
	buf[0] = (clock->month + '0');
	buf[1] = 'h';
	Serial.println(buf);
	// sending CSV format TODO: header file needs schema
	*/

}

bool cardExist()
{
	bool isPresent = digitalRead(CARD_DETECT_PIN);
	if (isPresent)
	{
		return true;
	}
	else
	{
		return false;
	}
}