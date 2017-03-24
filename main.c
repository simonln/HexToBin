#include "includes.h"

#define ONE_RECOARD_MAX_SIZE (43)
#define ONE_DATA_MAX_SIZE    (21)



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


