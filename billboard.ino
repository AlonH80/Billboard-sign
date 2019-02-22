// Alon Hartanu 305143422

typedef unsigned char BYTE;   // To save space
// Pins
const BYTE DIN=10;
const BYTE CS=11;
const BYTE CLK=12;
const BYTE BUTTON_RECEIVE_RATE=2;
const BYTE BUTTON_RECEIVE_DIR=3;
// Constants
const int SAFE_INTERRUPT=300;
const int RATE_JUMPS=100;
const int MAX_RATE=400;
const int MIN_RATE=100;  
const BYTE MATRIX_SIZE=8;
const BYTE SIGN_LEN=18;
const BYTE SPACE=2;
const int INTENSITY=255;
// Variables for interrupts
volatile int rate=100;
enum Direction {toLeft=0,toRight=1};
volatile int dir=1;
long lastChangeCall=0;
long lastRaiseCall=0;

BYTE EMPTY_COLUMN=0b0;
// letters
BYTE EMPTY_MATRIX[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN};
BYTE HEY[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b11111111,0b11111111,0b11000000,0b11000000,0b11000000,0b11000000,0b11000111,0b11000111,EMPTY_COLUMN};
BYTE ALEF[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b11100011,0b11110111,0b00011110,0b00011100,0b00111000,0b01111000,0b11101111,0b11000111,EMPTY_COLUMN};
BYTE KUF[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b11111000,0b11111000,0b11000000,0b11000000,0b11000000,0b11011111,0b11011111,0b11000000,EMPTY_COLUMN};
BYTE DALED[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b11000000,0b11111111,0b11111111,0b11000000,0b11000000,0b11000000,0b11000000,0b11000000,EMPTY_COLUMN};
BYTE MEM[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b00011111,0b00111111,0b11100011,0b11000011,0b11100011,0b00110011,0b01111011,0b11011011,EMPTY_COLUMN};
BYTE YUD[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b00000000,0b00000000,0b11110000,0b11110000,0b11000000,0b11000000,0b00000000,0b00000000,EMPTY_COLUMN};
BYTE TAF[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b11111111,0b11111111,0b11000000,0b11000000,0b11111111,0b11111111,0b11000011,0b11000011,EMPTY_COLUMN};
BYTE LAMED[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b01110000,0b01111000,0b01101100,0b01100110,0b01100011,0b01100001,0b11100000,0b11100000,EMPTY_COLUMN};
BYTE BET[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b00000011,0b11111111,0b11111111,0b11000011,0b11000011,0b11000011,0b11000011,0b11000011,EMPTY_COLUMN};
BYTE PEY[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b11111111,0b11111111,0b11000011,0b11000011,0b11011011,0b11011011,0b11011011,0b11011111,EMPTY_COLUMN};
BYTE VAV[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,0b00000000,0b00000000,0b11111111,0b11111111,0b11000000,0b11000000,0b00000000,0b00000000,EMPTY_COLUMN};
BYTE MAKAF[MATRIX_SIZE+SPACE]={EMPTY_COLUMN,EMPTY_COLUMN,0b00011000,0b00011000,0b00011000,0b00011000,EMPTY_COLUMN,EMPTY_COLUMN,EMPTY_COLUMN};

// Sign
BYTE* sign[SIGN_LEN]={EMPTY_MATRIX,HEY,ALEF,KUF,DALED,MEM,YUD,TAF,TAF,LAMED,MAKAF,ALEF,BET,YUD,BET,YUD,PEY,VAV};
BYTE* currMatrix=EMPTY_MATRIX;
int lastCol=MATRIX_SIZE,firstCol=1;

// Functions
void initializeLedMatrix();
void writeColumn(BYTE col,BYTE colAr);
bool getBit(BYTE num,BYTE index);
void setIntensity(BYTE level);
void shiftMatrix(BYTE mat[MATRIX_SIZE],BYTE col,Direction dir);
void writeMatrix(BYTE mat[MATRIX_SIZE]);
// Rolling
void rollRight();
void rollLeft();
// Interrupts
void switchRate();
void changeDir();

void setup() {
  Serial.begin(9600); 
  pinMode(DIN,OUTPUT);
  pinMode(CS,OUTPUT);
  pinMode(CLK,OUTPUT);
  pinMode(BUTTON_RECEIVE_RATE,INPUT_PULLUP);
  pinMode(BUTTON_RECEIVE_DIR,INPUT_PULLUP);
  Serial.println("Turn led matrix On.");
  initializeLedMatrix();
  setIntensity(INTENSITY);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RECEIVE_RATE), switchRate, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RECEIVE_DIR), changeDir, FALLING);
  writeMatrix(currMatrix);
}

void loop() 
{
  while (dir==toRight)
     rollRight();

  while (dir==toLeft)
     rollLeft();
}

void initializeLedMatrix()
{
  Serial.println("Initializing matrix.."); 
  //  writeWord(0xC01); // Normal operation (not shutdown)
  for (BYTE i=0; i<4; i++) writeBit(LOW);
  for (BYTE i=0; i<2; i++) writeBit(HIGH);
  for (BYTE i=0; i<2; i++) writeBit(LOW);
  for (BYTE i=0; i<7; i++) writeBit(LOW);
  writeBit(HIGH);
  latchBuf();  

  //  writeWord(0xB07); // Scan limit
  for (BYTE i=0; i<4; i++) writeBit(LOW);
  writeBit(HIGH);
  writeBit(LOW);
  writeBit(HIGH);
  writeBit(HIGH);
  for (BYTE i=0; i<5; i++) writeBit(LOW);
  for (BYTE i=0; i<3; i++) writeBit(HIGH);
  latchBuf();
  Serial.println("Initialize successfully done.");
}

void setIntensity (BYTE level)
{
  for (BYTE i=0; i<4; i++) writeBit(LOW);
  writeBit(HIGH);
  writeBit(LOW);
  writeBit(HIGH);
  writeBit(LOW);
  for (BYTE i=8;i>0;i--)
    writeBit(getBit(level,i-1));
  latchBuf();
}

void writeBit(bool b) // Write 1 bit to the buffer
{
  digitalWrite(DIN,b);
  digitalWrite(CLK,LOW);
  digitalWrite(CLK,HIGH);
}

void latchBuf() // Latch the entire buffer
{
  digitalWrite(CS,LOW);
  digitalWrite(CS,HIGH);
}

void writeColumn(BYTE col,BYTE colAr)
{
  for (BYTE i=0;i<4;i++)
    writeBit(LOW);
  for (BYTE i=4;i>0;i--)
    writeBit(getBit(col+1,i-1));
  for (BYTE i=8;i>0;i--)
  {
    writeBit(getBit(colAr,i-1));
  }
  latchBuf();
}

bool getBit(BYTE num,BYTE index)
{
    bool returnVal=(num&(1<<index))>0;
    return returnVal; 
}

void writeMatrix(BYTE mat[MATRIX_SIZE])
{
  for (int i=0;i<MATRIX_SIZE;i++)
      writeColumn(i,mat[i]);
  delay(rate);
}

void shiftMatrix(BYTE mat[MATRIX_SIZE],BYTE col,Direction dir)
{
    // Enter col from <dir> side of the matrix.
    if (dir==toRight)
    {
      for (BYTE i=0;i<MATRIX_SIZE-1;i++)
          mat[i]=mat[i+1];
      mat[MATRIX_SIZE-1]=col;
    }
    if (dir==toLeft)
    {
      for (BYTE i=MATRIX_SIZE;i>0;i--)
          mat[i-1]=mat[i-2];
      mat[0]=col;
    }
}

void switchRate()
{
  if (millis()-SAFE_INTERRUPT>lastRaiseCall)
  {
     rate+=RATE_JUMPS;
     if (rate>MAX_RATE)
        rate=MIN_RATE;
  }
  lastRaiseCall=millis();
}

void changeDir()
{
   if (millis()-SAFE_INTERRUPT>lastChangeCall)
       dir=!dir;
   lastChangeCall=millis();
}

void rollRight()
{
    Serial.print("Direction: to right, Rate: ");
    Serial.println(rate);
    firstCol++;lastCol++;
    if ((lastCol/(MATRIX_SIZE+SPACE))==0)
        shiftMatrix(currMatrix,EMPTY_COLUMN,toRight);
    else
        shiftMatrix(currMatrix,sign[lastCol/(MATRIX_SIZE+SPACE)][lastCol%(MATRIX_SIZE+SPACE)],toRight);
    writeMatrix(currMatrix);
    if (lastCol==SIGN_LEN*(MATRIX_SIZE+SPACE)) lastCol=0; 
    if (firstCol==SIGN_LEN*(MATRIX_SIZE+SPACE)) firstCol=0; 
}

void rollLeft()
{
    Serial.print("Direction: to left, Rate: ");
    Serial.println(rate);
    firstCol--;lastCol--;
    if ((firstCol/(MATRIX_SIZE+SPACE))==0)
        shiftMatrix(currMatrix,EMPTY_COLUMN,toLeft);
    else
        shiftMatrix(currMatrix,sign[firstCol/(MATRIX_SIZE+SPACE)][firstCol%(MATRIX_SIZE+SPACE)],toLeft);
    writeMatrix(currMatrix);
    if (lastCol==0) lastCol=SIGN_LEN*(MATRIX_SIZE+SPACE)-1; 
    if (firstCol==0) firstCol=SIGN_LEN*(MATRIX_SIZE+SPACE)-1; 
}
