/*

  Transmissions includes an id. Every 60 seconds the sensor transmits 6 packets:

    0000 1111 | 0011 0000 | 0101 1100 | 1110 0111 | 0110 0001
    iiii iiii | cccc ub?? | tttt tttt | tttt hhhh | hhhh ??nn

  - i: identification // changes on battery switch
  - c: CRC-4 // CCITT checksum, see below for computation specifics
  - u: unknown // (sometimes set at power-on, but not always)
  - b: battery low // flag to indicate low battery voltage
  - h: Humidity // BCD-encoded, each nibble is one digit, 'A0' means 100%rH
  - t: Temperature // in °F as binary number with one decimal place + 90 °F offset
  - n: Channel // Channel number 1 - 3
*/

int old_duration = 0;
int new_duration = 0;

unsigned long old_time = 0;
unsigned long new_time = 0;

boolean old_status = LOW;
boolean new_status = LOW;
boolean started = false;
byte input = 2;

uint8_t msg[5];
byte indexBit = 0;



void setup() {
  // put your setup code here, to run once:
  pinMode(input, INPUT_PULLUP);
  Serial.begin(500000);
}

void loop() {
  // put your main code here, to run repeatedly:
  receive();

}

void receive() {
  new_status = digitalRead(input);
  if (new_status != old_status) {
    new_time = micros();
    new_duration = new_time - old_time;

    if (!new_status && !started) {
      if (durationEquals(old_duration, 7850, 100) && (durationEquals(new_duration, 600, 100)) ) {// __________|"| //that's a start
        started = true;
      }
    }
    else if (new_status && started) {//right after rising edge

      if (durationEquals(old_duration, 600, 100)) {
        if (new_duration < 2700) {// it's a 0
          bitWrite(msg[indexBit / 8], 7 - (indexBit % 8), 0);
        }
        else {// it's a 1
          bitWrite(msg[indexBit / 8], 7 - (indexBit % 8), 1);
        }
        indexBit++;
      }
      else { // abort if high level duration is not correct
        started = false;
        indexBit = 0;
      }
      if (indexBit >= 40) {// stop when 40 bits are received
        started = false;
        indexBit = 0;

        if (infactory_crc_check()) {
          decode();
        }
        else {
          Serial.println("CRC Error");
        }
      }
    }

    old_time = new_time;
    old_duration = new_duration;
  }
  old_status = new_status;
}

void decode() {
  byte iD = msg[0];
  bool battery_low = (msg[1] >> 2) & 1;
  int  temp_raw    = (msg[2] << 4) | (msg[3] >> 4);
  byte humidity    = (msg[3] & 0x0F) * 10 + (msg[4] >> 4); // BCD, 'A0'=100%rH
  byte channel     = msg[4] & 0x03;
  float temp_f      = ((float)temp_raw * 0.1 - 122) * 5 / 9.;
  Serial.print("iD=");
  Serial.print(iD, HEX);
  Serial.print("; lo_bat=");
  Serial.print(battery_low);
  Serial.print("; hum=");
  Serial.print(humidity);
  Serial.print("; chn=");
  Serial.print(channel);
  Serial.print("; tmp=");
  Serial.println(temp_f);
}

bool durationEquals(int duration1, int duration2, int tolerance) {
  if (abs(duration1 - duration2) <= tolerance) return true;
  else return false;
}

void printHex(byte b) {
  if (b < 0x10) Serial.print("0");
  Serial.print(b, HEX);
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


bool infactory_crc_check() {
  uint8_t msg_crc, crc;
  msg_crc = msg[1] >> 4;
  // for CRC computation, channel bits are at the CRC position(!)
  msg[1] = (msg[1] & 0x0F) | (msg[4] & 0x0F) << 4;
  // crc4() only works with full bytes
  crc = crc4(msg, 4, 0x13, 0); // Koopmann 0x9, CCITT-4; FP-4; ITU-T G.704
  crc ^= msg[4] >> 4; // last nibble is only XORed
  return (crc == msg_crc);
}
