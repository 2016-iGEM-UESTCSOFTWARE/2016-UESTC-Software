/****************************************************************************
 * ISNT2BIT:File decryption (descrambing) program using the ISAAC method developed 
 *         by B. Jenkins to convert to nucleotide sequences to binary files. 
 *         The same code can be used for both encryption and decryption purposes.
 * Usage: isnt2bit in_file out_file token
 *        "in_file" is the input file name which is to be encrypted or decrypted  
 *        "out_file" is the out file name which has been encrypted or decrypted
 *        "token" is the password to encrypt the in_file. At most 256 ASCII code
 *        will be used. Longer string will be truncated.
 * iGEM UESTC SOFTWARE 2016 team.      
 * Part of BIO101 (DNA STORAGE SYSTEM) Project.
 ****************************************************************************/

/*
------------------------------------------------------------------------------
ISAAC64.c. By Bob Jenkins, 1996.  Public Domain.
------------------------------------------------------------------------------
*/
#include <string.h>
#ifndef STANDARD
#include "standard.h"
#endif
#ifndef ISAAC64
#include "isaac64.h"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
extern    ub8 randrsl[RANDSIZ], randcnt;
static    ub8 mm[RANDSIZ];
static    ub8 aa=0, bb=0, cc=0;

#define ind(mm,x)  (*(ub8 *)((ub1 *)(mm) + ((x) & ((RANDSIZ-1)<<3))))
#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
  x = *m;  \
  a = (mix) + *(m2++); \
  *(m++) = y = ind(mm,x) + a + b; \
  *(r++) = b = ind(mm,y>>RANDSIZL) + x; \
}

void isaac64()
{
  register ub8 a,b,x,y,*m,*m2,*r,*mend;
  m=mm; r=randrsl;
  a = aa; b = bb + (++cc);
  for (m = mm, mend = m2 = m+(RANDSIZ/2); m<mend; )
  {
    rngstep(~(a^(a<<21)), a, b, mm, m, m2, r, x);
    rngstep(  a^(a>>5)  , a, b, mm, m, m2, r, x);
    rngstep(  a^(a<<12) , a, b, mm, m, m2, r, x);
    rngstep(  a^(a>>33) , a, b, mm, m, m2, r, x);
  }
  for (m2 = mm; m2<mend; )
  {
    rngstep(~(a^(a<<21)), a, b, mm, m, m2, r, x);
    rngstep(  a^(a>>5)  , a, b, mm, m, m2, r, x);
    rngstep(  a^(a<<12) , a, b, mm, m, m2, r, x);
    rngstep(  a^(a>>33) , a, b, mm, m, m2, r, x);
  }
  bb = b; aa = a;
  //added by xwang
  randcnt = 0;
}

#define mix(a,b,c,d,e,f,g,h) \
{ \
   a-=e; f^=h>>9;  h+=a; \
   b-=f; g^=a<<9;  a+=b; \
   c-=g; h^=b>>23; b+=c; \
   d-=h; a^=c<<15; c+=d; \
   e-=a; b^=d>>14; d+=e; \
   f-=b; c^=e<<20; e+=f; \
   g-=c; d^=f>>17; f+=g; \
   h-=d; e^=g<<14; g+=h; \
}

void randinit(flag)
word flag;
{
   word i;
   ub8 a,b,c,d,e,f,g,h;
   aa=bb=cc=(ub8)0;
   a=b=c=d=e=f=g=h=0x9e3779b97f4a7c13LL;  /* the golden ratio */

   for (i=0; i<4; ++i)                    /* scramble it */
   {
     mix(a,b,c,d,e,f,g,h);
   }

   for (i=0; i<RANDSIZ; i+=8)   /* fill in mm[] with messy stuff */
   {
     if (flag)                  /* use all the information in the seed */
     {
       a+=randrsl[i  ]; b+=randrsl[i+1]; c+=randrsl[i+2]; d+=randrsl[i+3];
       e+=randrsl[i+4]; f+=randrsl[i+5]; g+=randrsl[i+6]; h+=randrsl[i+7];
     }
     mix(a,b,c,d,e,f,g,h);
     mm[i  ]=a; mm[i+1]=b; mm[i+2]=c; mm[i+3]=d;
     mm[i+4]=e; mm[i+5]=f; mm[i+6]=g; mm[i+7]=h;
   }

   if (flag) 
   {        /* do a second pass to make all of the seed affect all of mm */
     for (i=0; i<RANDSIZ; i+=8)
     {
       a+=mm[i  ]; b+=mm[i+1]; c+=mm[i+2]; d+=mm[i+3];
       e+=mm[i+4]; f+=mm[i+5]; g+=mm[i+6]; h+=mm[i+7];
       mix(a,b,c,d,e,f,g,h);
       mm[i  ]=a; mm[i+1]=b; mm[i+2]=c; mm[i+3]=d;
       mm[i+4]=e; mm[i+5]=f; mm[i+6]=g; mm[i+7]=h;
     }
   }

   isaac64();          /* fill in the first set of results */
   //modified by xwang
   //randcnt=RANDSIZ;    /* prepare to use the first set of results */
   randcnt=0;
}

// Get a random 64-bit value 0..MAXINT
ub8 iRandom()
{
	ub8 r = randrsl[randcnt];
	++randcnt;
	if (randcnt >RANDSIZ) {
		isaac64();
		randcnt = 0;
	}
	return r;
}
 
// Seed ISAAC with a string
// seed longer than RANDSIZ will be truncated
void iSeed(char *seed, int flag)
{
	register ub8 i,m;
	for (i=0; i<RANDSIZ; i++) mm[i]=0;
	m = strlen(seed);
	for (i=0; i<RANDSIZ; i++)
	{
	  // in case seed has less than 256 elements
          if (i>m) randrsl[i]=0;  else randrsl[i] = seed[i];
	}
	// initialize ISAAC with seed
	randinit(flag);
}

//bit2nt: one byte of bits convert into 4 bytes of A (65), C(67), G(71), T(84) sequence
// 00 -> A, 01 -> C, 10 -> G, 11 -> T 
static unsigned char nt_code[] = {65, 67, 71, 84};//ASCII code

//nt2bit, reverse conversion
ub8 nt2bit(unsigned char *nts) {
  ub8 output = 0; 
  ub8 i, j;
  for (i = 0; i<32; i++) 
     for(j = 0; j<4; j++)
	if(nt_code[j] == nts[i]) output += (j<<(2*i));
  return output;
}

// 4 nts >> one byte
unsigned char nt2byte(unsigned char *nts) {
  unsigned char byte = 0; 
  int i, j;
  for (i = 0; i<4; i++) 
     for(j = 0; j<4; j++)
	if(nt_code[j] == nts[i]) byte += (j<< (2*i));
  return byte;
}
/* Step 1: Reading the input file in BLOCKS
 *      2. Translate bits into nt sequences
 *      3: XOR with iRandom, for each block intitialized with the token    
 *      4. Write each block to the output
 *
 *      BLOCK SIZE is chosen as 256 bits (4*ub8 = 32 bytes)
 *      corresponding to 128 nts. 
 *
 *      WARNING: the encryption function is compromised since the fixed 
 *      maskers are applied for all blocks. 
 */ 
int main(int argc, char* argv[])
{
  int i, j, k;
  //check arguments
  if (argc < 4) {
     printf("ERROR: Too few arguments.\n");
     printf("Usage: %s input_file outpout_file token\n", argv[0]);
     return 1;
  }
  //should check file existence
  char *token = argv[3];

  //initialize random seed 
  iSeed(token, TRUE);
  int BLOCKSIZE = 4; //unit: ub8
  int BLOCKBYTE = 32*BLOCKSIZE +1;
  ub8 mask[BLOCKSIZE]; 
  for (i=0; i<BLOCKSIZE; i++) mask[i] = iRandom();

  //read file in the byte mode
  FILE* fi = fopen(argv[1], "rb");
  FILE* fo = fopen(argv[2], "wb");
  int fr; //WARNING: file operation return value, untreated
  
  //check file size
  fseek(fi, 0L, SEEK_END);
  unsigned int size = ftell(fi); //unit: bytes
  int cycles = size / BLOCKBYTE, tail = size % BLOCKBYTE;
  fseek(fi, 0L, SEEK_SET);

  unsigned char in[BLOCKBYTE];// 8*4*4 + 1 = 128 bytes + 1 newline -> 4*ub8
  ub8 out[BLOCKSIZE]; // = {0};

  for (i=0; i<cycles; i++) {
    // block-read 8*BLOCKSIZE = 32 bytes to 'in' from 'fi'
    fr = fread(in, BLOCKBYTE, 1, fi);
    // encode
    for (j=0; j<BLOCKSIZE; j++)
	    out[j] = nt2bit(&in[j*32]) ^ mask[j]; 
    fr = fwrite(out, BLOCKSIZE, 8, fo);
  }
  if (tail > 0) {
    //handle the tail, ub8 -> 32 bytes of nts
    tail = tail  - 1;//newline
    memset(in, 0, sizeof(in));
    fr = fread(in, tail, 1, fi);
    cycles = tail/32;
    for (j=0; j<cycles; j++) {
          out[j] = nt2bit(&in[j*32]) ^ mask[j]; 
    }
    fr = fwrite(out, cycles, 8, fo);
    if (tail % 32 > 0) {//tail of the tail, small than ub8
      ub8 tmask = mask[cycles];
      unsigned char tout;
      if (tail % 4 >0) printf("Corrupted File! NT should be 4*n");
      for (j=0; j<(tail/4); j++){
        tout = (nt2byte(&in[32*cycles + 4*j]) ^ ((tmask >> (8*j)) & 0xFF) );
        fr = fwrite(&tout, 1, 1, fo);
      }
    }
  }
  fclose(fi);
  fclose(fo);
  return 0;
}
