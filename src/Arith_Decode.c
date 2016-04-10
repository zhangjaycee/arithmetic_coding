#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define CODE_BITS 16
#define MAX_VALUE 0xffff      
#define No_of_chars 256                 //8位二进制一共有256种组合
#define EOF_symbol (No_of_chars+1)      //No_of_chars没有查到字符，则说明是EOF
#define No_of_symbols (No_of_chars+1)   // 总可读字符数
typedef long long code_t;           
code_t l, u, value, range;
int char_to_index[No_of_chars];         //用于查询字符转为索引数字的表
unsigned char index_to_char[No_of_symbols+1]; //用于查询索引数字转为字符的表
int cum_freq[No_of_symbols+1];      
int freq[No_of_symbols+1] = {0};
int buffer;      
int size;
int bits_to_go;        
FILE *fp_encode;
FILE *fp_decode;

void init();
int input_bit();
void decode();
int decode_symbol();
void check_filename(char *);
char filename_encode[60];
char filename_decode[60];

int main()
{
    int filename_len = -1;
    printf("===13020188003  张佳辰  数据压缩大作业1---算术编码(二进制)解压缩程序===\n");
    printf("please enter the filenname you want to decode:\n");
    scanf("%s",filename_encode);
    filename_len = strlen(filename_encode);
    while(filename_len < 0 || (fp_encode = fopen(filename_encode,"r")) == NULL){
        printf("open failed, try again!\n");
        scanf("%s",filename_encode);
        filename_len = strlen(filename_encode);
    }
    check_filename(filename_encode);
    decode();
    fseek (fp_encode, 0, SEEK_END); 
    fseek (fp_decode, 0, SEEK_END);  
    int size0=ftell (fp_encode);
    int size1=ftell (fp_decode);
    printf ("解压前文件%s: \t\t%d bytes.\n", filename_encode, size0);
    printf ("解压后文件%s: \t%d bytes.\n", filename_decode, size1);
    printf("压缩率为:\t\t\t %.2f%%\n",(float)size0/size1*100);
    printf("===13020188003  张佳辰  数据压缩大作业1---算术编码(二进制)解压缩程序===\n");
    fclose(fp_decode); 
    fclose(fp_encode); 


    return 0;
}

void init(){
    int i;
    fread(&size, 4, 1, fp_encode);
    for (i = 0; i<No_of_chars; i++) {          
        char_to_index[i] = i+1;                
        index_to_char[i+1] = i;                
    }
    //读入字符频率
    for(i = 1; i < No_of_symbols + 1; i++){
        fread(&freq[i], 4, 1, fp_encode);
    }
    //计算cum_freq数组
    cum_freq[No_of_symbols] = 0;
    for (i = No_of_symbols; i>0; i--) {       
        cum_freq[i-1] = cum_freq[i] + freq[i]; 
    }
    bits_to_go = 0;
    value = 0;  
    for (i = 1; i<=CODE_BITS; i++) {  
        value = ((value << 1) + input_bit()) & MAX_VALUE;// 读取下一位 
    }
    l = 0;  
    u = MAX_VALUE;
}

int input_bit()
{   
    int t;
    if (bits_to_go==0) {   
        if(buffer == EOF){
                return EOF_symbol;
        }
        fread(&buffer,1,1,fp_encode);
        bits_to_go = 8;
    }
    //从左到右取出二进制位，因为存的时候是从右到左
    t = buffer&1;
    buffer >>= 1;
    bits_to_go -= 1;
    return t;
}

int decode_symbol()
{   
    int cum;
    int symbol;
    range = (code_t)(u-l)+1;
    cum = (((code_t)(value-l)+1)*cum_freq[0]-1)/range;
    for (symbol = 1; cum_freq[symbol]>cum; symbol++) ; // 确定是哪个字符
    return symbol;
}

void decode(){
    unsigned char ch; 
    int symbol;
    init(); //解码前的一些初始化
    for(int i = 0; i< size; i++){ //开始循环解码
	printf("\r[");
        int count = 0;
        for(int j = 0; j < i; j+=size/50){
            printf("#");
            count ++;
        }
        while(count++ < 50){
            printf(".");
        }
        printf(" %5.2f%% ]",(float)i/size*100);
        symbol = decode_symbol();
        //if (symbol==EOF_symbol) 
        //    break; //读到EOF，解码完成 
        ch = index_to_char[symbol];//索引转换成字符
        //fputc(ch,fp_decode); //将解码结果写入到文件
        fwrite(&ch, 1, 1, fp_decode);
        u = l + (range*cum_freq[symbol-1])/cum_freq[0]-1; 
        l = l +  (range*cum_freq[symbol])/cum_freq[0];
        for(;;){  //调整映射范围                                
            if ((u & 0x8000 ) == (l & 0x8000)) { //e1和e2条件
                l = (l << 1) & MAX_VALUE;
                u = ((u << 1) + 1) & MAX_VALUE;
                value = ((value << 1) + input_bit()) & MAX_VALUE;// 读取下一位 
            }
            else if ((u & 0x8000)==0x8000 && (u & MAX_VALUE | 0xbfff)==0xbfff
                    && (l & MAX_VALUE | 0x7fff)==0x7fff && (l & 0x4000)==0x4000 ) {//e3条件
                l = (l << 1) & MAX_VALUE;
                u = ((u << 1) + 1) & MAX_VALUE;
                value = ((value << 1) + input_bit()) & MAX_VALUE; 
                l = (l ^ (1 << 15)) & MAX_VALUE;
                u = (u ^ (1 << 15)) & MAX_VALUE;
                value = (value ^ (1 << 15)) &MAX_VALUE;
            }
            else break;
        }
    }
    printf("\r[");
    for(int j = 0; j < 50; j++){
        printf("#");
    }
    printf(" 100.00%% ]\n");
}

void check_filename(char * name)
{
    int len = strlen(name);
    strcat(filename_decode, name);
    filename_decode[len -4] = '\0';
    strcat(filename_decode, "_decode");
    char suffix_len;
    suffix_len = fgetc(fp_encode);
    suffix_len -= '0';
    int i;
    for(i = 0; i < suffix_len; i++){
        filename_decode[len+3+i] = fgetc(fp_encode);
    }
    filename_decode[len+3+i] = '\0';
    if(NULL == (fp_decode = fopen(filename_decode, "w"))){
        printf("create failed!\n");
        exit(-1);
    }
}
