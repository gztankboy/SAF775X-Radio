#include "stdint.h"
#include "stdbool.h"

#define BLOCK_A    6
#define BLOCK_B    4
#define BLOCK_C    2
#define BLOCK_D    0

#define VERSION_A    0
#define VERSION_B    1

#define ReadyBit_PI    1
#define ReadyBit_PTY   2
#define ReadyBit_PS    4
#define ReadyBit_RT    8
#define ReadyBit_CT    16
#define ReadyBit_HB    128

#define RDS_LO    0
#define RDS_NORM  1
#define RDS_DX    2

struct RDSBuffer
{
  uint8_t status;
  uint16_t block_A;
  uint16_t block_B;
  uint16_t block_C;
  uint16_t block_D;
  uint16_t error;
};

struct RDSData
{
  uint8_t RDSFlag;
  uint16_t PI;             // Program Identification
  bool PI_Available;
  unsigned char PTY[9];    // Program Type
  unsigned char PS[9];     // Program Service name  [ 0A/B ]
  bool PS_Available;
  unsigned char RT[2][65]; // Radio Text  [ 2A/B ]
  int8_t RT_Type;          // Radio Text Type A:64CH B:32CH
  int8_t RT_Size[2];       // Radio Text Size
  uint8_t RT_Flag;         // 0->Text A, 1->Text B
  int8_t Hour;             // Clock-Time and Date  [ 4A ]
  int8_t Minute;           // -1: Not Availible
};


void RDS_Init(struct RDSData* init);
void RDS_Refresh(void);
void DecodeRDS(struct RDSBuffer *rawdata);




