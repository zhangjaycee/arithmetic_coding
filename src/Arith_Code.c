#include<stdio.h>
#include<stdlib.h>
#include<string.h>


#define CODE_BITS 16              
#define MAX_VALUE 0xffff      
#define No_of_chars 256                 //8位2进制一共有256种组合
#define EOF_symbol (No_of_chars+1)      //No_of_chars没有查到字符，则说明是EOF
#define No_of_symbols (No_of_chars+1)   // 总可读字符数
typedef long long code_t;               
code_t l, u;
int char_to_index[No_of_chars];         //用于查询字符转为索引数字的表
unsigned char index_to_char[No_of_symbols+1]; //用于查询索引数字转为字符的表
int cum_freq[No_of_symbols+1];         
int freq[No_of_symbols+1] = {0};
int buffer;      
int bits_to_go;        
FILE *fp_in;
FILE *fp_encode;

void output(int);  
void output_bit();
int scale3;   
int size;
int base;
void init();
void output(int bit);
void output_bit(int bit);
void encode();
void encode_symbol(int symbol,int cum_freq[]);
void done_encoding();
int check_filename(char * name);

int main()
{
    char filename_in[40];
    char filename_encode[60];
    int filename_len = -1;
    printf("===13020188003  张佳辰  数据压缩大作业1---算术编码(二进制)压缩程序===\n");
    //提示输入文件
   // printf("Please enter \"base\":\n");
   // scanf("%d",&base);
    printf("Please enter filename you want to encode:\n");
    scanf("%s",filename_in);
    filename_len = check_filename(filename_in);
    while(filename_len < 0 || (fp_in = fopen(filename_in,"r")) == NULL){
        printf("Open failed, please try again:\n");
        scanf("%s",filename_in);
        filename_len = check_filename(filename_in);
    }
    filename_in[filename_len] = '\0';
    filename_encode[0] = '\0';
    strcat(filename_encode, filename_in);
    strcat(filename_encode,"_encode.raw");
    strcat(filename_in,".raw");
    if(NULL == (fp_encode = fopen(filename_encode, "w"))){
        printf("encode file create failed!\n");
        exit(-1);
    }
    //压缩
    encode();
    //计算文件大小
    fseek (fp_in, 0, SEEK_END); 
    fseek (fp_encode, 0, SEEK_END);  
    int size0=ftell (fp_in);
    int size1=ftell (fp_encode);
    printf ("压缩前文件%s: \t\t%d bytes.\n", filename_in, size0);
    printf ("压缩后文件%s: \t%d bytes.\n", filename_encode, size1);
    printf("压缩率为:\t\t\t %.2f%%\n",(float)size1/size0*100);
    fclose(fp_encode); 
    fclose(fp_in); 
    printf("===13020188003  张佳辰  数据压缩大作业1---算术编码(二进制)压缩程序===\n");
    return 0;
}



void init(){
    int i;
    for (int i = 0; i<No_of_chars; i++) {          
        char_to_index[i] = i+1;                
        index_to_char[i+1] = i;                
    }
    fseek(fp_in, 0, SEEK_END);
    size=ftell (fp_in);
    fseek(fp_in, 0, SEEK_SET);
    //统计频率并写入压缩文件
    base = (size/MAX_VALUE + 1) * 2;
    printf("base:%d\n",base);
    for(int i = 1; i < No_of_symbols + 1; i++){
        freq[i] = base;
    }
    int count = 0;
    for (int i = 0; i < size; i++) {  
            //printf("[i]%d\n", i);
        count++;
        //printf("initing... %d\n", count);
        unsigned char ch;
        int symbol;
        //ch = fgetc(fp_in); 
        fread(&ch, 1, 1, fp_in);
        //if(ch == EOF)
        //   break;
        symbol = char_to_index[ch]; 
        freq[symbol] += 1;
    }
    //
    fwrite(&size, 4, 1, fp_encode);
    for(int i = 1; i < No_of_symbols + 1; i++){
        fwrite(&freq[i], 4, 1, fp_encode);
    }
    cum_freq[No_of_symbols] = 0;
    for (i = No_of_symbols; i>0; i--) {       
        cum_freq[i-1] = cum_freq[i] + freq[i]; 
        //printf("cum_freq[%d]=%d\n", i-1, cum_freq[i-1]);
    }
    buffer = 0;   //缓冲区开始为空
    bits_to_go = 8;                          
    l = 0;                          
    u = MAX_VALUE;
    scale3 = 0;
}


void output_bit(int bit)
{  
    //编码数据是从右到左进入缓冲区
    buffer >>= 1;                              
    if (bit) buffer |= 0x80;
    bits_to_go -= 1;
    //当缓冲区满了的时候输出
    if (bits_to_go<=0) {                        
        //printf("bbb\n");
        fwrite(&buffer, 1, 1, fp_encode);
        bits_to_go = 8; //缓冲区剩余位数恢复为8
    }
}



void encode_symbol(int symbol,int cum_freq[])
{  
    code_t range;  
    range = (code_t)(u-l)+1;
    
    u = l + (range*cum_freq[symbol-1])/cum_freq[0]-1;  
    l = l + (range*cum_freq[symbol])/cum_freq[0];
    for (;;)
    {                                  
        if ((u & 0x8000 ) == (l & 0x8000)) { 
            //printf("aaa\n");
            output((l & 0x8000) == 0x8000);
            l = (l << 1) & MAX_VALUE;
            u = ((u << 1) + 1) &MAX_VALUE ;
        }
        else if ((u & 0x8000)==0x8000 && (u & MAX_VALUE | 0xbfff)==0xbfff
                && (l & MAX_VALUE | 0x7fff)==0x7fff && (l & 0x4000)==0x4000 ) {
            //printf("aaa\n");
            scale3 += 1;
            l = (l << 1) & MAX_VALUE;
            u = ((u << 1) + 1) &MAX_VALUE ;
            l = (l ^ (1 << 15)) & MAX_VALUE;
            u = (u ^ (1 << 15)) & MAX_VALUE;
        }
        else break; 
    }
}


void done_encoding()
{   
    scale3 += 1;
    if ((l & MAX_VALUE | 0x7fff)==0x7fff && (l & 0x4000)==0x4000 ) 
        output(0);
    else output(1);
    unsigned char ch = buffer>>bits_to_go;
    fwrite(&ch, 1, 1, fp_encode);
}       


void output(int bit)
{  
    output_bit(bit); 
    while (scale3>0) {
        output_bit(!bit);
        scale3 -= 1;   
    }   
}



void encode(){
    init(); 
    //printf("init ok\n");
    fseek(fp_in, 0, SEEK_SET);
    for (int i = 0; i < size; i++) {  
        unsigned char ch;
        int symbol;
        //ch = fgetc(fp_in); 
        fread(&ch, 1, 1, fp_in);
      //  if(ch == EOF)
      //      break;
        symbol = char_to_index[ch]; 
        encode_symbol(symbol,cum_freq);
    //    if(symbol < 40) 
    //    {printf("[%d] [bits_to_go]%d [l]%lld [u]%lld [ch]%c [symbol]%d\n", i, bits_to_go,l,u,ch,symbol);}
    }
    //将EOF编码进去，作为终止符
    //encode_symbol(EOF_symbol,cum_freq);       
    done_encoding(); //编码结束将最后不满的缓冲区写入
}

int check_filename(char * name)
{
    int len = strlen(name);
    if(len < 5 || name[len-1] != 'w' || name[len-2] != 'a' || name[len-3] != 'r' || name[len-4] != '.')
        return -1;
    else
        return len - 4;
}

