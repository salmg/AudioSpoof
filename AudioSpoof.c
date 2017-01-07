/* Original code - Magnetic Stripe Encoder by geohot
*
- Modified by Salvador Mendoza (https://salmg.net/2017/01/06/how-to-transmit-mag-stripe-info-through-audio/)
- Adding some code from Samy Kamkar 
- for a better parity check and wave generation (https://github.com/samyk/magspoof/)
-
- Thanks to Luis Colunga (https://twitter.com/sinnet3000)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char wave_buffer[0x10000];
int wave_ptr=0;
unsigned char highchunk[]={ 0xff,0xff,0xff,0xff,0xff };
unsigned char lowchunk[]={ 0,0,0,0,0 };
unsigned int curTrack = 0;
struct riff_chunk{
       char chunk_id[4];
       int file_size;
       char riff_type[4];
};
struct fmt_chunk{
       char chunk_id[4];
       int chunk_size;
       short int compression_code;
       short int num_channels;
       int sample_rate;
       int bytes_second;
       short int block_align;
       short int bits_sample;
};
const int sublen[] = {
  32, 48, 48 };
const int bitlen[] = {
  7, 5, 5 };
const char* tracks[] = {
    "%B123456781234567^LASTNAME/FIRST^YYMMSSSDDDDDDDDDDDDDDDDDDDDDDDDD?\0", // Track 1
    ";123456781234567=00000001111111111111?\0" // Track 2
};
int dir;

void lowChunk(int many){
    for(int n=0; n<many; n++){
        memcpy(wave_buffer+wave_ptr, lowchunk, sizeof(lowchunk)); 
        wave_ptr+=sizeof(lowchunk);
    }
}
void highChunk(int many){
    for(int n=0; n<many; n++){
        memcpy(wave_buffer+wave_ptr, highchunk, sizeof(highchunk)); 
        wave_ptr+=sizeof(highchunk);
    }
}
void write0() {
    printf("0");
    if(wave_ptr == 0 || wave_buffer[wave_ptr-1] < 128)
        highChunk(2);
    else
        lowChunk(2);
}
void write1() {
    printf("1");
    if(wave_ptr==0 || wave_buffer[wave_ptr-1] < 128) {
        highChunk(1);
        lowChunk(1);
    }
    else {
        lowChunk(1);
        highChunk(1);
    }
}
void playBit(int writeBit){
    if (writeBit == 0) 
        write0();
    else 
        write1();
}
void playTrack(int track) {
  int tmp, crc, lrc = 0;
  track--; // index 0
  dir = 0;
  int i, j;
  // First put out a bunch of leading zeros.
  for (i = 0; i < 16; i++) 
      playBit(0);
  for (i = 0; tracks[track][i] != '\0'; i++){
    crc = 1;
    tmp = tracks[track][i] - sublen[track];
    for (j = 0; j < bitlen[track]-1; j++){
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      playBit(tmp & 1);
      tmp >>= 1;
    }
    //printf("-");
    playBit(crc);
    //printf("-");
  }
  tmp = lrc;
  crc = 1;
  for (j = 0; j < bitlen[track]-1; j++){
    crc ^= tmp & 1;
    playBit(tmp & 1);
    tmp >>= 1;
  }
  playBit(crc);
  for (i = 0; i < 16; i++)
    playBit(0);
}

int main()
{
    lowChunk(30);
    playTrack(1);     
    lowChunk(20);   
    struct riff_chunk myriff;
    struct fmt_chunk myfmt;   
    myriff.chunk_id[0]='R'; 
    myriff.chunk_id[1]='I'; 
    myriff.chunk_id[2]='F'; 
    myriff.chunk_id[3]='F';
    myriff.file_size=wave_ptr+4+4+sizeof(myriff)+sizeof(myfmt);
    myriff.riff_type[0]='W'; 
    myriff.riff_type[1]='A'; 
    myriff.riff_type[2]='V'; 
    myriff.riff_type[3]='E';
    myfmt.chunk_id[0]='f';  
    myfmt.chunk_id[1]='m';  
    myfmt.chunk_id[2]='t'; 
    myfmt.chunk_id[3]=0x20;
    myfmt.chunk_size=0x10;
    myfmt.compression_code=1;
    myfmt.num_channels=1;
    myfmt.sample_rate=8192;
    myfmt.bytes_second=8192;
    myfmt.block_align=1;
    myfmt.bits_sample=8;
    FILE *f=fopen("audiospoof.wav","wb");
    fwrite((void *)&myriff, 1, sizeof(myriff), f);
    fwrite((void *)&myfmt, 1, sizeof(myfmt), f);
    char data_chunk_id[]={ 'd', 'a', 't', 'a' }; fwrite(data_chunk_id, 1, sizeof(data_chunk_id), f);
    fwrite(&wave_ptr, 1, sizeof(wave_ptr), f);
    fwrite(wave_buffer, 1, wave_ptr, f);            
    fclose(f);
    printf("\n");
    return 0;
}
