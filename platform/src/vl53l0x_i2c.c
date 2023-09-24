#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <linux/i2c-dev.h>
#include "vl53l0x_def.h"
#include "vl53l0x_i2c.h"

#include "logger.h"
//////////////////////////////////////////////////////////////////////////////////	 
static int gs_i2c_tof_fd = -1;
static u8 gs_i2c_tof_7b_addr = -1;

VL53L0X_Error VL53L0X_i2c_init(const char* dev_name, u8 slave_addr)
{
    int ret;
    u8 addr_7b = slave_addr >> 1;

    DIY_LOG(LOG_INFO, "VL53L0X_i2c_init: slave_addr: 0x%02X, addr_7b: 0x%02X\n", slave_addr, addr_7b);
    //printf("VL53L0X_i2c_init: slave_addr: 0x%02X, addr_7b: 0x%02X\n", slave_addr, addr_7b);

    gs_i2c_tof_fd = open(dev_name, O_RDWR);
    if(gs_i2c_tof_fd < 0)
    {
        printf("open %s error, errno %d\n", dev_name, errno);
        return VL53L0X_ERROR_NOT_I2C_DEV_OPEN_ERROR;
    }

    ret = ioctl(gs_i2c_tof_fd, I2C_SLAVE, addr_7b);
    if(ret < 0)
    {
        printf("ioctl fd %d, addr 0x%02X error, ret %d,  errno %d\n", 
               gs_i2c_tof_fd, slave_addr,  ret, errno);
        close(gs_i2c_tof_fd);
        return VL53L0X_ERROR_NOT_I2C_DEV_IOCTL_ERROR;
    }
    else
    {
        gs_i2c_tof_7b_addr = addr_7b;
        printf("i2c init ok!\n");
        return VL53L0X_ERROR_NONE;
    }
}

VL53L0X_Error VL53L0X_i2c_close()
{
    int ret;
    if(gs_i2c_tof_fd < 0)
    {
        printf("close %d error, errno.\n", gs_i2c_tof_fd);
        return VL53L0X_ERROR_NOT_I2C_DEV_ERROR;
    }
    ret = close(gs_i2c_tof_fd);
    if(ret != 0)
    {
        DIY_LOG(LOG_INFO, "close i2c error, ret: %d, errno: %d\n", ret, errno);
        return VL53L0X_ERROR_NOT_I2C_DEV_CLOSE_ERROR;
    }
    else
    {
        return VL53L0X_ERROR_NONE;
    }
}

void VL_IIC_Start(void)
{
}

void VL_IIC_Stop(void)
{
}

u8 VL_IIC_Wait_Ack(void)
{
    return 0;
}

//����ACKӦ��
void VL_IIC_Ack(void)
{
}

//������ACKӦ��		    
void VL_IIC_NAck(void)
{
}

//IIC����һ���ֽ�
//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��			  
void VL_IIC_Send_Byte(u8 txd)
{
} 

//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
u8 VL_IIC_Read_Byte(void)
{
    return 0;
}

//IICдһ���ֽ�����
u8 VL_IIC_Write_1Byte(u8 SlaveAddress, u8 REG_Address,u8 REG_data)
{
    return 0;
}

//IIC��һ���ֽ�����
u8 VL_IIC_Read_1Byte(u8 SlaveAddress, u8 REG_Address,u8 *REG_data)
{
    return 0;
}

//IICдn�ֽ�����
u8 VL_IIC_Write_nByte(u8 SlaveAddress, u8 REG_Address,u16 len, u8 *buf)
{
    return 0;	
}

//IIC��n�ֽ�����
u8 VL_IIC_Read_nByte(u8 SlaveAddress, u8 REG_Address,u16 len,u8 *buf)
{
	return 0;
}

#define CHECK_I2C_DEV_FD \
{\
    if(gs_i2c_tof_fd < 0)\
    {\
        printf("%s error, fd is invalid: %d\n", __FUNCTION__, gs_i2c_tof_fd);\
        return 0;\
    }\
}

#define CHECK_I2C_SLAVE_ADDR(address) \
{\
    if(((address) >> 1) != gs_i2c_tof_7b_addr)\
    {\
        int ret;\
        ret = ioctl(gs_i2c_tof_fd, I2C_SLAVE, ((address) >> 1));\
        if(ret < 0)\
        {\
            printf("%s: ioctl fd %d addr 0x%02X error, ret %d, errno %d\n", \
                   __FUNCTION__, gs_i2c_tof_fd, (address), ret, errno);\
            return ret;\
        }\
        gs_i2c_tof_7b_addr = ((address) >> 1);\
    }\
}

u8 VL53L0X_write_multi(u8 address, u8 index,u8 *pdata,u16 count)
{
	u8 status = STATUS_OK;
    u8 *io_buf = NULL;
    ssize_t rw_ret;


    CHECK_I2C_DEV_FD;
    CHECK_I2C_SLAVE_ADDR(address);

    io_buf = (u8*)malloc(1 + count);
    if(!io_buf)
    {
        printf("%s: malloc error!\n", __FUNCTION__);
    }
    io_buf[0] = index;
    memcpy(&io_buf[1], pdata, count);
    rw_ret = write(gs_i2c_tof_fd, io_buf, count + 1);
    if(rw_ret <= 0)
    {
        DIY_LOG(LOG_INFO, "write ret: %d, errno: %d\n", rw_ret, errno);
        status = STATUS_FAIL;
    }

    free(io_buf);
    return status;

    /*
	if(VL_IIC_Write_nByte(address,index,count,pdata))
	{
	   status  = STATUS_FAIL;

	}

	return status;
    */
}

u8 VL53L0X_read_multi(u8 address,u8 index,u8 *pdata,u16 count)
{
	u8 status = STATUS_OK;
    ssize_t rw_ret;

    CHECK_I2C_DEV_FD;
    CHECK_I2C_SLAVE_ADDR(address);
    
    rw_ret = write(gs_i2c_tof_fd, &index, 1);
    if(rw_ret <= 0)
    {
        status  = STATUS_FAIL;
        return status;
    }

    rw_ret = read(gs_i2c_tof_fd, pdata, count);
    if(rw_ret <= 0)
    {
        status  = STATUS_FAIL;
    }
    return status;

    /*
	if(VL_IIC_Read_nByte(address,index,count,pdata))
	{
	  status  = STATUS_FAIL;
	}

	return status;
    */
}

//VL53L0X д1������(���ֽ�)
//address:��ַ
//index:ƫ�Ƶ�ַ
//data:����(8λ)
u8 VL53L0X_write_byte(u8 address,u8 index,u8 data)
{
	u8 status = STATUS_OK;

	status = VL53L0X_write_multi(address,index,&data,1);

	return status;
}

//VL53L0X д1������(˫�ֽ�)
//address:��ַ
//index:ƫ�Ƶ�ַ
//data:����(16λ)
u8 VL53L0X_write_word(u8 address,u8 index,u16 data)
{
	u8 status = STATUS_OK;
	
	u8 buffer[2];
	
	//��16λ���ݲ�ֳ�8λ
	buffer[0] = (u8)(data>>8);//�߰�λ
	buffer[1] = (u8)(data&0xff);//�Ͱ�λ
	
	if(index%2==1)
	{  
		//����ͨ�Ų��ܴ���Է�2�ֽڶ���Ĵ������ֽ�
		status = VL53L0X_write_multi(address,index,&buffer[0],1);
		status = VL53L0X_write_multi(address,index,&buffer[0],1);
	}else
	{
		status = VL53L0X_write_multi(address,index,buffer,2);
	}
	
	return status;
}

//VL53L0X д1������(���ֽ�)
//address:��ַ
//index:ƫ�Ƶ�ַ
//data:����(32λ)
u8 VL53L0X_write_dword(u8 address,u8 index,u32 data)
{
	
    u8 status = STATUS_OK;

    u8 buffer[4];	
	
	//��32λ���ݲ�ֳ�8λ
	buffer[0] = (u8)(data>>24);
	buffer[1] = (u8)((data&0xff0000)>>16);
	buffer[2] = (u8)((data&0xff00)>>8);
	buffer[3] = (u8)(data&0xff);
	
	status = VL53L0X_write_multi(address,index,buffer,4);
	
	return status;
	
}


//VL53L0X ��1������(���ֽ�)
//address:��ַ
//index:ƫ�Ƶ�ַ
//data:����(8λ)
u8 VL53L0X_read_byte(u8 address,u8 index,u8 *pdata)
{
	u8 status = STATUS_OK;
	 
	status = VL53L0X_read_multi(address,index,pdata,1);
	
	return status;
	 
}

//VL53L0X ��������(˫�ֽ�)
//address:��ַ
//index:ƫ�Ƶ�ַ
//data:����(16λ)
u8 VL53L0X_read_word(u8 address,u8 index,u16 *pdata)
{
	u8 status = STATUS_OK;
	
	u8 buffer[2];
	
	status = VL53L0X_read_multi(address,index,buffer,2);
	
	*pdata = ((u16)buffer[0]<<8)+(u16)buffer[1];
	
	return status;
	
}

//VL53L0X ��1������(���ֽ�)
//address:��ַ
//index:ƫ�Ƶ�ַ
//data:����(32λ)
u8 VL53L0X_read_dword(u8 address,u8 index,u32 *pdata)
{
	u8 status = STATUS_OK;
	
	u8 buffer[4];
	
	status = VL53L0X_read_multi(address,index,buffer,4);
	
	*pdata = ((u32)buffer[0]<<24)+((u32)buffer[1]<<16)+((u32)buffer[2]<<8)+((u32)buffer[3]);
	
	return status;
	
}
