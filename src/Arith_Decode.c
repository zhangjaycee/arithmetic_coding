#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define CODE_BITS 16              /* Number of bits in a code value   */
typedef unsigned int code_t;                /* Type of an arithmetic code value */

#define MAX_VALUE 0xffff      /* Largest code value */

#define No_of_chars 256                 /* Number of character symbols      */
#define EOF_symbol (No_of_chars+1)      /* Index of EOF symbol              */

#define No_of_symbols (No_of_chars+1)   /* Total number of symbols          */

/* TRANSLATION TABLES BETWEEN CHARACTERS AND SYMBOL INDEXES. */

int char_to_index[No_of_chars];         /* To index from character          */
unsigned char index_to_char[No_of_symbols+1]; /* To character from index    */

/* CUMULATIVE FREQUENCY TABLE. */

#define Max_frequency 16383             /* Maximum alled frequency count */
/*   2^14 - 1                       */
int cum_freq[No_of_symbols+1];          /* Cumulative symbol frequencies    */

//固定频率表，为了方便起见
int freq[No_of_symbols+1] = {0};

//用来存储编码值，是编码解码过程的桥梁。大小暂定１００，实际中可以修改
char code[100000];
int code_index=0;
int decode_index=0; 

//buffer为八位缓冲区，暂时存放编码制
int buffer;      
//buffer中还有几个比特没有用到，初始值为8
int bits_to_go;        
//超过了EOF的字符，也是垃圾
int garbage_bits;
FILE *fp_encode;
FILE *fp_decode;
//启用字符频率统计模型，也就是计算各个字符的频率分布区间
void start_model(){
    int i;
    for (i = 0; i<No_of_chars; i++) {          
        //为了便于查找
        char_to_index[i] = i+1;                
        index_to_char[i+1] = i;                
    }
    for(i = 1; i < No_of_symbols + 1; i++){
        fread(&freq[i], 1, 1, fp_encode);
        //printf("freq[%d] = %d\n", i, freq[i]);
    }
    //累计频率cum_freq[i-1]=freq[i]+...+freq[257], cum_freq[257]=0;
    cum_freq[No_of_symbols] = 0;
    for (i = No_of_symbols; i>0; i--) {       
        cum_freq[i-1] = cum_freq[i] + freq[i]; 
    }
    //这条语句是为了确保频率和的上线，这是后话，这里就注释掉
    //if (cum_freq[0] > Max_frequency);   /* Check counts within limit*/
}





void bit_plus_foll(int);   /* Routine that folls                    */
code_t l, u;    /* Ends of the current code region          */


//解码

code_t value;        /* Currently-seen code value                */

void start_inputing_bits()
{   
    bits_to_go = 0;                             /* Buffer starts out with   */
    garbage_bits = 0;                           /* no bits in it.           */
}


int input_bit()
{   
    int t;

    if (bits_to_go==0) {   
        fread(&buffer,1,1,fp_encode);
        if (buffer == EOF) {
            garbage_bits += 1;                      /* Return arbitrary bits*/
            if (garbage_bits>CODE_BITS-2) {   /* after eof, but check */
                fprintf(stderr,"Bad input file/n"); /* for too many such.   */
                    exit(-1);
            }
        }
        bits_to_go = 8;
    }
    //从左到右取出二进制位，因为存的时候是从右到左
    t = buffer&1;                               /* Return the next bit from */
    buffer >>= 1;                               /* the bottom of the byte. */
    bits_to_go -= 1;
    return t;
}

void start_decoding()
{   
    int i;
    value = 0;                                  /* Input bits to fill the   */
    for (i = 1; i<=CODE_BITS; i++) {      /* code value.              */
        value = 2*value+input_bit();
    }


    l = 0;                                    /* Full code range.         */
    u = MAX_VALUE;
}


int decode_symbol(int cum_freq[])
{   
    code_t range;                 /* Size of current code region              */
    int cum;                    /* Cumulative frequency calculated          */
    int symbol;                 /* Symbol decoded */
    range = (code_t)(u-l)+1;
    cum = (((code_t)(value-l)+1)*cum_freq[0]-1)/range;    /* Find cum freq for value. */
        
    for (symbol = 1; cum_freq[symbol]>cum; symbol++) ; /* Then find symbol. */
    u = l + (range*cum_freq[symbol-1])/cum_freq[0]-1;   /* Narrow the code region   *//* to that allotted to this */
    l = l +  (range*cum_freq[symbol])/cum_freq[0];
     for (;;)
    {                                  /* Loop to output bits.     */
        if ((u & 0x8000 ) == (l & 0x8000)) { //u<Half
            l = (l << 1) & MAX_VALUE;
            u = ((u << 1) + 1) & MAX_VALUE;
            value = ((value << 1) + input_bit()) & MAX_VALUE;            // Move in next input blt. 
        }
        else if ((u & 0x8000)==0x8000 && (u & MAX_VALUE | 0xbfff)==0xbfff
                && (l & MAX_VALUE | 0x7fff)==0x7fff && (l & 0x4000)==0x4000 ) {
            l = (l << 1) & MAX_VALUE;
            u = ((u << 1) + 1) & MAX_VALUE;
            value = ((value << 1) + input_bit()) & MAX_VALUE; 
            l = (l ^ (1 << 15)) & MAX_VALUE;
            u = (u ^ (1 << 15)) & MAX_VALUE;
            value = (value ^ (1 << 15)) &MAX_VALUE;
        }
        else break;                             /* Otherwise exit loop.     */
    }
    return symbol;
}

void decode(){
    start_model();                              /* Set up other modules.    */
    start_inputing_bits();
    start_decoding();
    for (;;){                                  /* Loop through characters. */
        int ch; int symbol;
        symbol = decode_symbol(cum_freq);       /* Decode next symbol.      */
        if (symbol==EOF_symbol) break;          /* Exit loop if EOF symbol. */
        ch = index_to_char[symbol];             /* Translate to a character.*/
        fputc(ch,fp_decode);                        /* Write that character.    */
        //putc(ch,stdout);                        /* Write that character.    */
        
    }
}

int check_filename(char * name)
{
    int len = strlen(name);
    if(len < 12){
        return -1;
    }
    if(0 != strcmp(name + len - 11, "_encode.txt")){
        return -1;
    }
    else
        return len - 11;
}
int main()
{
    char filename_encode[60];
    char filename_decode[60];
    int filename_len = -1;
    scanf("%s",filename_encode);
    filename_len = check_filename(filename_encode);
    while(filename_len < 0 || (fp_encode = fopen(filename_encode,"r")) == NULL){
        printf("open failed, try again!\n");
        scanf("%s",filename_encode);
        filename_len = check_filename(filename_encode);
    }
    filename_encode[filename_len] = '\0';
    filename_decode[0] = '\0';
    strcat(filename_decode, filename_encode);
    strcat(filename_decode,"_decode.txt");
    if(NULL == (fp_decode = fopen(filename_decode, "w"))){
        printf("create failed!\n");
        exit(-1);
    }
    decode();
    printf("Done!\n");
    fclose(fp_decode); 
    fclose(fp_encode); 


    return 0;
}
