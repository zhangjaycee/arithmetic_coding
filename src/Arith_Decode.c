#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define Code_value_bits 16              /* Number of bits in a code value   */
typedef long code_value;                /* Type of an arithmetic code value */

#define Top_value (((long)1<<Code_value_bits)-1)      /* Largest code value */


#define First_qtr (Top_value/4+1)       /* Point after first quarter        */
#define Half      (2*First_qtr)         /* Point after first half           */
#define Third_qtr (3*First_qtr)         /* Point after third quarter        */

#define No_of_chars 256                 /* Number of character symbols      */
#define EOF_symbol (No_of_chars+1)      /* Index of EOF symbol              */

#define No_of_symbols (No_of_chars+1)   /* Total number of symbols          */

/* TRANSLATION TABLES BETWEEN CHARACTERS AND SYMBOL INDEXES. */

int char_to_index[No_of_chars];         /* To index from character          */
unsigned char index_to_char[No_of_symbols+1]; /* To character from index    */

/* CUMULATIVE FREQUENCY TABLE. */

#define Max_frequency 16383             /* Maximum allowed frequency count */
/*   2^14 - 1                       */
int cum_freq[No_of_symbols+1];          /* Cumulative symbol frequencies    */

//固定频率表，为了方便起见
int freq[No_of_symbols+1] = {0};

//用来存储编码值，是编码解码过程的桥梁。大小暂定１００，实际中可以修改
char code[100000];
static int code_index=0;
static int decode_index=0; 

//buffer为八位缓冲区，暂时存放编码制
static int buffer;      
//buffer中还有几个比特没有用到，初始值为8
static int bits_to_go;        
//超过了EOF的字符，也是垃圾
static int garbage_bits;
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





static void bit_plus_follow(int);   /* Routine that follows                    */
static code_value low, high;    /* Ends of the current code region          */
static long bits_to_follow;     /* Number of opposite bits to output after */


//解码

static code_value value;        /* Currently-seen code value                */

void start_inputing_bits()
{   
    bits_to_go = 0;                             /* Buffer starts out with   */
    garbage_bits = 0;                           /* no bits in it.           */
}


int input_bit()
{   
    int t;
    //char char_in;

    if (bits_to_go==0) {   
        //buffer = fgetc(fp_encode); 
        fread(&buffer,1,1,fp_encode);
        //buffer = code[decode_index];
        //decode_index++;
        if (buffer == EOF) {
      //if(decode_index > code_index ){
            garbage_bits += 1;                      /* Return arbitrary bits*/
            if (garbage_bits>Code_value_bits-2) {   /* after eof, but check */
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
    for (i = 1; i<=Code_value_bits; i++) {      /* code value.              */
        value = 2*value+input_bit();
    }


    low = 0;                                    /* Full code range.         */
    high = Top_value;
}


int decode_symbol(int cum_freq[])
{   
    long range;                 /* Size of current code region              */
    int cum;                    /* Cumulative frequency calculated          */
    int symbol;                 /* Symbol decoded */
    range = (long)(high-low)+1;
    cum = (((long)(value-low)+1)*cum_freq[0]-1)/range;    /* Find cum freq for value. */
        
    for (symbol = 1; cum_freq[symbol]>cum; symbol++) ; /* Then find symbol. */
    high = low + (range*cum_freq[symbol-1])/cum_freq[0]-1;   /* Narrow the code region   *//* to that allotted to this */
    low = low +  (range*cum_freq[symbol])/cum_freq[0];

    for (;;) {                                  /* Loop to get rid of bits. */
        if (high<Half) {
            /* nothing */                       /* Expand low half.         */
        }
        else if (low>=Half) {                   /* Expand high half.        */
            value -= Half;
            low -= Half;                        /* Subtract offset to top. */
            high -= Half;
        }
        else if (low>=First_qtr && high <Third_qtr) {
            value -= First_qtr;
            low -= First_qtr;                   /* Subtract offset to middle*/
            high -= First_qtr;
        }
        else break;                             /* Otherwise exit loop.     */
        low = 2*low;
        high = 2*high+1;                        /* Scale up code range.     */
        value = 2*value+input_bit();            /* Move in next input blt. */
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
