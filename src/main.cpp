#include <Arduino.h>
#include <ArduinoJson.h>
#include <ETH.h>
#include <HTTPClient.h>
#include <Adafruit_AHTX0.h>

#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_CLOCK_MODE ETH_CLOCK_GPIO0_IN

Adafruit_AHTX0 aht;
Adafruit_Sensor *aht_humidity, *aht_temp;

IPAddress local_ip(192, 168, 1, 8);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const char *serverAddress = "http://192.168.1.7:3000/api/graphql";

String createJsonPayload(int value, int sensor)
{
    JsonDocument doc;
    doc["query"] = "mutation CreateEntry($entryInput: EntryCreate!) { createEntry(entryInput: $entryInput) { _id address sensor value }}";

    JsonObject variables_entryInput = doc.createNestedObject("variables").createNestedObject("entryInput");
    variables_entryInput["address"] = "0013a20040050001";
    variables_entryInput["sensor"] = sensor;
    variables_entryInput["value"] = value;

    String queryString;
    serializeJson(doc, queryString);
    Serial.println(queryString);

    return queryString;
}

void sendPostRequest(const String &payload)
{
    HTTPClient http;
    http.begin(serverAddress);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.println("Response: " + response);
    }
    else
    {
        Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    http.end();
}

void initializeEthernet()
{
    ETH.begin();
    ETH.config(local_ip, gateway, subnet);

    Serial.print("Connecting to Ethernet");
    while (!ETH.linkUp())
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println(" Connected!");
}

void initializeSensors()
{
    if (!aht.begin())
    {
        Serial.println("AHT10/AHT20 not found");
        while (1)
        {
            delay(10);
        }
    }

    Serial.println("AHT10/AHT20 Found!");
    aht_temp = aht.getTemperatureSensor();
    aht_humidity = aht.getHumiditySensor();
}

void getSensorDataAndSend()
{
    sensors_event_t humidity, temp;
    aht_humidity->getEvent(&humidity);
    aht_temp->getEvent(&temp);

    Serial.println("Temperature: " + String(temp.temperature));
    Serial.println("Humidity: " + String(humidity.relative_humidity) + "%");

    String temperaturePayload = createJsonPayload(int(temp.temperature), 4);
    String humidityPayload = createJsonPayload(int(humidity.relative_humidity), 5);

    if (ETH.linkUp())
    {
        sendPostRequest(temperaturePayload);
        sendPostRequest(humidityPayload);
    }
    else
    {
        Serial.println("Ethernet Disconnected");
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }

    Serial.println("Starting setup...");

    initializeEthernet();
    initializeSensors();
    getSensorDataAndSend();
}

void loop()
{
    delay(1000);
    getSensorDataAndSend();
}