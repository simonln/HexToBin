#include "includes.h"

#define ONE_RECOARD_MAX_SIZE (43)
#define ONE_DATA_MAX_SIZE    (21)
#define DATA_SIZE            (16)


//type
#define HEX_TYPE_DATARECORD      (0x01)
#define HEX_TYPE_EOF             (0x02)
#define HEX_TYPE_SEGMENTADDRESS  (0x03)
#define HEX_TYPE_EXLINARADDRESS  (0x04)
#define HEX_TYPE_STARTLINARADDR  (0X05)


typedef struct BIN_DATA
{
    int offset;
    int dataLen;
    byte type;
    byte data[DATA_SIZE];
}BIN_DATA_STRU;



//fp:file poniter
//buffer: save the data read from file
//len :length of data
//return 
//  0 success
//  1 error
int GetOneRecord(FILE *fp,byte *buffer,int *len)
{
    byte curChar=0,oldChar=0;
    int count=0,timeout=1000;
    while(timeout--)
    {
        curChar=fgetc(fp);
        buffer[count++]=curChar;
        if(oldChar=='\r'&&curChar=='\n')
        {
            *len=count;
            return 0;//complete
        }
        oldChar=curChar;//save the last char
    }
    return 1;
}

//rawData: the raw data
//rawLen:  the length of rawData
//realData: assic to byte
//realLen: the data len
//return 
//  0 success
//  1 error
int ReadOneRecord(byte *rawData,int rawLen,byte *realData,int *realLen)
{
    int i=1,count=0;//first byte is ':' and the last two byte is '\r' '\n'
    byte oneData=0,oneDataComplete=0;
    //printf("rawLen:%d\r\n",rawLen);
    for(i=1;i<rawLen-3;i+=2)
    {
        byte tmp1=rawData[i],tmp2=rawData[i+1];
        if((tmp1>='0'&&tmp1<='9') ||(tmp1>='A'&&tmp1<='F'))
        {
            if((tmp2>='0'&&tmp2<='9') ||(tmp2>='A'&&tmp2<='F'))
            {
                oneData=(tmp1>='A')?(tmp1-'A'+10):(tmp1-'0');
                oneData=(oneData<<4)+((tmp2>='A')?(tmp2-'A'+10):(tmp2-'0'));
                realData[count++]=oneData;
            }
            else
            {
              return 1;//error  
            }
        }
        else
        {
            return 1;//error
        }
    }
    *realLen=count;
    return 0;
}

//recordData: the record data
//recordLen : the length of record data
//binData : save the one record data to bin data
//return 
//   0 success
//   1 checksum is error

int AnalyzeOneRecord(byte *recordData,int recordLen,BIN_DATA_STRU *binData)
{
    int i=0;
    byte checksum=0;
    binData->dataLen = recordData[0];//the data length
    checksum+=recordData[0];
    binData->offset = (recordData[1]<<8)+recordData[2];//the offset
    checksum+=recordData[1] + recordData[2];
    binData->type = recordData[3];
    checksum+=recordData[3];
    for(i=4;i<recordLen-1)
    {
        binData->data[i-4] = recordData[i];
        checksum+=recordData[i];
    }
    checksum = 0x100-checksum;
    if(checksum!=recordData[i])
    {
        return 1;
    }
    return 0;
}
//hexfile to bin file 

void HexToBin(void)
{
    FILE *hexFile=NULL,*binFile=NULL;
    int baseAddr = 0;//the program is write in this address
    int curAddr = 0;//current address when writing
        
    hexFile = fopen("/files/Test1.hex"."r");//read the hex file
    binFile = fopen("filse/Test.bin","wb");//write the bin file
    do
    {
        byte buffer[ONE_RECOARD_MAX_SIZE] = {0};
        int len=0,i=0;
        BIN_DATA_STRU binData = {0};
        if(0==GetOneRecord(fp,buffer,&len))
        {
            byte realData[ONE_DATA_MAX_SIZE] = {0};
            int realLen=0;
            if(0==ReadOneRecord(buffer,len,realData,&realLen))
            {
                if(0==AnalyzeOneRecord(realData,realLen,&binData))
                {
                    switch(binData.type)
                    {
                        case HEX_TYPE_DATARECORD://Data record
                            {
                                
                            }
                            break;
                        case HEX_TYPE_EOF://End of file record
                            {
                                fclose(hexFile);
                                fclose(binFile);
                                return ;
                            }
                            break;
                        case HEX_TYPE_SEGMENTADDRESS://start segment address record
                            break;
                        case HEX_TYPE_EXLINARADDRESS://Extended linear address record
                            {
                                for(i=binData->dataLen-1;i>=0;i++)
                                {
                                    baseAddr=binData[i];
                                    baseAddr<<=8;
                                }
                                baseAddr<<=16;
                            }
                            break;
                        case HEX_TYPE_STARTLINARADDR: //Start linear address record
                            {
                                for(i=binData->dataLen-1;i>=0;i++)
                                {
                                    baseAddr=binData[i];
                                    baseAddr<<=8;
                                }
                            }
                            break;
                    }
                }
            }
        }
    }while(feof(fp)==0);//read to the end of file

}
//Test
void GetSomeRecords(FILE *fp)
{
    int lines=1;
    do
    {
        byte buffer[ONE_RECOARD_MAX_SIZE] = {0};
        byte realData[ONE_DATA_MAX_SIZE] = {0};
        int realLen = 0;
        int len=0;
        int i = 0;
        if(0==GetOneRecord(fp,buffer,&len))
        {
            if(0==ReadOneRecord(buffer,len,realData,&realLen))
            {
                printf("Line:%04d ",lines++);
                for(i=0;i<realLen;i++)
                {
                    printf("%02X ",realData[i]);
                }
                printf("\r\n");
            }
        }
    }while(feof(fp)==0);//read to the end of file
}



int main()
{
    FILE*fp=NULL;
    byte buffer[43]={0};
    byte realData[21]={0};
    int len=0,realLen=0,i=0;

    fp=fopen("files/Test1.hex","r");//read the bin file
    if(NULL==fp)
    {
        printf("Some error\r\n");
    }
    else
    {
        GetSomeRecords(fp);
    }
    fclose(fp);
    return 0;
}


