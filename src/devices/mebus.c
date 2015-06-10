#include "rtl_433.h"

static int mebus433_callback(uint8_t bb[BITBUF_ROWS][BITBUF_COLS], int16_t bits_per_row[BITBUF_ROWS]) {
    int temperature_before_dec;
    int temperature_after_dec;
    int16_t temp;
    int8_t  hum;

    if (bb[0][0] == 0 && bb[1][4] !=0 && (bb[1][0] & 0b01100000) && bb[1][3]==bb[5][3] && bb[1][4] == bb[12][4]){
	// Upper 4 bits are stored in nibble 1, lower 8 bits are stored in nibble 2
	// upper 4 bits of nibble 1 are reserved for other usages.
        temp = (int16_t)((uint16_t)(bb[1][1] << 12 ) | bb[1][2]<< 4);
        temp = temp >> 4;
	// lower 4 bits of nibble 3 and upper 4 bits of nibble 4 contains
	// humidity as decimal value
	hum  = (bb[1][3] << 4 | bb[1][4] >> 4);

        temperature_before_dec = abs(temp / 10);
        temperature_after_dec = abs(temp % 10);

        fprintf(stdout, "Sensor event:\n");
        fprintf(stdout, "protocol       = Mebus/433\n");
        fprintf(stdout, "address        = %i\n", bb[1][0] & 0b00011111);
        fprintf(stdout, "channel        = %i\n",((bb[1][1] & 0b00110000) >> 4)+1);
        fprintf(stdout, "battery        = %s\n", bb[1][1] & 0b10000000?"Ok":"Low");
        fprintf(stdout, "unkown1        = %i\n",(bb[1][1] & 0b01000000) >> 6); // always 0?
        fprintf(stdout, "unkown2        = %i\n",(bb[1][3] & 0b11110000) >> 4); // always 1111?
        fprintf(stdout, "temperature    = %s%d.%d°C\n",temp<0?"-":"",temperature_before_dec, temperature_after_dec);
        fprintf(stdout, "humidity       = %i%%\n", hum);
        fprintf(stdout, "%02x %02x %02x %02x %02x\n",bb[1][0],bb[1][1],bb[1][2],bb[1][3],bb[1][4]);

        if (debug_output)
            debug_callback(bb, bits_per_row);

        return 1;
    }
    return 0;
}

r_device mebus433 = {
    /* .id             = */ 10,
    /* .name           = */ "Mebus 433",
    /* .modulation     = */ OOK_PWM_D,
    /* .short_limit    = */ 300,
    /* .long_limit     = */ 600,
    /* .reset_limit    = */ 1500,
    /* .json_callback  = */ &mebus433_callback,
    /* .json_callback  = */ //&debug_callback,
    /* .disabled       = */ 0
};
