#include "includes.h"
#include <stdlib.h>
#include <string.h>

#define HEXFORMAT_LEN				 (11)
#define ONE_RECORD_MAX_SIZE  (523) //DATA_SIZE*2 + HEX_FORMAT_LEN + NEW_LINE_LEN
#define ONE_DATA_MAX_SIZE    (260)  //DATA_SIZE + 5
#define DATA_SIZE            (255)


//type
#define HEX_TYPE_DATARECORD        (0x00)
#define HEX_TYPE_EOF               (0x01)
#define HEX_TYPE_EXSEGMENTADDRESS  (0X02)
#define HEX_TYPE_SEGMENTADDRESS    (0x03)
#define HEX_TYPE_EXLINEARADDRESS   (0x04)
#define HEX_TYPE_STARTLINEARADDR   (0X05)




typedef unsigned char Byte;

typedef struct
{
    int offset;
    int size;
    Byte type;
    Byte data[DATA_SIZE];
}BinData;


//address data
typedef struct
{
	int address;
	int data_size;
	long data_pos;
}Address;

//node struct 
typedef struct Node
{
	Address addr;
	struct Node *prev;
	struct Node *next; 
}AddrNode;


//fp:file poniter
//buffer: save the data read from file
//len :length of data
//return 
//  0 success
//  1 error
int GetOneRecord(FILE *fp,Byte *buffer,int *len)
{
    Byte cur_char = 0;
    int count = 0,timeout = ONE_RECORD_MAX_SIZE;
    while(timeout--)
    {
        //curChar=fgetc(fp);
				fread(&cur_char,1,1,fp);
        buffer[count++] = cur_char;
				//linux: 0x0D 0x0A  "\n"
				//windows: 0x0A		"\r\n"
				if(cur_char== '\n')
        {
            *len = count;
            return 0;//complete
        }
    }
    return 1;
}

//judge the charater is hex asiic 0-9 or A-F
int IsHex(char c)
{
	if(((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')))
	{
		return 1;		//true
	}
	else
	{
		return 0; //false
	}
}

//translate the two asiic character to hex character
int Asiic2Hex(Byte *hex_buff,Byte *asiic_buff,int asiic_len)
{
	int i = 0;
	Byte one_hex = 0;
	for( i = 0 ; i < asiic_len -1 ; i += 2)
	{
		Byte hi = asiic_buff[i];
		Byte lo = asiic_buff[i + 1];
		if(IsHex(hi) && IsHex(lo))
		{
					one_hex = (hi >= 'A')?(hi - 'A' + 10):(hi - '0');
					one_hex = (one_hex << 4) + ((lo >= 'A')?(lo - 'A' + 10):(lo - '0'));
					hex_buff[i / 2]  = one_hex;
		}
		else
		{
			return 1;			//format error
		}
	}
	return 0;
}

//translate bin data to hehx string
int Hex2Asiic(Byte *bin_data,int bin_data_len,char *str)
{
	char *ptr = str;
	int i =  0;
	for(i = 0 ; i < bin_data_len ; ++i)
	{
#ifdef _WIN32
		sprintf_s(ptr,bin_data_len * 2 + 1,"%02X"	,bin_data[i]);
#else
		sprintf(ptr,"%02X",bin_data[i]);
#endif
	}
	ptr += 2;
	str[bin_data_len * 2] = '\0';
	return 0;
}


//raw_data: the raw data
//raw_len:  the length of raw_data
//real_data: assic to byte
//real_len: the data len
//return 
//  0 success
//  1 error
int ReadOneRecord(Byte *raw_data,int raw_len,Byte *real_data,int *real_len)
{
    //int i = 1,count = 0;	//first byte is ':' and the last two byte is '\r' '\n'
    //Byte one_data = 0,one_data_complete = 0;
    int hex_len = 0;	//hex asiic character length
		if(raw_data[raw_len - 2] == '\r')
		{
			hex_len = raw_len - 3;
		}
		else
		{
			hex_len = raw_len - 2;
		}

		if(0 == Asiic2Hex(real_data,&raw_data[1],hex_len))
		{
			*real_len = hex_len / 2;
			return 0;
		}
		return 1;
}

//record_data: the record data
//record_len : the length of record data
//bin_data : save the one record data to bin data
//return 
//   0 success
//   1 checksum is error

int AnalyzeOneRecord(Byte *record_data,int record_len,BinData *bin_data)
{
    int i = 0;
    Byte checksum = 0;

    bin_data->size = record_data[0];	//the data length
    checksum +=record_data[0];
    bin_data->offset = ((int)record_data[1] << 8) + record_data[2];	//the offset
    checksum += record_data[1] + record_data[2];
    bin_data->type = record_data[3];
    checksum += record_data[3];
    for(i = 4 ; i < record_len - 1 ; ++i)
    {
        bin_data->data[i - 4] = record_data[i];
        checksum += record_data[i];
    }
    checksum = 0x100 - checksum;
    if(checksum != record_data[i])
    {
        return 1;
    }
    return 0;
}

//handle the data
//fp:the hex file handler
//bin_data: the stucture of one record of hex file
int HandleData(FILE* fp,BinData *bin_data)
{
    Byte buffer[ONE_RECORD_MAX_SIZE] ={ 0 };
    Byte real_data[ONE_DATA_MAX_SIZE] = { 0 };

		int len = 0;
    int real_len = 0;

    if(0 == GetOneRecord(fp,buffer,&len))
    {
        if(0==ReadOneRecord(buffer,len,real_data,&real_len))
        {
            if(0==AnalyzeOneRecord(real_data,real_len,bin_data))
            {
                return 0;//ok
            }
        }
    }
    return 1;//error
}

//hexfile to bin file 
//input:the path of input hex file
//output:the path of output bin file
//return 
//   0 OK
//   1 error
int HexToBin(char *input,char *output)
{
    FILE *hex_file = NULL,*bin_file = NULL;
		int start_addr = 0; //the start address of application
		int base_addr = 0; //base address in record
		int cur_addr = 0; //current address when writing

    BinData bin_data = {0};
    int i = 0;
        
    //read the hex file
#ifdef _WIN32
    if(fopen_s(&hex_file,input,"rb") != 0)
    {
        return 1;
    }
    //write the bin file
    if(fopen_s(&hex_file,output,"wb") != 0)
    {
        return 1;
    }
#else
		hex_file = fopen(input,"rb");
		if(hex_file == NULL)
		{
			return 1;
		}
		bin_file = fopen(output,"wb");
		if(bin_file == NULL)
		{
			return 1;
		}
#endif
		printf("start...\r\n");
    while(feof(hex_file) == 0)
    {
        if(HandleData(hex_file,&bin_data) == 0)
        {
            switch(bin_data.type)
            {
                case HEX_TYPE_DATARECORD://Data record
                    {

												if(start_addr == 0)
												{
													start_addr = base_addr + bin_data.offset;	//record the start address in application
												}
                        //assume the base address not change
                        cur_addr = ftell(bin_file);
                       // oldBaseAddr = oldBaseAddr?oldBaseAddr:baseAddr;//if oldBaseAddr==0 and copy baseAddr to oldBaseAddr
                        if(cur_addr < (bin_data.offset + base_addr - start_addr))
                        {
                            //pad with 0x00
                            for(i = (bin_data.offset + base_addr - start_addr - cur_addr); i > 0 ; --i)
                            {
                                fputc('\0',bin_file);
                            }
                        }
                        fwrite(bin_data.data,1,bin_data.size,bin_file);
                    }
                    break;
                case HEX_TYPE_EOF://End of file record
                    {
                        fclose(hex_file);
                        fclose(bin_file);
                        return 0;
                    }
                case HEX_TYPE_EXSEGMENTADDRESS:
                    break;
                case HEX_TYPE_SEGMENTADDRESS://start segment address record
                    break;
                case HEX_TYPE_EXLINEARADDRESS://Extended linear address record
                    {
                       // oldBaseAddr = baseAddr;//save the old  base address
                        base_addr = 0;//clear
                        for(i = 0 ; i < bin_data.size ; ++i)
                        { 
                            base_addr += bin_data.data[i];
                            if( i < (bin_data.size - 1))
                            {
                                base_addr <<= 8;
                            }                          
                        }
                        base_addr <<= 16;
                    }
                    break;
                case HEX_TYPE_STARTLINEARADDR: //Start linear address record
                    {
                        //oldBaseAddr = baseAddr;//save the old  base address
                        base_addr = 0;//clear
                        for( i = 0 ; i < bin_data.size ; ++i)
                        {  
                            base_addr += bin_data.data[i];
                            if(i < (bin_data.size - 1))
                            {
                                base_addr <<= 8;
                            }                          
                        }
                    }
                    break;
            }
        }
    }
    fclose(hex_file);
    fclose(bin_file);
    return 0;
}


//item be add to list
////head_ptr: head of list
////item : will be add to list
void AddToList(AddrNode **head_ptr,AddrNode *item)
{
	AddrNode *ptr = *head_ptr;
	if(ptr == NULL)	//the list is none
	{
		*head_ptr = item;
		return;
	}
	//find the right position
	while((ptr->next != NULL) && (ptr->addr.address <= item->addr.address))
	{
		ptr = ptr->next;
	}

	//there are one item in list
	if(ptr->prev == NULL)
	{
		if(ptr->addr.address > item->addr.address)
		{
			*head_ptr = item;
			item->next = ptr;
			ptr->prev  = item;
		}
		else
		{
			ptr->next = item;
			item->prev = ptr;
		}
	}
	else
	{
		//it is rear if list
		if(ptr->next == NULL)
		{
			ptr->next = item;
			item->prev = ptr;
		}
		else
		{
			ptr->prev->next = item;
			ptr->prev = item;
			item->prev = ptr->prev;
			item->next = ptr;
		}
	}

}


//analyze the hex file and get the address of data
////hex_file: hex file handle
////nodes_ptr:the head of node
////addr_count: the count of address record
//// return:
////	0 success
////	1 error
int AnalyzeAddr(FILE *hex_file,AddrNode **nodes_ptr,int *addr_count)
{
	BinData bin_data = { 0 };

	int base_addr  = 0;	//base address of record 

	int i = 0 ;

	int count  = 0; //the count of address

	while(feof(hex_file)  == 0)
	{
		if(HandleData(hex_file,&bin_data) == 0)
		{
			switch(bin_data.type)
			{
				case HEX_TYPE_DATARECORD:	//data record 
					{
						AddrNode *one_node = (AddrNode*)malloc(sizeof(AddrNode));
						one_node->addr.address = bin_data.offset + base_addr;
						one_node->addr.data_size = bin_data.size;
						fseek(hex_file,-2,SEEK_CUR);	//back 2 byte ,check this byte is '\r'
						if(fgetc(hex_file) == '\r')
						{
							one_node->addr.data_pos = ftell(hex_file) - ((bin_data.size + 1) * 2 + 1); //newline  mark and checksum 
						}
						else
						{
							one_node->addr.data_pos = ftell(hex_file) - ((bin_data.size + 1) * 2 + 0);
						}
						fseek(hex_file , 1, SEEK_CUR); //set the postion to the start of line
						one_node->next = NULL;
						one_node->prev = NULL;
						AddToList(nodes_ptr,one_node);
						++count;
					//	printf("I:%d A:%0X D:%0X P:%0X \r\n",i,one_node->addr.address,one_node->addr.data_size,one_node->addr.data_pos);

					}
					break;
				case HEX_TYPE_EOF:	//end of file record
					{
						*addr_count = count;
						return 0;
					}
					break;
				case HEX_TYPE_EXSEGMENTADDRESS:	//Extended segement address record 
					{
						if(bin_data.size == 2)
						{
							base_addr = ((bin_data.data[0] << 8) + bin_data.data[1]) << 4;
						}
					}
					break;
				case HEX_TYPE_SEGMENTADDRESS:		//start segement address record
					break;
				case HEX_TYPE_EXLINEARADDRESS:	//Extended linear address record
					{
						base_addr = 0; //clear
						for( i = 0 ; i < bin_data.size; ++i)
						{
							base_addr += bin_data.data[i];
							if(i < (bin_data.size - 1))
							{
								base_addr <<= 8;
							}
						}
						base_addr <<= 16;
					}
					break;
				case HEX_TYPE_STARTLINEARADDR:	//start linear address record
					{
						base_addr = 0;
						for(i = 0 ; i < bin_data.size ; ++i)
						{
							base_addr += bin_data.data[i];
							if(i < (bin_data.size - 1))
							{
								base_addr <<= 8;
							}
						}
					}
					break;
				default:
					break;

			}
		}
	}
	*addr_count = count;
	return 0;

}


void Swap(Address *array1 , Address *array2)
{
	Address temp = *array1;
	*array1 = *array2;
	*array2 = temp;
}


//Shell sort the array of address by ascending
////array: the array of address
////length: the length of array
//// return:
////	1 success
////  0 error
int AddSort(Address *array,int length)
{
	int step = 1;
	int i = 0, j = 0;
	while(step < (length  / 3))
	{
		step = 3 * step + 1;
	}
	while(step >= 1)
	{
		for( i = step ; i < length; ++i)
		{
			for(j = i; (j > step) && (array[j].address  < array[j - step].address) ; j -= step)
			{
				Swap(&array[j],&array[j - step]); //swap
			}
		}
		step /= 3; //change the step
	}
	return 0;
}

//copy the node to array
////array:array of address
////head:head of node
void NodeListCopyToArray(Address *array , AddrNode *head)
{
	int i = 0;
	AddrNode *ptr = NULL;
	while(head != NULL)
	{
		array[i++] = head->addr;
		ptr = head;
		head = head->next;
		free(ptr);
	}
}

//write data to bin file
////binFile:bin file handle
////hexFile:hex file handle
////addrArray: array of address
////length: the length of addrArray
////padding: padding with this char
int BinWrite(FILE *bin_file , FILE *hex_file,AddrNode *head, int length,Byte padding)
{
	if(head == NULL)
	{
		return 1;	//error
	}
	int base_addr = head->addr.address;
	int base_addr_file = 0;
	int cur_addr = 0;		//current address when writing

	int i = 0 , j = 0;
	Byte *buffer = (Byte*)malloc(2 * DATA_SIZE * sizeof(Byte));		//save the asiic character
	Byte *bin_buff = (Byte*)malloc(DATA_SIZE * sizeof(Byte));

	base_addr_file = ftell(bin_file);		//write bin data from here
	while(head != NULL)
	{
		fseek(hex_file ,head->addr.data_pos,SEEK_SET);	//set file position to data position
		fread(buffer,1,(head->addr.data_size * 2),hex_file);
		if(0 == Asiic2Hex(bin_buff,buffer,(head->addr.data_size * 2)))
		{
			cur_addr = ftell(bin_file);
			if((cur_addr - base_addr_file) < (head->addr.address - base_addr))
			{
				for( j = (head->addr.address - base_addr - (cur_addr - base_addr_file)); j > 0;--j)
				{
					fputc(padding,bin_file);
				}
			}
			fwrite(bin_buff,1,head->addr.data_size,bin_file);
			head = head->next;
		}
		else
		{
			return 2;
		}
	}
	free(buffer);
	free(bin_buff);
	return 0;
}



//override HexToBin
int HexToBin2(char *input,char *output)
{
	
    FILE *hex_file = NULL,*bin_file = NULL;

       
		AddrNode *node_ptr = NULL;

		int addr_count  = 0; //the count of address

    //read the hex file
#ifdef _WIN32
    if(fopen_s(&hex_file,input,"rb") != 0)
    {
        return 1;
    }
    //write the bin file
    if(fopen_s(&hex_file,output,"wb") != 0)
    {
        return 1;
    }
#else
		hex_file = fopen(input,"rb");
		if(hex_file == NULL)
		{
			return 1;
		}
		bin_file = fopen(output,"wb");
		if(bin_file == NULL)
		{
			return 1;
		}
#endif

	printf("start...\r\n");
	if(0 == AnalyzeAddr(hex_file,&node_ptr,&addr_count))
	{
		printf("Num:%d " ,addr_count); 
		if(0 == BinWrite(bin_file,hex_file,node_ptr,addr_count,0x00))
		{
			printf("OK\r\n");
		}
		else
		{
			return 3;	//write error
		}
	}
	else
	{
		return 2;	//analyze error
	}
	return 0;

}

/*
//Test
void GetSomeRecords(FILE *fp)
{
    int lines=1;
    do
    {
        byte buffer[ONE_RECORD_MAX_SIZE] = {0};
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


//test handleData function
void Test_HandleData(void)
{
    FILE *fp=NULL;
    BIN_DATA_STRU binData = {0};
    int i = 0;
    int lines = 5;

    fp = fopen("files/Test1.hex","r");//read the hex file
    if(NULL==fp)
    {
        printf("Open file error!");
    }
    else
    {
       while(feof(fp)==0)
       {
            if(handleData(fp,&binData)==0)
            {
                printf("DataLen:%02X\r\n",binData.dataLen);
                printf("offSet:%02X\r\n",binData.offset);
                printf("type:%02X\r\n",binData.type);
                printf("Data:");
                for(i=0;i<binData.dataLen;i++)
                {
                    printf("%02X ",binData.data[i]);
                }
                printf("\r\n");
                printf("P:%ld\r\n",ftell(fp));
            }
       }

    }
    
}
*/

int main()
{
/*
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
        //GetSomeRecords(fp);
	//printf("OK\r\n");
	if(GetOneRecord(fp,buffer,&len)==0)
	{
		for(i=0;i<len;i++)
		{
			printf("%02X ",buffer[i]);
		}
		printf("\r\n");
	}
    }
    fclose(fp);
*/

    char hexFilePath[] = "files/Test_Block.hex";
    char binFilePath[] = "files/Test_Block123.bin";

   // if(0 != HexToBin2(hexFilePath,binFilePath))
	 if(0 != HexToBin(hexFilePath,binFilePath))
    {
        printf("Some error\r\n");
    }
    else
    {
        printf("OK\r\n");
    }

    //Test_HandleData();


    return 0;
}


