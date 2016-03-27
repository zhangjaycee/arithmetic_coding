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
FILE *fp_in;
FILE *fp_encode;
//启用字符频率统计模型，也就是计算各个字符的频率分布区间
void start_model(){
    int i;
    /*
    for (int i = 0; i<No_of_chars; i++) {          
        //为了便于查找
        char_to_index[i] = i+1;                
        index_to_char[i+1] = i;                
    }
    */
    //累计频率cum_freq[i-1]=freq[i]+...+freq[257], cum_freq[257]=0;
    cum_freq[No_of_symbols] = 0;
    for (i = No_of_symbols; i>0; i--) {       
        cum_freq[i-1] = cum_freq[i] + freq[i]; 
    }
    //这条语句是为了确保频率和的上线，这是后话，这里就注释掉
    //if (cum_freq[0] > Max_frequency);   /* Check counts within limit*/
}


//初始化缓冲区，便于开始接受编码值
void start_outputing_bits()
{  
    buffer = 0;                                //缓冲区一开始为空
    bits_to_go = 8;                          
}


void output_bit(int bit)
{  
    //为了写代码方便，编码数据是从右到左进入缓冲区的。记住这一点！
    buffer >>= 1;                              
    if (bit) buffer |= 0x80;
    bits_to_go -= 1;
    //当缓冲区满了的时候，就输出存起来
    if (bits_to_go==0) {                        
        code[code_index]=buffer;
        //
        //printf("%c ",code[code_index]);
        fwrite(&code[code_index], 1, 1, fp_encode);
        //fputc(code[code_index], fp_encode);
        code_index++;

        bits_to_go = 8; //重新恢复为8
    }
}


void done_outputing_bits()
{   
    //编码最后的时候，当缓冲区没有满，则直接补充０
    char ch = buffer>>bits_to_go;
    code[code_index] = ch;
    //fputc(code[code_index], fp_encode);
    fwrite(&code[code_index], 1, 1, fp_encode);
    code_index++;
    //fputc(EOF, fp_encode);
    //fwrite(EOF, 1, 1, fp_encode);
}



static void bit_plus_follow(int);   /* Routine that follows                    */
static code_value low, high;    /* Ends of the current code region          */
static long bits_to_follow;     /* Number of opposite bits to output after */


void start_encoding()
{   
    for(int i=0;i<100;i++)code[i]='\0';

    low = 0;                            /* Full code range.                 */
    high = Top_value;
    bits_to_follow = 0;                 /* No bits to follow           */
}


void encode_symbol(int symbol,int cum_freq[])
{  
    long range;                 /* Size of the current code region          */
    range = (long)(high-low)+1;

    high = low + (range*cum_freq[symbol-1])/cum_freq[0]-1;  /* Narrow the code region  to that allotted to this */
    low = low + (range*cum_freq[symbol])/cum_freq[0]; /* symbol.                  */

    for (;;)
    {                                  /* Loop to output bits.     */
        if (high<Half) {
            bit_plus_follow(0);                 /* Output 0 if in low half. */
        }
        else if (low>=Half) {                   /* Output 1 if in high half.*/
            bit_plus_follow(1);
            low -= Half;
            high -= Half;                       /* Subtract offset to top. */
        }
        else if (low>=First_qtr  && high<Third_qtr) {  /* Output an opposite bit　later if in middle half. */
                bits_to_follow += 1;
                low -= First_qtr;                   /* Subtract offset to middle*/
                high -= First_qtr;
        }
        else break;                             /* Otherwise exit loop.     */
        low = 2*low;
        high = 2*high+1;                        /* Scale up code range.     */
    }
}

/* FINISH ENCODING THE STREAM. */

void done_encoding()
{   
    bits_to_follow += 1;                       /* Output two bits that      */
    if (low<First_qtr) bit_plus_follow(0);     /* select the quarter that   */
    else bit_plus_follow(1);                   /* the current code range    */
}                                              /* contains.                 */


static void bit_plus_follow(int bit)
{  
    output_bit(bit);                           /* Output the bit.           */
    while (bits_to_follow>0) {
        output_bit(!bit);                      /* Output bits_to_follow     */
        bits_to_follow -= 1;                   /* opposite bits. Set        */
    }                                          /* bits_to_follow to zero.   */
}



void  convert_index()
{
    for (int i = 0; i<No_of_chars; i++) {          
        //为了便于查找
        char_to_index[i] = i+1;                
        index_to_char[i+1] = i;                
    }
}
void encode(){
    convert_index();
    start_outputing_bits();
    start_encoding();
    for(int i = 1; i < No_of_symbols + 1; i++){
        freq[i] = 1;
    }
    for (;;) {                                 /* Loop through characters. */
        int ch;
        int symbol;
        ch = fgetc(fp_in);                    /* Read the next character. */
        if(ch == EOF)
            break;
        symbol = char_to_index[ch];            /* Translate to an index.    */
        freq[symbol] += 1;
    }
    for(int i = 0; i < No_of_symbols + 1; i++){
        printf("freq[%d] = %d\n", i, freq[i]);
    }
    start_model();                             /* Set up other modules.     */
    fseek(fp_in, 0, SEEK_SET);
    for (;;) {                                 /* Loop through characters. */
        int ch;
        int symbol;
        ch = fgetc(fp_in);                     /* Read the next character. */
        if(ch == EOF)
            break;
        symbol = char_to_index[ch];            /* Translate to an index.    */
        encode_symbol(symbol,cum_freq);        /* Encode that symbol.       */

    }
    //将EOF编码进去，作为终止符
    encode_symbol(EOF_symbol,cum_freq);       
    done_encoding();                           /* Send the last few bits.   */
    done_outputing_bits();

}

int check_filename(char * name)
{
    int len = strlen(name);
    if(len < 5 || name[len-1] != 't' || name[len-2] != 'x' || name[len-3] != 't' || name[len-4] != '.')
        return -1;
    else
        return len - 4;
}

int main()
{
    char filename_in[40];
    char filename_encode[60];
    char filename_decode[60];
    int filename_len = -1;
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
    strcat(filename_encode,"_encode.txt");
    if(NULL == (fp_encode = fopen(filename_encode, "w"))){
        printf("encode file create failed!\n");
        exit(-1);
    }
    encode();
    fclose(fp_encode); 
    fclose(fp_in); 
    printf("Done!\n");
    return 0;
}