#ifndef esp1_sec_h
#define esp1_sec_h

#define DEVICE_NAME "XXXXXXXXXXX"
#define DEVICE_LOCAL "Predio_XXXXX"
#define AWS_IOT_ENDPOINT "XXXXXXXXXX.iot.XXXXXXXXXXX.amazonaws.com" // The MQTTT endpoint for the device (unique for each AWS account but shared amongst devices within the account)
#define AWS_IOT_PUBLISH_TOPIC   "pub/risco/"DEVICE_LOCAL // The MQTT topic that this device should publish to
#define AWS_IOT_SUBSCRIBE_TOPIC "sub/risco/"DEVICE_LOCAL
#define CHAVE_TELEGRAM "XXXXXXXXXXXXXXXXXXXXXXX"  
#define CHAVE_TS "XXXXXXXXXXXXXXXX" // chave privada para escrita canal ThingSpeak
#define CANAL_TS XXXXXXXXXXX //chave do canal ThingSpeak
const int DEVICE_ID = XXXXXXXXXXX;

const char *WIFI_SSID = "XXXXXXXXXXX";
const char *WIFI_PASSWORD = "XXXXXXXXXXX";

// Amazon's root CA. This should be the same for everyone.
const char AWS_CERT_CA[] = "-----BEGIN CERTIFICATE-----\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"-----END CERTIFICATE-----\n";

// The private key for your device ESPX
const char AWS_CERT_PRIVATE[] = "-----BEGIN RSA PRIVATE KEY-----\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"-----END RSA PRIVATE KEY-----\n";

// The certificate for your device ESPX
const char AWS_CERT_CRT[] = "-----BEGIN CERTIFICATE-----\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXX\n" \
"XXXXXXXXXXX\n" \
"-----END CERTIFICATE-----\n";

#endif