
#define PIN_TX 4

uint8_t msg[5];

uint8_t id = 0x2C;
uint8_t channel = 0x01;
bool batteryOK = 0;// 0 means OK
uint8_t humidity = 69;
float temperature = 1.;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_TX, OUTPUT);


}

void loop() {
  // put your main code here, to run repeatedly:
  temperature += 1.;
  if (temperature >= 35.) temperature = 1.;


  buildMsg();
  sendRF();

  delay(68000);

}

void buildMsg(){

  // put your setup code here, to run once:
  msg[0] = id;
  msg[4] = channel + ((humidity%10) << 4 );
  msg[3] = humidity/10;

  int binTemp = ((temperature * 9./5) + 32 + 90)*10;
  msg[3] += (binTemp & 0x0F) << 4;
  msg[2] = binTemp >> 4;

  msg[1] = batteryOK << 2;

  msg[1] = (msg[1] & 0x0F) | (msg[4] & 0x0F) << 4;
  // crc4() only works with full bytes
  uint8_t crc = crc4(msg, 4, 0x13, 0); // Koopmann 0x9, CCITT-4; FP-4; ITU-T G.704
  crc ^= msg[4] >> 4; // last nibble is only XORed

  msg[1] = (msg[1] & 0x0F) | (crc & 0x0F) << 4;

  for (int i=0; i<=4; i++){
    
    Serial.print(msg[i],HEX);
    Serial.print(" ");
  
  }
}

uint8_t crc4(uint8_t const message[], unsigned nBytes, uint8_t polynomial, uint8_t init)
{
    unsigned remainder = init << 4; // LSBs are unused
    unsigned poly = polynomial << 4;
    unsigned bit;

    while (nBytes--) {
        remainder ^= *message++;
        for (bit = 0; bit < 8; bit++) {
            if (remainder & 0x80) {
                remainder = (remainder << 1) ^ poly;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder >> 4 & 0x0f; // discard the LSBs
}

void sendRF()
{
/*low long 4000
low short 1800
high short 700
*/

  for (int n=0; n<6; n++)
  {
    for (int i=0; i<10; i++){
      digitalWrite(PIN_TX, !(i%2));
      delayMicroseconds(1000);
    }
    delayMicroseconds(7000);

    for (int i = 0; i<5; i++) {
      for (int b = 7; b>=0; b--) {
        digitalWrite(PIN_TX, HIGH);
        delayMicroseconds(700);
        digitalWrite(PIN_TX, LOW);        
        if (bitRead(msg[i], b)){
          delayMicroseconds(4000);
        }
        else{
          delayMicroseconds(1800);
        }
        
      }


    }
    digitalWrite(PIN_TX, HIGH);
    delayMicroseconds(1000);
    digitalWrite(PIN_TX, LOW);
    
    delayMicroseconds(16000);
  }
}
