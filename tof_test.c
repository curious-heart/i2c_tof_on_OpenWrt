#include <malloc.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

#include "tof_measure.h"

#define MAX_CHAR_NUM_READ 16
int main(int argc, char **argv)
{
    int ret;
    bool end;
    char r_buf[MAX_CHAR_NUM_READ + 1];

    ret = tof_open();
    if(ret != 0)
    {
        printf("tof_open error: %d\n", ret);
        return ret;
    }

    end = false;
    while(!end)
    {
        int op;
        printf("choose operation:\n");
        printf("0: test reference register.\n");
        printf("1: single measurement.\n");
        printf("2: continuous measurement.\n");
        printf("3: endless measurement.\n");
        printf("-1: exit.\n");
        fgets(r_buf, sizeof(r_buf), stdin);
        sscanf(r_buf, "%d", &op);
        switch(op)
        {
            case 0:
                tof_ref_register_test();
                break;

            case 1:
                {
                    unsigned short meas_result;
                    meas_result = tof_single_measure();
                    printf("Single measurement result: %d.\n", meas_result);
                }
                break;

            case 2:
                {
                    unsigned short * meas_result;
                    int count;
                    float interval;
                    int ret;

                    printf("please input measure count(%d~%d)  and interval in seconds(%.02f~%.02f):",
                    TOF_CONTI_MEAS_MIN_COUNT,
                    TOF_CONTI_MEAS_MAX_COUNT,
                    (double)TOF_CONTI_MEAS_MIN_INTERVAL,
                    (double)TOF_CONTI_MEAS_MAX_INTERVAL);

                    fgets(r_buf, sizeof(r_buf), stdin);
                    sscanf(r_buf, "%d %f", &count, &interval);
                    if(count < TOF_CONTI_MEAS_MIN_COUNT) count = TOF_CONTI_MEAS_MIN_COUNT;
                    if(count > TOF_CONTI_MEAS_MAX_COUNT) count = TOF_CONTI_MEAS_MAX_COUNT;
                    if(interval < TOF_CONTI_MEAS_MIN_INTERVAL) interval = TOF_CONTI_MEAS_MIN_INTERVAL;
                    if(interval > TOF_CONTI_MEAS_MAX_INTERVAL) interval = TOF_CONTI_MEAS_MAX_INTERVAL;
                    meas_result = (unsigned short*)malloc(count);
                    if(!meas_result)
                    {
                        printf("Try to measure continuously, but malloc error.\n");
                        break;
                    }
                    ret = tof_continuous_measure(meas_result, count, interval);
                    if(ret <= 0)
                    {
                        printf("get no meaurenets...\n");
                    }
                    else
                    {
                        int i;
                        printf("get %d measurements.\n", ret);
                        for(i = 0; i < ret; ++i)
                        {
                            printf("%d: %d\n", i,  meas_result[i]);
                        }
                    }
                    free(meas_result);
                }
                break;

            case 3:
                {
                    float interval;
                    printf("please input interval in seconds(%.02f~%.02f):",
                                (double)TOF_CONTI_MEAS_MIN_INTERVAL,
                                (double)TOF_CONTI_MEAS_MAX_INTERVAL);
                    fgets(r_buf, sizeof(r_buf), stdin);
                    sscanf(r_buf, "%f", &interval);
                    tof_continuous_measure(NULL, -1, interval);
                }
                break;

            case -1:
                end = true;
                break;

            default:
                break;
        }
    }
    
    ret = tof_close();
    if(ret != 0)
    {
        printf("tof_close error: %d\n", ret);
    }
    return ret;
}
