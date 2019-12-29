#include "i2ccontrol.h"

static i2cControl *gI2CControl;
//slave id + 1 , reg addr = 8a
//    bool ret = gI2CControl->WriteReg(slaveId+1, 138, value, mode);
bool write(unsigned int slaveId, unsigned int regAddr, unsigned int value, I2C_WR_modes mode){
    qInfo("Write i2c is called! SlaveId: %04X regAddr: %04X value: %04X mode: %d", slaveId, regAddr, value, mode);

    bool ret = gI2CControl->WriteReg(slaveId, 224, 1, I2C_WR_modes::I2CMODE_ADDR8_VALUE8);
    ret = gI2CControl->WriteReg(slaveId, 132, value, mode);
    //bool ret = gI2CControl->WriteReg(slaveId, regAddr, value, mode);
    unsigned int feedback = 0;
    gI2CControl->ReadRegs(slaveId, regAddr, 1, &feedback, mode);
    //Sleep(10);
    qInfo("Read value: %04x", feedback);
    return true;
}

bool read(unsigned int slaveId, unsigned int regAddr, unsigned int *value, I2C_WR_modes mode){
    qInfo("Read i2c is called! SlaveId: %04X regAddr: %04X mode: %d", slaveId, regAddr, mode);
    bool ret = gI2CControl->ReadRegs(slaveId, regAddr, 1, value, mode);
    //Sleep(10);
    qInfo("Read value: %04x", *value);
    return ret;
}

bool write2(unsigned int slaveId, unsigned int regAddr, unsigned int length, unsigned int *value, I2C_WR_modes mode){
    qInfo("Batch Write i2c is called! SlaveId: %04X regAddr: %04X length: %d mode: %d", slaveId, regAddr, length, mode);
    bool ret = gI2CControl->WriteRegs(slaveId, regAddr, length, value, mode);
    //Sleep(10);
    return ret;
}

bool read2(unsigned int slaveId, unsigned int regAddr, unsigned int length, unsigned int *value, I2C_WR_modes mode){
    *value = 0;
    qInfo("Batch Read i2c is called! SlaveId: %04X regAddr: %04X length: %d mode: %d", slaveId, regAddr, length, mode);
    bool ret = gI2CControl->ReadRegs(slaveId, regAddr, length, value, mode);
    //Sleep(10);
    return ret;
}

i2cControl::i2cControl(QObject *parent) : QObject(parent)
{
    gI2CControl = this;
    hDLL = LoadLibrary(L"usbi2cio.dll");
    if( hDLL != NULL) {
        qInfo("usbi2cio.dll load successfully");
        GetDllVersion=(GETDLLVERSION)GetProcAddress(hDLL,"DAPI_GetDllVersion");
        OpenDeviceInstance=(OPENDEVICEINSTANCE)GetProcAddress(hDLL,"DAPI_OpenDeviceInstance");
        CloseDeviceInstance=(CLOSEDEVICEINSTANCE)GetProcAddress(hDLL,"DAPI_CloseDeviceInstance");
        DetectDevice=(DETECTDEVICE)GetProcAddress(hDLL,"DAPI_DetectDevice");
        GetDeviceCount=(GETDEVICECOUNT)GetProcAddress(hDLL,"DAPI_GetDeviceCount");
        GetDeviceInfo=(GETDEVICEINFO)GetProcAddress(hDLL,"DAPI_GetDeviceInfo");
        OpenDeviceBySerialId=(OPENDEVICEBYSERIALID)GetProcAddress(hDLL,"DAPI_OpenDeviceBySerialId");
        GetSerialId=(GETSERIALID)GetProcAddress(hDLL,"DAPI_GetSerialId");
        ConfigIoPorts=(CONFIGIOPORTS)GetProcAddress(hDLL,"DAPI_ConfigIoPorts");
        GetIoConfig=(GETIOCONFIG)GetProcAddress(hDLL,"DAPI_GetIoConfig");
        ReadIoPorts=(READIOPORTS)GetProcAddress(hDLL,"DAPI_ReadIoPorts");
        WriteIoPorts=(WRITEIOPORTS)GetProcAddress(hDLL,"DAPI_WriteIoPorts");
        ReadI2c=(READI2C)GetProcAddress(hDLL,"DAPI_ReadI2c");
        WriteI2c=(WRITEI2C)GetProcAddress(hDLL,"DAPI_WriteI2c");
        WORD version = GetDllVersion();
        qInfo("UsbI2cIo.dll version: %d", version);
    }else {
        qCritical("UsbI2cIo.dll load fail");
    }

    //Sunny AF Driver
    sunny_driver_hDll = LoadLibrary(L"SunnyMotorDriver.dll");

    if ( sunny_driver_hDll != NULL) {
        qInfo("SunnyMotorDriver.dll load successfully");
        SMD_Init=(SMD_INIT)GetProcAddress(sunny_driver_hDll, "SMD_Init");
        SMD_MoveTo=(SMD_MOVETO)GetProcAddress(sunny_driver_hDll, "SMD_MoveTo");
        SMD_ReadHallCode=(SMD_READHALLCODE)GetProcAddress(sunny_driver_hDll, "SMD_ReadHallCode");
        SMD_Position=(SMD_POSITION)GetProcAddress(sunny_driver_hDll,"Move_position");
    } else {
        int d = GetLastError();
        qCritical("SunnyMotorDriver.dll load fail: %d", d);
    }
}

bool i2cControl::readi2c()
{
    TransI2C.byTransType = I2C_TRANS_8ADR;
    LONG lReadCnt;
    // initialize I2C transaction structure
    TransI2C.wMemoryAddr = 0x00;
    TransI2C.bySlvDevAddr = 0x00;
    TransI2C.wCount = 1;
    for (int i = 0; i < 255; i++)
    {
        TransI2C.bySlvDevAddr++;
        lReadCnt = ReadI2c(hDevice[0], &TransI2C);
        if(lReadCnt == TransI2C.wCount) {
            qInfo("Read value[%d]: %x %x", i, TransI2C.Data[i], TransI2C.bySlvDevAddr);
        }
    }

    return true;
}

bool i2cControl::openDevice()
{
    int deviceCount = GetDeviceCount(LPSTR("UsbI2cIo"));
    qInfo("Device count: %d", deviceCount);
    if (deviceCount > 0)
    {
        if(deviceOnFlag)
            return true;
        //Try to open device
        hDevice[0]=OpenDeviceInstance(LPSTR("UsbI2ddcIo"), 0);
        if(hDevice == INVALID_HANDLE_VALUE) {
            qCritical("Cannot find UsbI2cIo");
        } else {
            deviceOnFlag = true;
            return true;
        }
    } else {
        qInfo("Cannot find UsbI2cIo device");
    }
    closeDevice();
    return false;
}

bool i2cControl::closeDevice()
{
    if(deviceOnFlag)
    {
        return CloseDeviceInstance(hDevice[0]);
        deviceOnFlag = false;
    }
    return true;
}

int i2cControl::vcm_move(int pos)
{
    return SMD_MoveTo(45, pos, 200);
}

int i2cControl::vcm_init()
{
    int ret = SMD_Init(write, read, write2, read2, nullptr, 45, "", true, false, false);
    qInfo("SMD_init finished : %d", ret);
//    ret = SMD_MoveTo(45, 1000, 200);
//    qInfo("SMD_MoveTo finished : %d", ret);
    //int feedback = 0;
    //SMD_ReadHallCode(45,1000,&feedback,512);
    qInfo("Finish vcm_init");
    return ret;
}

int i2cControl::ois_move(int x, int y)
{
//    unsigned int slaveId = 0x7c;
//    //Select gyro (ST)
//    bool ret = gI2CControl->WriteReg(slaveId, 0xF015, 0x02, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
//    Sleep(150);
//    //Check if servo is on
//    unsigned int value = ReadReg(slaveId, 0xF010, 1, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
//    if (value == 0x03)
//    {
//        //Move XY with servo on
//        qInfo("Move xy with servo on, x = %d, y = %d", x, y);
//        ret = gI2CControl->WriteReg(slaveId, 0x0114, x, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
//        Sleep(150);
//        ret = gI2CControl->WriteReg(slaveId, 0x0164, y, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
//    }
//    else if (value == 0x00)
//    {
//        //Move XY with servo off
//        qInfo("Move xy with servo off, x = %d, y = %d", x, y);
//        ret = gI2CControl->WriteReg(slaveId, 0x0128, x, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
//        Sleep(150);
//        ret = gI2CControl->WriteReg(slaveId, 0x0178, y, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
//    }
//    else
//    {
//        return -1;
//    }

//    return ret;

    int ret = SMD_Position(write2, read2, true, x);
    if (ret != 0)
    {
        qInfo("Move xy with servo on, x = %d, y = %d", x, y);
    }
    ret = SMD_Position(write2, read2, false, y);
    if (ret != 0)
    {
        qInfo("Move xy with servo on, x = %d, y = %d", x, y);
    }
    return ret;
}

int i2cControl::ois_move_x(int x)
{
    unsigned int slaveId = 0x48;
    //Standby off
    bool ret = gI2CControl->WriteReg(slaveId, 0xF019, 0x00, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    Sleep(150);
    //Select gyro (ST)
    ret = gI2CControl->WriteReg(slaveId, 0xF015, 0x02, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    Sleep(150);
    //Check if servo is on
    unsigned int value = ReadReg(slaveId, 0xF010, 1, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    if (value == 0x03)
    {
        //Move X with servo on
        qInfo("Move x with servo on, x = %d", x);
        ret = gI2CControl->WriteReg(slaveId, 0x00DC, x, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    }
    else if (value == 0x00)
    {
        //Move X with servo off
        qInfo("Move x with servo off, x = %d", x);
        ret = gI2CControl->WriteReg(slaveId, 0x0128, x, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    }
    else
    {
        return -1;
    }

    return ret;
}

int i2cControl::ois_move_y(int y)
{
    unsigned int slaveId = 0x48;
    //Standby off
    bool ret = gI2CControl->WriteReg(slaveId, 0xF019, 0x00, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    Sleep(150);
    //Select gyro (ST)
    ret = gI2CControl->WriteReg(slaveId, 0xF015, 0x02, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    Sleep(150);
    //Check if servo is on
    unsigned int value = ReadReg(slaveId, 0xF010, 1, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    if (value == 0x03)
    {
        //Move Y with servo on
        qInfo("Move y with servo on, y = %d", y);
        ret = gI2CControl->WriteReg(slaveId, 0x012C, y, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    }
    else if (value == 0x00)
    {
        //Move Y with servo off
        qInfo("Move y with servo off, y = %d", y);
        ret = gI2CControl->WriteReg(slaveId, 0x0178, y, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    }
    else
    {
        return -1;
    }

    return ret;
}

int i2cControl::ois_read_xy()
{
    bool ret = 0;
    unsigned int slaveId = 0x48;
    unsigned int x, y;
    //Check if servo is on
    unsigned int value = ReadReg(slaveId, 0xF010, 1, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    if (value == 0x03)
    {
        x = ReadReg(slaveId, 0x00DC, 4, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
        y = ReadReg(slaveId, 0x012C, 4, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
        qInfo("Servo is on, x = %d, y = %d", x, y);
    }
    else if (value == 0x00)
    {
        x = ReadReg(slaveId, 0x0128, 4, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
        y = ReadReg(slaveId, 0x0178, 4, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
        qInfo("Servo is off, x = %d, y = %d", x, y);
    }
    else
    {
        ret = -1;
    }

    return ret;
}

int i2cControl::vcm_read_hall_code(int pos)
{
    int slaveId = 45;
    int regAddr = pos;
    int value = 0;
    int mode = 512;
    int ret = SMD_ReadHallCode(slaveId,regAddr,&value,mode);
    return ret;
}

int i2cControl::ois_servo_on()
{
    unsigned int slaveId = 0x48;
    //Servo On
    bool ret = gI2CControl->WriteReg(slaveId, 0xF010, 0x03, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    Sleep(150);
    return ret;
}

int i2cControl::ois_servo_off()
{
    unsigned int slaveId = 0x48;
    //Servo Off
    bool ret = gI2CControl->WriteReg(slaveId, 0xF010, 0x00, I2C_WR_modes::I2CMODE_ADDR16_VALUE32);
    Sleep(150);
    return ret;
}

BOOL i2cControl::WriteReg(unsigned int slaveId, unsigned int regAddr, unsigned int value, I2C_WR_modes mode)
{
    if(mode == I2CMODE_ADDR8_VALUE8)
       {
           TransI2C.byTransType = I2C_TRANS_8ADR;
           TransI2C.wCount = 1;      // number of bytes to write
           TransI2C.Data[0] = value;  // Data byte 0 value
       } else if (mode == I2CMODE_ADDR8_VALUE16) {
           TransI2C.byTransType = I2C_TRANS_8ADR;
           TransI2C.wCount = 2;      // number of bytes to write
           TransI2C.Data[1] = value;  // Data byte 0 value
           TransI2C.Data[0] = value>>8;
       } else if (mode == I2CMODE_ADDR16_VALUE8) {
           TransI2C.byTransType = I2C_TRANS_16ADR;
           TransI2C.wCount = 1;      // number of bytes to write
           TransI2C.Data[0] = value;
       }
       else if (mode == I2CMODE_ADDR16_VALUE16) {
           TransI2C.byTransType = I2C_TRANS_16ADR;
           TransI2C.wCount = 2;      // number of bytes to write
           TransI2C.Data[1] = value;  // Data byte 0 value
           TransI2C.Data[0] = value>>8;
       } else if (mode == I2CMODE_ADDR16_VALUE32) {
           TransI2C.byTransType = I2C_TRANS_16ADR;
           TransI2C.wCount = 4;      // number of bytes to write
           TransI2C.Data[3] = value;
           TransI2C.Data[2] = value>>8;
           TransI2C.Data[1] = value>>16;  // Data byte 0 value
           TransI2C.Data[0] = value>>24;
       }
       if (!deviceOnFlag) {
           qCritical("i2c device cannot find. Write I2C fail.");
           return false;
       }
       LONG lWriteCnt;
       TransI2C.wMemoryAddr = regAddr;
       TransI2C.bySlvDevAddr = slaveId;   // I2C device Id (R/W bit is handled by API function)
       lWriteCnt = WriteI2c(hDevice[0], &TransI2C);
       if(lWriteCnt == TransI2C.wCount) {
           qInfo("write i2c success TransI2C.bySlvDevAddr: %x %x" ,  TransI2C.bySlvDevAddr, TransI2C.wMemoryAddr);
       }
       else {
           qCritical("Fail to write the i2c: %d", lWriteCnt);
           return false;
       }
       return true;
//    return WriteRegs(slaveId, regAddr, 1, &value, mode);
}

unsigned int i2cControl::ReadReg(unsigned int slaveId, unsigned int regAddr, unsigned int length, I2C_WR_modes mode)
{
    unsigned int value = 0;
    ReadRegs(slaveId, regAddr, 1, &value, mode);
    return value;
}

//ToDo: Not yet complete
BOOL i2cControl::WriteRegs(unsigned int slaveId, unsigned int regAddr, unsigned int length, unsigned int *value, I2C_WR_modes mode)
{
    if(mode == I2CMODE_ADDR8_VALUE8)
    {
        TransI2C.byTransType = I2C_TRANS_8ADR;
    } else if (mode == I2CMODE_ADDR8_VALUE16) {
        TransI2C.byTransType = I2C_TRANS_8ADR;
    } else if (mode == I2CMODE_ADDR16_VALUE8) {
        TransI2C.byTransType = I2C_TRANS_16ADR;
    } else if (mode == I2CMODE_ADDR16_VALUE16) {
        TransI2C.byTransType = I2C_TRANS_16ADR;
    } else if (mode == I2CMODE_ADDR16_VALUE32) {
        TransI2C.byTransType = I2C_TRANS_16ADR;
    }
    if (!deviceOnFlag) {
        qCritical("i2c device cannot find. Write I2C fail.");
        return false;
    }

    TransI2C.bySlvDevAddr = slaveId;
    int maxDataLen = 1088;  // I2C_TRANS.Data数组的长度
    int writedDataLen = 0;  // 已写入到I2C_TRANS.Data数组的长度
    int batchIndex = 0;
    LONG lWriteCnt; // I2C device Id (R/W bit is handled by API function)

    TransI2C.wMemoryAddr = regAddr;
    for(int i = 0; i < length; i++)
    {
        if(mode == I2CMODE_ADDR8_VALUE8 || mode == I2CMODE_ADDR16_VALUE8)
        {
            TransI2C.Data[writedDataLen] = value[i];
            //qInfo("[%d]: %04X %04X",i, TransI2C.Data[writedDataLen], value[i]);
            writedDataLen += 1;
        } else if (mode == I2CMODE_ADDR8_VALUE16 || mode == I2CMODE_ADDR16_VALUE16) {
            unsigned short tmpValue = value[i];
            memcpy(&TransI2C.Data[writedDataLen], &tmpValue, 2);
            if(NEED_CONVERT_ENDIAN)
            {
                swap(TransI2C.Data[writedDataLen], TransI2C.Data[writedDataLen + 1]);
            }
            writedDataLen += 2;
        }
        else if (mode == I2CMODE_ADDR16_VALUE32) {
            memcpy(&TransI2C.Data[writedDataLen], &value[i], 4);
            if(NEED_CONVERT_ENDIAN)
            {
                swap(TransI2C.Data[writedDataLen], TransI2C.Data[writedDataLen + 3]);
                swap(TransI2C.Data[writedDataLen + 1], TransI2C.Data[writedDataLen + 2]);
            }
            writedDataLen += 4;
        }

        if(writedDataLen == maxDataLen || i == length - 1)
        {
            TransI2C.wCount = writedDataLen;
            lWriteCnt = WriteI2c(hDevice[0], &TransI2C);
            const char* log = "Batch write i2c %s. Batch index: %d, Data len: %d, TransI2C.bySlvDevAddr: %x %x";
            if(lWriteCnt == TransI2C.wCount) {
                qInfo(log, "successful", batchIndex, TransI2C.wCount, TransI2C.bySlvDevAddr, TransI2C.wMemoryAddr);

                // Verification
//                bool verificationRes = true;
//                I2C_TRANS tmpI2cTrans = TransI2C;
//                memset(tmpI2cTrans.Data, 0, 1088);
//                ReadI2c(hDevice[0], &tmpI2cTrans);
//                for(int t = 0; t < tmpI2cTrans.wCount; t++)
//                {
//                    if(TransI2C.Data[t] != tmpI2cTrans.Data[t])
//                    {
//                        qCritical("Verification failed! Writen value: %x, Read value: %x.", TransI2C.Data[t], tmpI2cTrans.Data[t]);
//                        verificationRes = false;
//                    }
//                }
//                if(!verificationRes)
//                {
//                    return false;
//                }

                // clear data for next batch
                TransI2C.wMemoryAddr += maxDataLen;
                writedDataLen = 0;
                batchIndex ++;
            }
            else {
                qInfo("failed Batch index: %d Data len: %d slaveId: %04X regAddr: %04X", batchIndex, TransI2C.wCount, TransI2C.bySlvDevAddr, TransI2C.wMemoryAddr);
                return false;
            }
        }
    }
    return true;
}

//Not yet complete
BOOL i2cControl::ReadRegs(unsigned int slaveId, unsigned int regAddr, unsigned int length, unsigned int *value, I2C_WR_modes mode)
{
    int dataBits = 0;
    if(mode == I2CMODE_ADDR8_VALUE8)
    {
        TransI2C.byTransType = I2C_TRANS_8ADR;
        dataBits = 1;
    } else if (mode == I2CMODE_ADDR8_VALUE16) {
        TransI2C.byTransType = I2C_TRANS_8ADR;
        dataBits = 2;
    } else if (mode == I2CMODE_ADDR16_VALUE8) {
        TransI2C.byTransType = I2C_TRANS_16ADR;
        dataBits = 1;
    }
    else if (mode == I2CMODE_ADDR16_VALUE16) {
        TransI2C.byTransType = I2C_TRANS_16ADR;
        dataBits = 2;
    } else if (mode == I2CMODE_ADDR16_VALUE32) {
        TransI2C.byTransType = I2C_TRANS_16ADR;
        dataBits = 4;
    }
    if (!deviceOnFlag){
        qCritical("i2c device cannot find. Read I2C fail.");
        return false;
    }

    TransI2C.bySlvDevAddr = slaveId;
    TransI2C.wMemoryAddr = regAddr;
    int dataLenToBeRead = length * dataBits;
    int maxDataLen = 1088;  // I2C_TRANS.Data数组的长度
    int readDataLen = 0;  // 已从I2C_TRANS.Data数组读取的长度
    int batchIndex = 0;
    int valueIndex = 0;
    LONG lReadCnt;; // I2C device Id (R/W bit is handled by API function)
    // TransI2C.wCount = length;
    while (dataLenToBeRead > 0) {
        readDataLen = dataLenToBeRead >= maxDataLen ? maxDataLen : dataLenToBeRead;
        TransI2C.wCount = readDataLen;

        lReadCnt = ReadI2c(hDevice[0], &TransI2C);
        QString log = "Batch read i2c %s. Batch index: %d, Data len: %d, TransI2C.bySlvDevAddr: %x %x";
        if(lReadCnt == TransI2C.wCount) {
            qInfo(log.toStdString().c_str(), "successful", batchIndex, TransI2C.wCount, TransI2C.bySlvDevAddr, TransI2C.wMemoryAddr);

            int convertedDataLen = 0;
            while (convertedDataLen < readDataLen) {
                if(mode == I2CMODE_ADDR8_VALUE8 || mode == I2CMODE_ADDR16_VALUE8)
                {
                    value[valueIndex] = TransI2C.Data[convertedDataLen];
                    convertedDataLen += 1;
                } else if (mode == I2CMODE_ADDR8_VALUE16 || mode == I2CMODE_ADDR16_VALUE16) {
                    unsigned short tmpValue = 0;
                    if(NEED_CONVERT_ENDIAN)
                    {
                        swap(TransI2C.Data[convertedDataLen], TransI2C.Data[convertedDataLen + 1]);
                    }
                    memcpy(&tmpValue, &TransI2C.Data[convertedDataLen], 2);
                    value[valueIndex] = tmpValue;
                    convertedDataLen += 2;
                }
                else if (mode == I2CMODE_ADDR16_VALUE32) {
                    if(NEED_CONVERT_ENDIAN)
                    {
                        swap(TransI2C.Data[convertedDataLen], TransI2C.Data[convertedDataLen + 3]);
                        swap(TransI2C.Data[convertedDataLen + 1], TransI2C.Data[convertedDataLen + 2]);
                    }
                    value[valueIndex] = *((unsigned int*)(&TransI2C.Data[convertedDataLen]));
                    convertedDataLen += 4;
                }
                valueIndex ++;
            }

            dataLenToBeRead -= readDataLen;
            batchIndex++;
            TransI2C.wMemoryAddr += readDataLen;
        }
        else {
            qCritical(log.toStdString().c_str(), "failed", batchIndex, TransI2C.wCount, TransI2C.bySlvDevAddr, TransI2C.wMemoryAddr);
            return false;
        }
    }

    return true;
}
