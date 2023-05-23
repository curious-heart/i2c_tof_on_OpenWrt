#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "vl53l0x_i2c.h"
#include <malloc.h>
#include <stdbool.h>
#include <unistd.h>

#include "tof_measure.h"


void print_pal_error(VL53L0X_Error Status){
    char buf[VL53L0X_MAX_STRING_LENGTH];
    VL53L0X_GetPalErrorString(Status, buf);
    printf("API Status: %i : %s\n", Status, buf);
}

void print_range_status(VL53L0X_RangingMeasurementData_t* pRangingMeasurementData){
    char buf[VL53L0X_MAX_STRING_LENGTH];
    uint8_t RangeStatus;

    /*
     * New Range Status: data is valid when pRangingMeasurementData->RangeStatus = 0
     */

    RangeStatus = pRangingMeasurementData->RangeStatus;

    VL53L0X_GetRangeStatusString(RangeStatus, buf);
    printf("Range Status: %i : %s\n", RangeStatus, buf);

}


VL53L0X_Error WaitMeasurementDataReady(VL53L0X_DEV Dev) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t NewDatReady=0;
    uint32_t LoopNb;

    // Wait until it finished
    // use timeout to avoid deadlock
    if (Status == VL53L0X_ERROR_NONE) {
        LoopNb = 0;
        do {
            Status = VL53L0X_GetMeasurementDataReady(Dev, &NewDatReady);
            if ((NewDatReady == 0x01) || Status != VL53L0X_ERROR_NONE) {
                break;
            }
            LoopNb = LoopNb + 1;
            VL53L0X_PollingDelay(Dev);
        } while (LoopNb < VL53L0X_DEFAULT_MAX_LOOP);

        if (LoopNb >= VL53L0X_DEFAULT_MAX_LOOP) {
            Status = VL53L0X_ERROR_TIME_OUT;
        }
    }

    return Status;
}

VL53L0X_Error WaitStopCompleted(VL53L0X_DEV Dev) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint32_t StopCompleted=0;
    uint32_t LoopNb;

    // Wait until it finished
    // use timeout to avoid deadlock
    if (Status == VL53L0X_ERROR_NONE) {
        LoopNb = 0;
        do {
            Status = VL53L0X_GetStopCompletedStatus(Dev, &StopCompleted);
            if ((StopCompleted == 0x00) || Status != VL53L0X_ERROR_NONE) {
                break;
            }
            LoopNb = LoopNb + 1;
            VL53L0X_PollingDelay(Dev);
        } while (LoopNb < VL53L0X_DEFAULT_MAX_LOOP);

        if (LoopNb >= VL53L0X_DEFAULT_MAX_LOOP) {
            Status = VL53L0X_ERROR_TIME_OUT;
        }
	
    }

    return Status;
}
    
static const char* gs_i2c_dev_name = "/dev/i2c-0";
static const u8 gs_i2c_tof_8b_addr = 0x52;

int main(int argc, char **argv)
{
    int ret;
    bool end;

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
        scanf("%d", &op);
        switch(op)
        {
            case 0:
                tof_ref_register_test();
                break;

            case 1:
                {
                    uint16_t meas_result;
                    meas_result = tof_single_measure();
                    printf("Single measurement result: %d.\n", meas_result);
                }
                break;

            case 2:
                {
                    uint16_t * meas_result;
                    int count;
                    float interval;
                    int ret;

                    printf("please input measure count(%d~%d)  and interval in seconds(%.02f~%.02f):",
                    TOF_CONTI_MEAS_MIN_COUNT,
                    TOF_CONTI_MEAS_MAX_COUNT,
                    (double)TOF_CONTI_MEAS_MIN_INTERVAL,
                    (double)TOF_CONTI_MEAS_MAX_INTERVAL);

                    scanf("%d%f", &count, &interval);
                    if(count < TOF_CONTI_MEAS_MIN_COUNT) count = TOF_CONTI_MEAS_MIN_COUNT;
                    if(count > TOF_CONTI_MEAS_MAX_COUNT) count = TOF_CONTI_MEAS_MAX_COUNT;
                    if(interval < TOF_CONTI_MEAS_MIN_INTERVAL) interval = TOF_CONTI_MEAS_MIN_INTERVAL;
                    if(interval > TOF_CONTI_MEAS_MAX_INTERVAL) interval = TOF_CONTI_MEAS_MAX_INTERVAL;
                    meas_result = (uint16_t*)malloc(count);
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
                    scanf("%f", &interval);
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

/*------------------------------------------------------------*/
static VL53L0X_Dev_t gs_MyDevice;
static VL53L0X_Error tof_measure_prepare()
{
    VL53L0X_Dev_t *pMyDevice = &gs_MyDevice;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    uint8_t VhvSettings;
    uint8_t PhaseCal;

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_StaticInit\n");
        Status = VL53L0X_StaticInit(pMyDevice); // Device Initialization
        // StaticInit will set interrupt by default
        print_pal_error(Status);
    }
    
    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_PerformRefCalibration\n");
        Status = VL53L0X_PerformRefCalibration(pMyDevice,
        		&VhvSettings, &PhaseCal); // Device Initialization
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_PerformRefSpadManagement\n");
        Status = VL53L0X_PerformRefSpadManagement(pMyDevice,
        		&refSpadCount, &isApertureSpads); // Device Initialization
        print_pal_error(Status);
    }

    return Status;
}

int tof_open()
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    VL53L0X_Dev_t *pMyDevice = &gs_MyDevice;
    VL53L0X_Version_t                   Version;
    VL53L0X_Version_t                  *pVersion   = &Version;
    VL53L0X_DeviceInfo_t                DeviceInfo;

    int32_t status_int;

    pMyDevice->I2cDevAddr      = gs_i2c_tof_8b_addr;
    pMyDevice->comms_type      =  1;
    pMyDevice->comms_speed_khz =  400;


    Status = VL53L0X_i2c_init(gs_i2c_dev_name, gs_i2c_tof_8b_addr);
    if (Status != VL53L0X_ERROR_NONE) 
    {
        printf("i2c init error.\n");
        return -1;
    }

    /*
     * Disable VL53L0X API logging if you want to run at full speed
     */
#ifdef VL53L0X_LOG_ENABLE
    VL53L0X_trace_config("test.log", TRACE_MODULE_ALL, TRACE_LEVEL_ALL, TRACE_FUNCTION_ALL);
#endif

    /*
     *  Get the version of the VL53L0X API running in the firmware
     */

    status_int = VL53L0X_GetVersion(pVersion);
    if (status_int != VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    /*
     *  Verify the version of the VL53L0X API running in the firmrware
     */

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf("VL53L0X API Version: Your firmware has %d.%d.%d (revision %d)",
                pVersion->major, pVersion->minor, pVersion->build, pVersion->revision);
    }

    // End of implementation specific
    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_DataInit\n");
        Status = VL53L0X_DataInit(pMyDevice); // Data initialization
        print_pal_error(Status);
    }
    
    if(Status == VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_GetDeviceInfo(pMyDevice, &DeviceInfo);
    }
    if(Status == VL53L0X_ERROR_NONE)
    {
        printf("VL53L0X_GetDeviceInfo:\n");
        printf("Device Name : %s\n", DeviceInfo.Name);
        printf("Device Type : %s\n", DeviceInfo.Type);
        printf("Device ID : %s\n", DeviceInfo.ProductId);
        printf("ProductRevisionMajor : %d\n", DeviceInfo.ProductRevisionMajor);
        printf("ProductRevisionMinor : %d\n", DeviceInfo.ProductRevisionMinor);
    }
    
    return Status;
}

int tof_close()
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    printf ("Close TOF.\n");
    status_int = VL53L0X_i2c_close();
    if (status_int != 0)
    {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    print_pal_error(Status);
    return status_int;
}

int tof_continuous_measure(unsigned short* result_buf, int count, float interval)
{
    VL53L0X_Dev_t *pMyDevice = &gs_MyDevice;
    VL53L0X_RangingMeasurementData_t    RangingMeasurementData;
    VL53L0X_RangingMeasurementData_t   *pRangingMeasurementData    = &RangingMeasurementData;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int measure_cnt = 0;
    bool endless = false;

    if((result_buf == NULL) || (count <= 0))
    {
        endless = true; //THIS IS JUST FOR TEST.
    }

    if(count < TOF_CONTI_MEAS_MIN_COUNT) count = TOF_CONTI_MEAS_MIN_COUNT;
    if(count > TOF_CONTI_MEAS_MAX_COUNT) count = TOF_CONTI_MEAS_MAX_COUNT;
    if(interval < TOF_CONTI_MEAS_MIN_INTERVAL) interval = TOF_CONTI_MEAS_MIN_INTERVAL;
    if(interval > TOF_CONTI_MEAS_MAX_INTERVAL) interval = TOF_CONTI_MEAS_MAX_INTERVAL;

    printf("tof continuous measure, count %d, interval %.02f seconds.\n",
            count, interval);

    Status = tof_measure_prepare();
    if(Status != VL53L0X_ERROR_NONE)
    {
        printf("tof measure prepare error.\n");
        return measure_cnt;
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_SetDeviceMode\n");
        Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING); // Setup in single ranging mode
        print_pal_error(Status);
    }
    
    if(Status == VL53L0X_ERROR_NONE)
    {
		printf ("Call of VL53L0X_StartMeasurement\n");
		Status = VL53L0X_StartMeasurement(pMyDevice);
		print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        measure_cnt = 0;
        if(endless) count = 1;
        while(measure_cnt < count)
        {
            Status = WaitMeasurementDataReady(pMyDevice);

            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_GetRangingMeasurementData(pMyDevice, pRangingMeasurementData);

                if(result_buf)
                {
                    *(result_buf + measure_cnt) = pRangingMeasurementData->RangeMilliMeter;
                }

                printf("%d: %d\n", measure_cnt, pRangingMeasurementData->RangeMilliMeter);
                // Clear the interrupt
                VL53L0X_ClearInterruptMask(pMyDevice, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
                VL53L0X_PollingDelay(pMyDevice);
                if(interval > 0)
                {
                    sleep(interval);
                }

            } else 
            {
                break;
            }
            if(!endless)
            {
                ++measure_cnt;
            }
        }
    }

    
    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_StopMeasurement\n");
        Status = VL53L0X_StopMeasurement(pMyDevice);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Wait Stop to be competed\n");
        Status = WaitStopCompleted(pMyDevice);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_ClearInterruptMask(pMyDevice,
                        VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
    }

    return measure_cnt;
}

unsigned short tof_single_measure()
{
    VL53L0X_Dev_t *pMyDevice = &gs_MyDevice;
    VL53L0X_RangingMeasurementData_t    RangingMeasurementData;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    FixPoint1616_t LimitCheckCurrent;
    uint16_t measure_ret = (uint16_t)(-1);

    Status = tof_measure_prepare();
    if(Status != VL53L0X_ERROR_NONE)
    {
        printf("tof measure prepare error.\n");
        return measure_ret;
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        // no need to do this when we use VL53L0X_PerformSingleRangingMeasurement
        printf ("Call of VL53L0X_SetDeviceMode\n");
        Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING); // Setup in single ranging mode
        print_pal_error(Status);
    }

    // Enable/Disable Sigma and Signal check
    if (Status == VL53L0X_ERROR_NONE) 
    {
        Status = VL53L0X_SetLimitCheckEnable(pMyDevice,
        		VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
    }
    if (Status == VL53L0X_ERROR_NONE) 
    {
        Status = VL53L0X_SetLimitCheckEnable(pMyDevice,
        		VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
    }
    if (Status == VL53L0X_ERROR_NONE) 
    {
        Status = VL53L0X_SetLimitCheckEnable(pMyDevice,
        		VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1);
    }

    if (Status == VL53L0X_ERROR_NONE) 
    {
        Status = VL53L0X_SetLimitCheckValue(pMyDevice,
        		VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD,
        		(FixPoint1616_t)(1.5*0.023*65536));
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_PerformSingleRangingMeasurement\n");
        Status = VL53L0X_PerformSingleRangingMeasurement(pMyDevice,
                &RangingMeasurementData);

        printf("Single Ranging Measurement Finished.\n");


        print_pal_error(Status);
        print_range_status(&RangingMeasurementData);

        VL53L0X_GetLimitCheckCurrent(pMyDevice,
                VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &LimitCheckCurrent);

        printf("RANGE IGNORE THRESHOLD: %f\n\n", (float)LimitCheckCurrent/65536.0);

        if (Status == VL53L0X_ERROR_NONE) 
        {
            measure_ret = RangingMeasurementData.RangeMilliMeter;
        }
    }
    return measure_ret;
}

void tof_ref_register_test()
{
    u8 ret;
    u8 test_reg_idx[] = {0xC0, 0xC1, 0xC2, 0x51, 0x61};
    int i;
    u8 one_byte_data;
    u16 two_byte_data;
    for(i = 0; i < sizeof(test_reg_idx); ++i)
    {
        if(i < 3)
        {
            ret = VL53L0X_read_byte(gs_i2c_tof_8b_addr,
                    test_reg_idx[i], &one_byte_data);
            printf("ret: %d, register 0x%02X data: 0x%02X.\n",
                    ret, test_reg_idx[i], one_byte_data);
        }
        else
        {
            ret = VL53L0X_read_word(gs_i2c_tof_8b_addr,
                    test_reg_idx[i], &two_byte_data);
            printf("ret: %d, register 0x%02X data: 0x%04X.\n",
                    ret, test_reg_idx[i], two_byte_data);
        }
    }
}

