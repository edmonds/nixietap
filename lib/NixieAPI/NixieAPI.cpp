#include "NixieAPI.h"

NixieAPI::NixieAPI() {
}
void NixieAPI::applyKey(String key, uint8_t selectAPI) {
    switch(selectAPI) {
        case 0 : 
                timezonedbKey = key;
                #ifdef DEBUG
                    Serial.println("applyKey successful, timezonedb key is: " + timezonedbKey);
                #endif // DEBUG
                break;
        case 1 : 
                ipStackKey = key;
                #ifdef DEBUG
                    Serial.println("applyKey successful, ipstack key is:  " + ipStackKey);
                #endif // DEBUG
                break;
        case 2 : 
                googleLocKey = key;
                #ifdef DEBUG
                    Serial.println("applyKey successful, Google location key is:  " + googleLocKey);
                #endif // DEBUG
                break;
        case 3 : 
                googleTimeZoneKey = key;
                #ifdef DEBUG
                    Serial.println("applyKey successful, Google timezone key is:  " + googleTimeZoneKey);
                #endif // DEBUG
                break;
        default: 
                Serial.println("Unknown value of selectAPI!");
                break;
    }
}
String NixieAPI::MACtoString(uint8_t* macAddress) {
    char macStr[18] = { 0 };
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
    return  String(macStr);
}
/*                                                          *
 *  Function to get a list of surrounding WiFi signals in   *
 *  JSON format to get location via Google Location API.    *
 *                                                          */
String NixieAPI::getSurroundingWiFiJson() {
    String wifiArray = "[\n";
    int8_t numWifi = WiFi.scanNetworks();
    #ifdef DEBUG
        Serial.println(String(numWifi) + " WiFi networks found.");
    #endif // DEBUG
    for(uint8_t i = 0; i < numWifi; i++) {
        // Serial.print("WiFi.BSSID(i) = ");
        // Serial.println((char *)WiFi.BSSID(i));
        wifiArray += "{\"macAddress\":\"" + MACtoString(WiFi.BSSID(i)) + "\",";
        wifiArray += "\"signalStrength\":" + String(WiFi.RSSI(i)) + ",";
        wifiArray += "\"channel\":" + String(WiFi.channel(i)) + "}";
        if (i < (numWifi - 1)) {
            wifiArray += ",\n";
        }
    }
    WiFi.scanDelete();
    wifiArray += "]";
    #ifdef DEBUG
        Serial.println("WiFi list :\n" + wifiArray);
    #endif // DEBUG
    return wifiArray;
}
/*                                                      *
 * Calls the ipify API to get a public IP address.      *
 * If that fails, it tries the same with the seeip API. * 
 * https://www.ipify.org/   https://seeip.org/          *
 *                                                      */
String NixieAPI::getPublicIP() {
    //Add a SSL client
    WiFiClient client;
    String headers = "", body = "", ip = "";
    bool finishedHeaders = false, currentLineIsBlank = false, gotResponse = false, bodyStarts = false, bodyEnds = false;
    long timeout;
    if(client.connect("api.ipify.org", 80)) {
        #ifdef DEBUG
            Serial.println("Connected to ipify.org!");
        #endif // DEBUG
        client.print("GET /?format=json HTTP/1.1\r\nHost: api.ipify.org\r\n\r\n");
        timeout = millis() + MAX_CONNECTION_TIMEOUT;
        // checking the timeout
        while(client.available() == 0) {
            if(timeout - millis() < 0) {
                #ifdef DEBUG
                    Serial.println("Client Timeout!");
                #endif // DEBUG
                client.stop();
                break;
            }
        }
        if(client.available()) {
            //marking we got a response
            gotResponse = true;
        }
    } 
    if(!gotResponse && client.connect("ip.seeip.org", 80)) {
        #ifdef DEBUG
            Serial.println("Failed to fetch public IP address from ipify.org! Now trying with seeip.org.");
            Serial.println("Connected to seeip.org!");
        #endif // DEBUG
        client.print("GET /json HTTP/1.1\r\nHost: ip.seeip.org\r\n\r\n");
        timeout = millis() + MAX_CONNECTION_TIMEOUT;
        // checking the timeout
        while(client.available() == 0) {
            if(timeout - millis() < 0) {
                #ifdef DEBUG
                    Serial.println("Client Timeout!");
                #endif // DEBUG
                client.stop();
                break;
            }
        }
        if(client.available()) {
            //marking we got a response
            gotResponse = true;
        }
    } 
    if(gotResponse) {
        while(client.available()) {
            char c = client.read();
            if(finishedHeaders) {   // Separate json file from rest of the received response.
                if(c == '{') {      // If this additional filtering is not performed. seeip.org json response will not be accepted by ArduinoJson lib.
                    bodyStarts = true;
                }
                if(bodyStarts && !bodyEnds) {
                    body = body + c;
                }
                if(c == '}') {
                    bodyEnds = true;
                }
            } else {
                if(currentLineIsBlank && c == '\n') {
                    finishedHeaders = true;
                }
                else {
                    headers = headers + c;
                }
            }
            if(c == '\n') {
                currentLineIsBlank = true;
            } else if(c != '\r') {
                currentLineIsBlank = false;
            }
        }
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(body);
        if(root.success()) {
            ip = root["ip"].as<String>();
            #ifdef DEBUG
                Serial.println("Your Public IP location is: " + ip);
            #endif // DEBUG
            return ip;
        } else {
            #ifdef DEBUG
                Serial.println("Failed to parse JSON!");
            #endif // DEBUG
        }
    } else {
        #ifdef DEBUG
            Serial.println("Failed to fetch public IP address from ipify.org!");
            Serial.println("Failed to obtain a public IP address from any API!");
        #endif // DEBUG
        return ip;
    }

    return ip;
}
/*                                                    *
 *  Calls IPStack API to get latitude and longitude   *
 *  coordinates relative to your public IP location.  *
 *  The API can automatically detect your public IP   *
 *  address without the need to send it.              *
 *  Free up to 10.000 request per month.              *
 *  https://ipstack.com/                              *
 *                                                    */
String NixieAPI::getLocFromIpstack(String publicIP) {
    HTTPClient http;
    String payload = "", location = "";
    if(publicIP == "") {
        publicIP = "check";
    }
    String URL = "http://api.ipstack.com/" + publicIP + "?access_key=" + ipStackKey + "&output=json&fields=country_name,region_name,city,latitude,longitude";
    http.setUserAgent(UserAgent);
    if(!http.begin(URL)) {
        #ifdef DEBUG
            Serial.println(F("getLocFromIpstack: Connection failed!"));
        #endif // DEBUG
    } else {
        #ifdef DEBUG
            Serial.println("Connected to api.ipstack.com!");
        #endif // DEBUG
        int stat = http.GET();
        if(stat > 0) {
            if(stat == HTTP_CODE_OK) {
                payload = http.getString();
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(payload);
                if(root.success()) {
                    String country = root["country_name"];
                    String region = root["region_name"];
                    String city = root["city"];
                    String lat = root["latitude"];
                    String lng = root["longitude"];
                    location = lat + "," + lng;
                    #ifdef DEBUG
                        Serial.print("Your IP location is: " + country + ", " + region + ", " + city + ". ");
                        Serial.println("With coordinates: latitude: " + lat + ", " + "longitude: " + lng);
                    #endif // DEBUG
                } else {
                    #ifdef DEBUG
                        Serial.println(F("getLocFromIpstack: JSON parse failed!"));
                        Serial.println(payload);
                    #endif // DEBUG
                }
            } else {
                #ifdef DEBUG
                    Serial.printf("getLocFromIpstack: [HTTP] GET reply %d\r\n", stat);
                #endif // DEBUG
            }
        } else {
            #ifdef DEBUG
                Serial.printf("getLocFromIpstack: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
            #endif // DEBUG
        }
    }
    http.end();

    return location;
}
/*                                                                     *
 *  Calls Google Location API to get current location using            *
 *  surrounding WiFi signals information.                              *
 *  Free up to 2,500 requests per day.                                 *
 *  $0.50 USD / 1,000 additional requests.                             *
 *  up to 100,000 daily, if billing is enabled.                        *
 *  https://developers.google.com/maps/documentation/geolocation/intro *
 *                                                                     */
String NixieAPI::getLocFromGoogle() {
    WiFiClientSecure client;
    String location = "", lat = "", lng = "", accuracy = "";
    String headers = "", hull = "", response = "";
    bool finishedHeaders = false, currentLineIsBlank = false, gotResponse = false;
    const char* googleLocApiHost = "www.googleapis.com";
    const char* googleLocApiUrl = "/geolocation/v1/geolocate";
    if(client.connect(googleLocApiHost, 443)) {
        #ifdef DEBUG
            Serial.println("Connected to Google Location API endpoint!");
        #endif // DEBUG
    } else {
        #ifdef DEBUG
            Serial.println("getLocFromGoogle: HTTPS error!");
        #endif // DEBUG
        return location;
    }
    String body = "{\"wifiAccessPoints\":" + getSurroundingWiFiJson() + "}";
    #ifdef DEBUG
        Serial.println("requesting URL: " + String(googleLocApiUrl) + "?key=" + googleLocKey);
    #endif // DEBUG
    String request = String("POST ") + String(googleLocApiUrl);
    if(googleLocKey != "") 
        request += "?key=" + googleLocKey;
    request += " HTTP/1.1\r\n";
    request += "Host: " + String(googleLocApiHost) + "\r\n";
    request += "User-Agent: " + String(UserAgent) + "\r\n";
    request += "Content-Type:application/json\r\n";
    request += "Content-Length:" + String(body.length()) + "\r\n";
    request += "Connection: close\r\n\r\n";
    request += body;
    #ifdef DEBUG
        Serial.println("Request: \n" + request);
    #endif // DEBUG
    client.println(request);
    #ifdef DEBUG
        Serial.println("getLocFromGoogle: Request sent!");
    #endif // DEBUG
    // Wait for response
    long timeout = millis() + MAX_CONNECTION_TIMEOUT;
    // checking the timeout
    while(client.available() == 0) {
        if(timeout - millis() < 0) {
            #ifdef DEBUG
                Serial.println("getLocFromGoogle: Client Timeout!");
            #endif // DEBUG
            client.stop();
            break;
        }
    }
    while(client.available()) {
        char c = client.read();
        if(finishedHeaders) {
            hull = hull + c;
        } else {
            if(currentLineIsBlank && c == '\n') {
                finishedHeaders = true;
            }
            else {
                headers = headers + c;
            }
        }
        if(c == '\n') {
            currentLineIsBlank = true;
        } else if(c != '\r') {
            currentLineIsBlank = false;
        }
        gotResponse = true;
    }
    if(gotResponse) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(hull);
        if(root.success()) {
            accuracy = root["accuracy"].as<String>();
            lat = root["location"]["lat"].as<String>(); 
            lng = root["location"]["lng"].as<String>();
            location = lat + "," + lng;
            #ifdef DEBUG
                Serial.println("Your location is: " + location + " \nThe accuracy of the estimated location is: " + accuracy + "m");
            #endif // DEBUG
        } else {
            #ifdef DEBUG
                Serial.println("getLocFromGoogle: Failed to parse JSON!");
            #endif // DEBUG
        }
    }
    return location;
}
/*                                                     *
 *  Calls ip-api API to get latitude and longitude     *
 *  coordinates relative to your public IP location.   *
 *  Free up to 150 request per minute.                 *
 *  http://ip-api.com/                                 *
 *                                                     */
String NixieAPI::getLocFromIpapi(String publicIP) {
    HTTPClient http;
    String payload = "", location = "";
    String URL = "http://ip-api.com/json/" + publicIP;
    http.setUserAgent(UserAgent);
    if(!http.begin(URL)) {
        #ifdef DEBUG
            Serial.println(F("getLocFromIpapi: Connection failed!"));
        #endif // DEBUG
    } else {
        #ifdef DEBUG
            Serial.println("Connected to ip-api.com!");
        #endif // DEBUG
        int stat = http.GET();
        if(stat > 0) {
            if(stat == HTTP_CODE_OK) {
                payload = http.getString();
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(payload);
                if(root.success()) {
                    String country = root["country"];
                    String region = root["region"];
                    String city = root["city"];
                    String lat = root["lat"];
                    String lng = root["lon"];
                    location = lat + "," + lng;
                    #ifdef DEBUG
                        Serial.print("Your IP location is: " + country + ", " + region + ", " + city + ". ");
                        Serial.println("With coordinates: latitude: " + lat + ", " + "longitude: " + lng);
                    #endif // DEBUG
                } else {
                    #ifdef DEBUG
                        Serial.println(F("getLocFromIpapi: JSON parse failed!"));
                        Serial.println(payload);
                    #endif // DEBUG
                }
            } else {
                #ifdef DEBUG
                    Serial.printf("getLocFromIpapi: [HTTP] GET reply %d\r\n", stat);
                #endif // DEBUG
            }
        } else {
            #ifdef DEBUG
                Serial.printf("getLocFromIpapi: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
            #endif // DEBUG
        }
    }
    http.end();

    return location;
}
/*                                                            *
 *  Calls IPStack API to get time zone parameters according   *
 *  to your public IP location. The API can automatically     *
 *  detect your public IP address without the need to send it.*
 *  To get the time zone parameters from this API,you need    *
 *  to buy their services.  https://ipstack.com/              *
 *                                                            */
int NixieAPI::getTimeZoneOffsetFromIpstack(time_t now, String publicIP, uint8_t *dst) {
    HTTPClient http;
    int tz = 0;
    if(publicIP == "") {
        publicIP = "check";
    }
    String URL = "http://api.ipstack.com/" + publicIP + "?access_key=" + ipStackKey + "&output=json&fields=time_zone.id,time_zone.gmt_offset,time_zone.is_daylight_saving";
    String payload, tzname;
    http.setUserAgent(UserAgent);
    if(!http.begin(URL)) {
        #ifdef DEBUG
            Serial.println(F("getIpstackTimeZoneOffset: Connection failed!"));
        #endif // DEBUG
    } else {
        #ifdef DEBUG
            Serial.println("Connected to api.ipstack.com!");
        #endif // DEBUG
        int stat = http.GET();
        if(stat > 0) {
            if(stat == HTTP_CODE_OK) {
                payload = http.getString();
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(payload);
                if(root.success()) {
                    tz = ((root["gmt_offset"].as<int>()) / 60);  // Time Zone offset in minutes.
                    *dst = root["is_daylight_saving"].as<int>(); // DST ih hours.
                    tzname = root["id"].as<String>();
                    #ifdef DEBUG
                        Serial.println("Your Time Zone name is:" + tzname + " (Offset from UTC: " + String(tz) + ")");
                        Serial.printf("Is DST(Daylight saving time) active at your location: %s", *dst == 1 ? "Yes (+1 hour)" : "No (+0 hour)");
                    #endif // DEBUG
                } else {
                    #ifdef DEBUG
                        Serial.println(F("getTimeZoneOffset: JSON parse failed!"));
                        Serial.println(payload);
                    #endif // DEBUG
                }
            } else {
                #ifdef DEBUG
                    Serial.printf("getTimeZoneOffset: [HTTP] GET reply %d\r\n", stat);
                #endif // DEBUG
            }
        } else {
            #ifdef DEBUG
                Serial.printf("getTimeZoneOffset: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
            #endif // DEBUG
        }
    }
    http.end();

    return tz;
}
/*                                                                  *
 *  Calls Google TimeZone API to get time zone parameters           *
 *  for your latitude and longitude location.                       *
 *  Free up to 2,500 requests per day.                              *
 *  $0.50 USD / 1,000 additional requests.                          *
 *  up to 100,000 daily, if billing is enabled.                     *
 *  https://developers.google.com/maps/documentation/timezone/start *
 *                                                                  */
int NixieAPI::getTimeZoneOffsetFromGoogle(time_t now, String location, uint8_t *dst) {
    HTTPClient http;
    int tz = 0;
    String URL = "https://maps.googleapis.com/maps/api/timezone/json?location=" + location + "&timestamp=" + String(now) + "&key=" + googleTimeZoneKey;
    #ifdef DEBUG
        Serial.println("Requesting URL: " + URL);
    #endif // DEBUG
    String payload, tzName, tzId;
    http.setIgnoreTLSVerifyFailure(true);   // https://github.com/esp8266/Arduino/pull/2821
    http.setUserAgent(UserAgent);
    if(!http.begin(URL, googleTimeZoneCrt)) {
        #ifdef DEBUG
            Serial.println(F("getTimeZoneOffset: [HTTP] connect failed!"));
        #endif // DEBUG
    } else {
        int stat = http.GET();
        if(stat > 0) {
            if(stat == HTTP_CODE_OK) {
                payload = http.getString();
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(payload);
                if(root.success()) {
                    tz = ((root["rawOffset"].as<int>()) / 60);  // Time Zone offset in minutes.
                    *dst = ((root["dstOffset"].as<int>()) / 3600); // DST ih hours.
                    tzName = root["timeZoneName"].as<String>();
                    tzId = root["timeZoneId"].as<String>();
                    #ifdef DEBUG
                        Serial.println("Your Time Zone name is:" + tzName + " (Offset from UTC: " + String(tz) + ") at location: " + tzId);
                        Serial.printf("Is DST(Daylight saving time) active at your location: %s", *dst == 1 ? "Yes (+1 hour)" : "No (+0 hour)");     
                    #endif // DEBUG
                } else {
                    #ifdef DEBUG
                        Serial.println(F("getTimeZoneOffset: JSON parse failed!"));
                        Serial.println(payload);
                    #endif // DEBUG
                }
            } else {
                #ifdef DEBUG
                    Serial.printf("getTimeZoneOffset: [HTTP] GET reply %d\r\n", stat);
                #endif // DEBUG
            }
        } else {
            #ifdef DEBUG
            Serial.printf("getTimeZoneOffset: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
            #endif // DEBUG
        }
    }
    http.end();

    return tz;
}
/*                                                                      *
 *  Calls timezonedb.com API to get time zone parameters for            *
 *  your latitude and longitude location.                               *
 *  API is free for personal and non-commercial usage.                  *
 *  There is a rate limit where you can only send request to            *
 *  the server once per second. Additional queries will get blocked.    *
 *  With premium packet you can get time zone parameters by IP address. *
 *  https://timezonedb.com/                                             *
 *                                                                      */
int NixieAPI::getTimeZoneOffsetFromTimezonedb(time_t now, String location, String ip, uint8_t *dst) {
    HTTPClient http;
    int tz = 0;
    String URL, payload, tzname;
    location.replace(",", "&lng="); // This API request this format of coordinates: &lat=45.0&lng=19.0
    if(ip == "")
        URL = "http://api.timezonedb.com/v2/get-time-zone?key=" + timezonedbKey + "&format=json&by=position&position&lat=" + location + "&time=" + String(now) + "&fields=zoneName,gmtOffset,dst";
    else 
        URL = "http://api.timezonedb.com/v2/get-time-zone?key=" + timezonedbKey + "&format=json&by=ip&ip=" + ip + "&time=" + String(now) + "&fields=zoneName,gmtOffset,dst";
    http.setUserAgent(UserAgent);
    if(!http.begin(URL)) {
        #ifdef DEBUG
            Serial.println(F("getTimeZoneOffsetFromTimezonedb: Connection failed!"));
        #endif // DEBUG
    } else {
        #ifdef DEBUG
            Serial.println("Connected to api.timezonedb.com!");
        #endif // DEBUG
        int stat = http.GET();
        if(stat > 0) {
            if(stat == HTTP_CODE_OK) {
                payload = http.getString();
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(payload);
                if(root.success()) {
                    tz = ((root["gmtOffset"].as<int>()) / 60);  // Time Zone offset in minutes.
                    *dst = root["dst"].as<int>(); // DST ih hours.
                    if(*dst) {
                        tz -= 60;
                    }
                    tzname = root["zoneName"].as<String>();
                    #ifdef DEBUG
                        Serial.println("Your Time Zone name is:" + tzname + " (Offset from UTC: " + String(tz) + ")");
                        Serial.printf("Is DST(Daylight saving time) active at your location: %s", *dst == 1 ? "Yes (+1 hour)" : "No (+0 hour)");
                    #endif // DEBUG
                } else {
                    #ifdef DEBUG
                        Serial.println(F("getTimeZoneOffsetFromTimezonedb: JSON parse failed!"));
                        Serial.println(payload);
                    #endif // DEBUG
                }
            } else {
                #ifdef DEBUG
                    Serial.printf("getTimeZoneOffsetFromTimezonedb: [HTTP] GET reply %d\r\n", stat);
                #endif // DEBUG
            }
        } else {
            #ifdef DEBUG
                Serial.printf("getTimeZoneOffsetFromTimezonedb: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
            #endif // DEBUG
        }
    }
    http.end();

    return tz;
}

NixieAPI nixieTapAPI = NixieAPI();