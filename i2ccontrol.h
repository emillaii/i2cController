#ifndef I2CCONTROL_H
#define I2CCONTROL_H

#include <QObject>
#include <windows.h>
#include <stdio.h>
#include "Usbi2cio.h"
#include <atlstr.h>

typedef enum {
    I2CMODE_UNKNOWN = -1,
    I2CMODE_ADDR8_VALUE8 = 0,
    I2CMODE_ADDR8_VALUE16 = 1,
    I2CMODE_ADDR16_VALUE8 = 2,
    I2CMODE_ADDR16_VALUE16 = 3,
    I2CMODE_ADDR16_VALUE32 = 4,
} I2C_WR_modes;

class i2cControl : public QObject
{
    Q_OBJECT

    const bool NEED_CONVERT_ENDIAN = false;

public:
    explicit i2cControl(QObject *parent = nullptr);
    I2C_TRANS TransI2C;
    bool deviceOnFlag = false;
    long *IoRead;
    long ulIoPortConfig;
    HANDLE hDevice[127];
    HINSTANCE hDLL;
    HINSTANCE sunny_driver_hDll;
    HINSTANCE ois_hDll;
    typedef WORD(CALLBACK* GETDLLVERSION)(void);
    typedef HANDLE(CALLBACK* OPENDEVICEINSTANCE)(LPSTR,BYTE);
    typedef BOOL(CALLBACK* CLOSEDEVICEINSTANCE)(HANDLE);
    typedef BOOL(CALLBACK* DETECTDEVICE)(HANDLE);
    typedef BYTE(CALLBACK* GETDEVICECOUNT)(LPSTR);
    typedef BYTE(CALLBACK* GETDEVICEINFO)(LPSTR,LPDEVINFO);
    typedef HANDLE(CALLBACK* OPENDEVICEBYSERIALID)(LPSTR,LPSTR);
    typedef BOOL(CALLBACK* GETSERIALID)(HANDLE,LPSTR);
    typedef BOOL(CALLBACK* CONFIGIOPORTS)(HANDLE,ULONG);
    typedef BOOL(CALLBACK* GETIOCONFIG)(HANDLE,LPLONG);
    typedef BOOL(CALLBACK* READIOPORTS)(HANDLE,LPLONG);
    typedef BOOL(CALLBACK* WRITEIOPORTS)(HANDLE,ULONG,ULONG);
    typedef BOOL(CALLBACK* READI2C)(HANDLE,PI2C_TRANS);
    typedef BOOL(CALLBACK* WRITEI2C)(HANDLE,PI2C_TRANS);
    GETDLLVERSION GetDllVersion;
    OPENDEVICEINSTANCE OpenDeviceInstance;
    CLOSEDEVICEINSTANCE CloseDeviceInstance;
    DETECTDEVICE DetectDevice;
    GETDEVICECOUNT GetDeviceCount;
    GETDEVICEINFO GetDeviceInfo;
    OPENDEVICEBYSERIALID OpenDeviceBySerialId;
    GETSERIALID GetSerialId;
    CONFIGIOPORTS ConfigIoPorts;
    GETIOCONFIG GetIoConfig;
    READIOPORTS ReadIoPorts;
    WRITEIOPORTS WriteIoPorts;
    READI2C ReadI2c;
    WRITEI2C WriteI2c;

    bool readi2c();
    bool openDevice();
    bool closeDevice();

    //VCM operation
    int vcm_move(int);
    int vcm_init();
    int vcm_read_hall_code(int pos);

    //OIS operation
    int ois_servo_on();
    int ois_servo_off();
    int ois_move(int x, int y);
    int ois_move_x(int x);
    int ois_move_y(int y);
    int ois_read_xy();
//    void test(int *, int);

    BOOL write_reg_fun(unsigned int, unsigned int, unsigned int, I2C_WR_modes);
    BOOL(CALLBACK i2cControl::*read_reg_fun)(unsigned int, unsigned int, unsigned int *, I2C_WR_modes);
    BOOL(CALLBACK i2cControl::*write_regs_fun)(unsigned int, unsigned int, unsigned int, unsigned int *, I2C_WR_modes);
    BOOL(CALLBACK i2cControl::*read_regs_fun)(unsigned int, unsigned int, unsigned int, unsigned int *, I2C_WR_modes);

    BOOL WriteReg(unsigned int slaveId, unsigned int regAddr, unsigned int value, I2C_WR_modes mode);
    unsigned int ReadReg(unsigned int slaveId, unsigned int regAddr, unsigned int length, I2C_WR_modes mode);

    BOOL WriteRegs(unsigned int slaveId, unsigned int regAddr, unsigned int length, unsigned int *values, I2C_WR_modes mode);
    BOOL ReadRegs(unsigned int slaveId, unsigned int regAddr, unsigned int length, unsigned int *values, I2C_WR_modes mode);

    //Sunny dll function loading
    typedef int(WINAPIV* SMD_MOVETO)(int,int,int);
    typedef int(WINAPIV* SMD_INIT)(void*, void*, void*, void*, void*, int, CString, bool, bool, bool);
    typedef int(WINAPIV* SMD_READHALLCODE)(int, int, int*, int);
    typedef int(WINAPIV* SMD_POSITION)(void*, void*, bool, unsigned int);

    SMD_INIT SMD_Init;
    SMD_MOVETO SMD_MoveTo;
    SMD_READHALLCODE SMD_ReadHallCode;
    SMD_POSITION SMD_Position;
signals:

public slots:

private:
    void swap(BYTE& a, BYTE& b)
    {
        a = a ^ b;
        b = b ^ a;
        a = a ^ b;
    }
};

#endif // I2CCONTROL_H
