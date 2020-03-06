// tejedor klemm 01

#include <limits.h>
#include <SPI.h>


#define LATCH 6           // ROWS and COLS
#define COLS_DATA 7
#define COLS_SH   8

#define col_0 B00000001
#define col_1 B00000010
#define col_2 B00000100
#define col_3 B00001000
#define col_4 B00010000
#define col_5 B00100000
#define col_6 B01000000
#define col_7 B10000000

uint16_t cols[] = { col_1, col_2, col_3, col_4, col_5, col_6, col_7, col_0,
                    col_1, col_2, col_3, col_4, col_5, col_6, col_7, col_0
                  };

uint16_t value = 0;

#define NUM_COLS 16
#define NUM_CHIPS 12

uint16_t data[96] = {};  // 96 filas

unsigned long iterations = 0;
unsigned long l;
// http://unixwiz.net/techtips/reading-cdecl.html
unsigned long count = 0;
unsigned long ptime = 0;
unsigned long dtime = 0;

unsigned int latchTime = 10; // us  no tiene sentido en este programa porque toda la secuencia toma mas tiempo...

int frame = 0;

boolean canUpdate = true;
uint16_t frameBuffer[96] = {};
long showTime = 0;
long idleTime = 0;
int timeMult = 1;
int linesErased = 0;

int ANIM_START = 0;
int ANIM_END = 95;

void setup() {

  pinMode(LATCH, OUTPUT);
  pinMode(COLS_DATA, OUTPUT);
  pinMode(COLS_SH, OUTPUT);

  digitalWrite(SS, HIGH);  // ensure SS stays high
  Serial.begin(115200);
  SPI.begin ();

  SPI.setBitOrder(MSBFIRST); // escribo de abajo para arriba
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider (SPI_CLOCK_DIV8);

  Serial.println(115200);

  getRandomSeed();

  iterations = random(10E6);


  for (int i = 0; i < 96; i++) {
    data[i] = diex(iterations + i, 0);
  }

  fill_n(data, 96, UINT_MAX);
  update();
}


const int IDLE = -1;
const int KNIT = 0;
const int SHOW = 1;
const int ERASE = 2;

int state = IDLE;

void loop() {

  switch (state) {
    case IDLE:
      if(millis() > idleTime) state = KNIT;
      Serial.println(F("idle"));
      break;

    case KNIT:
      if (ptime + dtime < millis()) {
        dtime = random(0, 25* timeMult);
        ptime = millis();
        animation();
      }
      break;

    case SHOW:
      Serial.println(F("show"));
      if (millis() > showTime) state = ERASE;
      break;

    case ERASE:
      erase();
      Serial.println(F("erase"));
      break;
  }
  render();
}


void getRandomSeed()
{
  uint16_t seed = 0;
  for (int i = 0; i < 1E3; i++) seed += seed ^ analogRead(random(6));
  randomSeed(seed);
}

void erase() {  
  for (int i = 0; i < 96; i++) {
    frameBuffer[i] = frameBuffer[i + 1];
  }
  frameBuffer[95] = 0;
  linesErased++;
  if (linesErased > ANIM_END) {
    state = IDLE;
    linesErased = 0;
    idleTime = millis() + 3E3 * timeMult;
  }
}

inline void animation() {  
  renderLine();
//  renderLine2();
  if (frame == ANIM_END) {
    state = SHOW;
    frame = 0;
    update();
    showTime = random(4E3, 7E3) + millis();
  }
  
  Serial.println(frame); 
}


int knot = 0;
int inc = 1;

inline void renderLine() {
  if(knot >= 0 && knot < 16){        
    knot += inc;
    int value = data[frame] >> knot & 1;
    frameBuffer[frame] |= value << knot;
  }else{
    inc *= -1;
    knot += inc;
    frame++ % 96;
  }
}


void renderLine2() {
  if(knot >= 0 && knot < 16){        
    knot += inc;
//    int value = data[frame] >> knot & 1;
    frameBuffer[frame] = (frame % 2 == 0) ? data[frame] << knot : data[frame] << 16 - knot;
  }else{
    inc *= -1;
    knot += inc;
    frame++ % 96;
  }
}


void copy(uint16_t fb[], uint16_t data[], uint16_t time) {
  fb[time] = data[time];
}

void update() {
  iterations += 96;
  for (int i = 0; i < 96; i++) {
    data[i] = diex(iterations + i, 0);
  }
}

void fill_n(uint16_t arr[], int length, uint16_t value) {
  for (int i = 0 ; i < length; i++) {
    arr[i] = value;
  }
}


inline void render() {
  for (int column = 0 ; column < 16; column++) {
    for (int board = 2; board >= 0; board--) {
      //      SPI.transfer((1 << column));
      //      SPI.transfer((1 << (7 - (column % 8))) );
      //      SPI.transfer(0);
      //      SPI.transfer(0);
      SPI.transfer(getBits(board, 0, column));
      SPI.transfer(getBits(board, 1, column));
      SPI.transfer(getBits(board, 2, column));
      SPI.transfer(getBits(board, 3, column));
      //        SPI.transfer(1);SPI.transfer(1);SPI.transfer(1);SPI.transfer(1); // 31 32 33 34
      //        SPI.transfer(1);SPI.transfer(1);SPI.transfer(1);SPI.transfer(1); // 21 22 23 24
      //        SPI.transfer(1);  SPI.transfer(1);SPI.transfer(1);SPI.transfer(1);   // 11 12 13 14
    }
    latch(column);
  }
}

uint8_t getBits(int board, int chip, int column) {
  uint8_t value = 0;
  for (int i = 0; i < 8; i++) {
    value |= (frameBuffer[board * 32 + chip * 8 + i] >> column) & 1;
    if (i != 7) value <<= 1;
  }
  return value;
}
void latch(int column) {
  // comentarios para lado derecho
  shiftOut(COLS_DATA, COLS_SH, LSBFIRST, column < 8 ? cols[column] : 0);     // izquierda  // conector 2
  shiftOut(COLS_DATA, COLS_SH, LSBFIRST, column >= 8 ? cols[column % 8] : 0); // derecha // conector 1
  digitalWrite(LATCH, LOW);
  delayMicroseconds(latchTime);
  digitalWrite(LATCH, HIGH);
}

static inline uint16_t diex(long t, int formula)
{
  unsigned ut = unsigned(t);


  uint16_t val = ut * (( ut >> 1 | ut >> 7));

  return val;

  //  switch (formula) {
  //    case 0:
  //      return ut * (((ut >> 12) | (ut >> 8)) & (31 & (ut >> 4))); //1
  //      break;
  //    case 1:
  //      return ut * (((ut >> 12) & (ut >> 8)) ^ (31 & (ut >> 3)));
  //      break;
  //    case 2:
  //      return ut * (((ut >> 23) & (ut >> 13)) ^ (19 & (ut >> 5)));
  //      break;
  //  }

}






unsigned char reverse(unsigned char b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}




void black() {
  for (int column = 0 ; column < NUM_COLS; column++) {
    SPI.transfer(0);
    SPI.transfer(0);
    latch(column);
  }
}





