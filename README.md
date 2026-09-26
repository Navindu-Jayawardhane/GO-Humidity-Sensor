# GO Humidity Sensor — Simple Firebase + GitHub Pages

This version intentionally removes Firebase email/password authentication.

## Data flow

GO sensor
-> Wheatstone bridge
-> amplifier
-> ESP32 GPIO34
-> Wi-Fi
-> Firebase Realtime Database
-> GitHub Pages
-> live graph

## Important

This is a prototype configuration. The Firebase rules allow unauthenticated reads and writes.

That is convenient for initial testing but is NOT appropriate for a production/public deployment because anyone who knows the database URL could potentially write data.

For the research prototype, use this to verify the complete data path first. After it works, secure the database before publishing it widely.

## Sensor calculation

The original working ADC conversion is preserved:

    int adcValue = analogRead(ADC_PIN);
    float voltage = adcValue * (3.3 / 4095.0);

There is:

- no DHT sensor
- no voltage-to-RH conversion
- no artificial humidity value
- no smoothing/filtering

This keeps the electrical response visible for calibration experiments.

## Setup

### Firebase

1. Create a Firebase project.
2. Create a Realtime Database.
3. Set the Realtime Database rules using `firebase/database.rules.json`.
4. Add a Web App in Firebase.
5. Copy the Web App configuration into `website/index.html`.
6. Copy the database URL into the ESP32 sketch.

### ESP32

Edit:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `FIREBASE_DATABASE_URL`

Install the ArduinoJson library and upload the sketch.

### GitHub Pages

Upload the `website` files to a GitHub repository and enable GitHub Pages.

## Research calibration

Do not assume a voltage-to-RH equation.

Instead collect paired measurements:

sensor voltage + reference RH + temperature + experimental condition

Then fit and validate a calibration model from the measured data.
