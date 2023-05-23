#ifndef _TOF_MEASURE_H_
#define _TOF_MEASURE_H_

int tof_open();
int tof_close();
int tof_continuous_measure(unsigned short* result_buf, int count);
unsigned short tof_single_measure();
void tof_ref_register_test();

#endif
