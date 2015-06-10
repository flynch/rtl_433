#include "rtl_433.h"

static float
get_temperature (uint8_t * msg)
{
  uint32_t temp_f = msg[4] & 0x7f;
  temp_f <<= 4;
  temp_f |= ((msg[5] & 0xf0) >> 4);
  temp_f -= 400;
  if (msg[4] & 0x80) {
    temp_f = -temp_f;
  }
  return (temp_f / 10.0f);
}

static uint8_t
get_humidity (uint8_t * msg)
{
  uint8_t humidity = ( (msg[5] & 0x0f) << 4) | ((msg[6] & 0xf0) >> 4);
  return humidity;
}

static uint8_t 
calculate_checksum (uint8_t *buff, int length)
{
  uint8_t mask = 0x7C;
  uint8_t checksum = 0x64;
  uint8_t data;
  int byteCnt;	

  for (byteCnt=0; byteCnt < length; byteCnt++)
  {
    int bitCnt;
    data = buff[byteCnt];

    for (bitCnt = 7; bitCnt >= 0 ; bitCnt--)
    {
      uint8_t bit;

      // Rotate mask right
      bit = mask & 1;
      mask =  (mask >> 1 ) | (mask << 7);
      if ( bit )
      {
	mask ^= 0x18;
      }

      // XOR mask into checksum if data bit is 1	
      if ( data & 0x80 )
      {
	checksum ^= mask;
      }
      data <<= 1; 
    }
  }
  return checksum;
}

static int
validate_checksum (uint8_t * msg, int len)
{
  uint8_t expected = ((msg[6] & 0x0f) << 4) | ((msg[7] & 0xf0) >> 4);
  
  uint8_t pkt[5];
  pkt[0] = ((msg[1] & 0x0f) << 4) | ((msg[2] & 0xf0) >> 4);
  pkt[1] = ((msg[2] & 0x0f) << 4) | ((msg[3] & 0xf0) >> 4);
  pkt[2] = ((msg[3] & 0x0f) << 4) | ((msg[4] & 0xf0) >> 4);
  pkt[3] = ((msg[4] & 0x0f) << 4) | ((msg[5] & 0xf0) >> 4);
  pkt[4] = ((msg[5] & 0x0f) << 4) | ((msg[6] & 0xf0) >> 4);
  uint8_t calculated = calculate_checksum (pkt, 5);

  if (expected == calculated)
    return 0;
  else {
    fprintf(stderr, "Checksum error in Ambient Weather message.  Expected: %02x  Calculated: %02x\n", expected, calculated);
    fprintf(stderr, "Message: "); int i; for (i=0; i<len; i++) fprintf(stderr, "%02x ", msg[i]); fprintf(stderr, "\n\n");
    return -1;
  }
}

static uint16_t
get_device_id (uint8_t * msg)
{
  uint16_t deviceID = ( (msg[2] & 0x0f) << 4) | ((msg[3] & 0xf0)  >> 4);
  return deviceID;
}

static uint16_t
get_channel (uint8_t * msg)
{
  uint16_t channel = (msg[3] & 0x07) + 1;
  return channel;
}

static int
ambient_weather_parser (uint8_t bb[BITBUF_ROWS][BITBUF_COLS], int16_t bits_per_row[BITBUF_ROWS])
{
  /* shift all the bits left 1 to align the fields */
  int i;
  for (i = 0; i < BITBUF_COLS-1; i++) {
    uint8_t bits1 = bb[0][i] << 1;
    uint8_t bits2 = (bb[0][i+1] & 0x80) >> 7;
    bits1 |= bits2;
    bb[0][i] = bits1;
  }

  /* DEBUG: print out the received packet */
  /*
  fprintf(stderr, "\n! ");
  for (i = 0 ; i < BITBUF_COLS ; i++) {
    fprintf (stderr, "%02x ", bb[0][i]);
  }
  fprintf (stderr,"\n\n");
  */

  if ( (bb[0][0] == 0x00) && (bb[0][1] == 0x14) && (bb[0][2] & 0x50) ) {
    fprintf (stderr, "\nSensor Temperature Event:\n");
    fprintf (stderr, "protocol      = Ambient Weather\n");

    if (validate_checksum (bb[0], BITBUF_COLS)) {
      return 0;
    }
    
    float temperature = get_temperature (bb[0]);
    fprintf (stderr, "temp          = %.1f\n", temperature);

    uint8_t humidity = get_humidity (bb[0]);
    fprintf (stderr, "humidity      = %d\n", humidity);
  
    uint16_t channel = get_channel (bb[0]);
    fprintf (stderr, "channel       = %d\n", channel);

    uint16_t deviceID = get_device_id (bb[0]);
    fprintf (stderr, "id            = %d\n", deviceID);

  } 

  return 0;
}

static int
ambient_weather_callback (uint8_t bb[BITBUF_ROWS][BITBUF_COLS], int16_t bits_per_row[BITBUF_ROWS])
{
  return ambient_weather_parser (bb, bits_per_row);
}

r_device ambient_weather = {
    /* .id             = */ 14,
    /* .name           = */ "Ambient Weather Temperature Sensor",
    /* .modulation     = */ OOK_MANCHESTER,
    /* .short_limit    = */ 125,
    /* .long_limit     = */ 0, // not used
    /* .reset_limit    = */ 600,
    /* .json_callback  = */ &ambient_weather_callback,
    /* .disabled       = */ 0
};
