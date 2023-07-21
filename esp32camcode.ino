/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-take-photo-display-web-server/
  
  IMPORTANT!!! 
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <ESPAsyncWebSrv.h>
#include <StringArray.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ctime>

// Replace with your network credentials
const char* ssid = "yourWifi";
const char* password = "yourPassword";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

boolean takeNewPhoto = false;

// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASHLIGHT_PIN     4

const int LEDC_CHANNEL = 2;      // Replace with the appropriate LEDC channel number
const int LEDC_RESOLUTION = 8;   // Adjust the LEDC resolution (bit depth)
const int FLASHLIGHT_ON = 10;   // Adjust the brightness level when flashlight is on

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>//FRONTEND
<html>
<head>
  <title>Text Detection Example - Magic: The Gathering</title>
  <style>
    body {
      font-family: 'Arial', sans-serif;
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: #151515;
      color: #fff;
    }

    h1 {
      text-align: center;
      margin-bottom: 30px;
      font-size: 36px;
      letter-spacing: 2px;
      text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);
    }

    button {
      display: block;
      width: 100%;
      margin: 10px 0;
      padding: 15px;
      font-size: 18px;
      font-weight: bold;
      letter-spacing: 1px;
      background-color: #dc4c00;
      color: #fff;
      border: none;
      cursor: pointer;
      border-radius: 30px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      transition: background-color 0.3s ease;
    }

    button:hover {
      background-color: #c13900;
    }

    .photo-container {
      text-align: center;
      margin-bottom: 20px;
    }

    img {
      max-width: 100%;
      border: 1px solid #ddd;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      transition: box-shadow 0.3s ease;
    }

    img:hover {
      box-shadow: 0 8px 16px rgba(0, 0, 0, 0.3);
    }

    #cardInfo {
      padding: 20px;
      background-color: #202020;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      transition: background-color 0.3s ease;
    }

    #cardInfo h2 {
      margin-bottom: 15px;
      text-align: center;
      font-size: 24px;
      text-transform: uppercase;
      letter-spacing: 2px;
      color: #dc4c00;
    }

    #cardInfo p {
      margin: 10px 0;
      font-size: 18px;
    }

    #cardInfo span {
      font-weight: bold;
      color: #fff;
    }

    #cardInfo span.card-name {
      color: #dc4c00;
      text-decoration: underline;
    }
  </style>
</head>
<body>
  <h1>Text Detection Example - Magic: The Gathering</h1>
  <div class="photo-container">
    <button onclick="capturePhoto()">CAPTURE PHOTO</button>
    <button onclick="uploadFile().catch(console.error)">Upload</button>
    <button onclick="location.reload();">REFRESH PAGE</button>
    <div><img src="saved-photo" alt="Captured Photo"></div>
  </div>
  <div id="cardInfo">
    <h2>Card Information</h2>
    <p><span class="card-name">Name:</span> <span id="cardName">N/A</span></p>
    <p><span>Price:</span> <span id="cardPrice">N/A</span></p>
    <p><span>Artist:</span> <span id="cardArtist">N/A</span></p>
  </div>// END FRONTEND
  <script>
 function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
// The ID of your GCS bucket
    const bucketName = 'cards-from-scanner';

// The local file path to upload
    


    async function deleteImage() {
      const apiUrl = `https://storage.googleapis.com/storage/v1/b/${bucketName}/o/${encodeURIComponent(fileName)}`;

      const response = await fetch(apiUrl, {
        method: 'DELETE',
      });

      if (response.ok) {
        console.log(`Image ${filePath} deleted from ${bucketName}`);
      } else {
        console.error('Error deleting image:', response.statusText);
      }
    }



    async function uploadFile() {
      const folder = 'saved-photo/';
      const date = new Date();
      const day = date.getDate();
      const month = date.getMonth() + 1;
      const year = date.getFullYear();
      const hours = date.getHours();
      const minutes = date.getMinutes();
      const seconds = date.getSeconds();
      const fname = `photo_${day}-${month}-${year}_${hours}-${minutes}-${seconds}`;
      const fileEnd = '.jpg';
      const filePath = folder + fname + fileEnd;
      
        const project = 'esp32-cam'; // Replace with your project ID
       

        const apiUrl = `https://storage.googleapis.com/upload/storage/v1/b/${bucketName}/o?uploadType=media&name=${encodeURIComponent(filePath)}`;

        const response = await fetch(filePath);
        const fileData = await response.blob();

        const uploadUrl = apiUrl.replace('uploadType=media', 'uploadType=resumable');
        
        const uploadHeaders = {
          'X-Upload-Content-Type': response.headers.get('Content-Type'),
          'X-Upload-Content-Length': response.headers.get('Content-Length'),
        };

        const uploadResponse = await fetch(uploadUrl, {
          method: 'POST',
          headers: uploadHeaders,
        });

        const uploadLocation = uploadResponse.headers.get('Location');

        const uploadDataResponse = await fetch(uploadLocation, {
          method: 'PUT',
          body: fileData,
        });

        if (uploadDataResponse.ok) {
          console.log(`File ${filePath} uploaded to ${bucketName}`);
        } else {
          console.error('Error uploading file:', uploadDataResponse.statusText);
        }
        performTextDetection(filePath);
      }




    async function performTextDetection(filePath) {
      const imageUrl = "gs://cards-from-scanner/" + filePath; // Replace with the URL of your image
      
      // Perform text detection using Google Cloud Vision API  & replace with your key
      const response = await fetch("https://vision.googleapis.com/v1/images:annotate?key=AIzaSyB7jr59dhKCoK9R9f9idx4Y_vhG0UyAWcQ", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          requests: [
            {
              image: {
                source: {
                  imageUri: imageUrl,
                },
              },
              features: [
                {
                  type: "TEXT_DETECTION",
                },
              ],
            },
          ],
        }),
      });

      const data = await response.json();
      if (data.responses && data.responses.length > 0 && data.responses[0].textAnnotations) {
        const textAnnotations = data.responses[0].textAnnotations;
        if (textAnnotations.length > 0) {
          const firstLine = textAnnotations[0].description.split('\n')[0];
          const urlprefix = "https://api.scryfall.com/cards/named?fuzzy=";
          const urlsuffix = firstLine.replace(/ /g, "+");
          const url = urlprefix + urlsuffix;
          console.log(url);

          // Perform HTTP request to Scryfall API
          fetch(url)
            .then(response => response.json())
            .then(data => {
              // filterung out which info from the json is for us important
        if (data.name) {
          document.getElementById('cardName').textContent = data.name;
        } else {
          document.getElementById('cardName').textContent = 'N/A';
        }
        if (data.prices && data.prices.usd) {
          document.getElementById('cardPrice').textContent = `$${data.prices.usd}`;
        } else {
          document.getElementById('cardPrice').textContent = 'N/A';
        }
        if (data.artist) {
          document.getElementById('cardArtist').textContent = data.artist;
        } else {
          document.getElementById('cardArtist').textContent = 'N/A';
        }
      })
            .catch(error => console.error("Error:", error));
        } else {
          console.log("No lines of text found in the image.");
        }
      } else {
        console.log("No text detected in the image.");
      }
    }
    




  </script>
</body>
</html>)rawliteral";

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
   
    // Print ESP32 Local IP Address
    Serial.print("IP Address: http://");
    Serial.println(WiFi.localIP());

    // Turn-off the 'brownout detector'
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // OV2640 camera module
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    /*
    sensor_t * s = esp_camera_sensor_get();
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    
    s->set_vflip(s, 1);          // 0 = disable , 1 = enable
    /*s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  */

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
    s->set_hmirror(s, 0);
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 0); // up the brightness just a bit
    s->set_saturation(s, 0); // lower the saturation
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    takeNewPhoto = true;
    request->send_P(200, "text/plain", "Taking Photo");
  });

  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });

  pinMode(FLASHLIGHT_PIN, OUTPUT);
  digitalWrite(FLASHLIGHT_PIN, LOW);
  // Start server
  server.begin();

}

bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

 
void capturePhotoSaveSpiffs( void ) {

  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    ledcSetup(LEDC_CHANNEL, 5000, LEDC_RESOLUTION); // Configure LEDC settings
    ledcAttachPin(FLASHLIGHT_PIN, LEDC_CHANNEL);    // Attach LEDC pin
    ledcWrite(LEDC_CHANNEL, FLASHLIGHT_ON);      

    //digitalWrite(FLASHLIGHT_PIN, HIGH);
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    digitalWrite(FLASHLIGHT_PIN, LOW);
    
    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      
      
      file.write(fb->buf, fb->len); // payload (image), payload length
      
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
  //ledcWrite(LEDC_CHANNEL, 0); 
}


void loop() {
  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    takeNewPhoto = false;
  }
  delay(1);
}
