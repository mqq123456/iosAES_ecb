#include "AES.h"
#include <assert.h>
#include <stdio.h>
#include <fstream>
#include <iostream>

inline unsigned long my_rotl(unsigned long value, int shift)
{
        const int BITS_COUNT = sizeof(value) * 8;
        int shiftLeft = shift % BITS_COUNT;
        int shiftRight = BITS_COUNT - shiftLeft;
        return ((value << shiftLeft) | (value >> shiftRight));
}

inline unsigned long my_rotr(unsigned long value, int shift)
{
        const int BITS_COUNT = sizeof(value) * 8;
        int shiftRight = shift % BITS_COUNT;
        int shiftLeft = BITS_COUNT - shiftRight;
        return ((value << shiftLeft) | (value >> shiftRight));
}

using namespace std;

namespace { 

unsigned char gf2_8_inv[256];
unsigned char byte_sub[256];
unsigned char inv_byte_sub[256];


unsigned long Rcon[60];

unsigned long T0[256];
unsigned long T1[256];
unsigned long T2[256];
unsigned long T3[256];

unsigned long I0[256];
unsigned long I1[256];
unsigned long I2[256];
unsigned long I3[256];

unsigned long T4[256];
unsigned long T5[256];
unsigned long T6[256];
unsigned long T7[256];
unsigned long I4[256];
unsigned long I5[256];
unsigned long I6[256];
unsigned long I7[256];

bool tablesInitialized = false;

#define xmult(a) ((a)<<1) ^ (((a)&128) ? 0x01B : 0)

#define VEC4(a,b,c,d) (((unsigned long)(a)) | (((unsigned long)(b))<<8) | (((unsigned long)(c))<<16) | (((unsigned long)(d))<<24))

#define GetByte(a,n) ((unsigned char)((a) >> (n<<3)))

#define RotByte(a) my_rotr(a,8)

#define RotByteL(a) my_rotl(a,8)

inline unsigned char GF2_8_mult(unsigned char a, unsigned char b)
        { 
        unsigned char result = 0;

        int count = 8;
        while (count--)
                {
                if (b&1)
                        result ^= a;
                if (a&128)
                        {
                        a <<= 1;
                        a ^= (0x1B);
                        }
                else
                        a <<= 1;
                b >>= 1;
                }
        return result;
        } // GF2_8_mult

bool CheckLargeTables(bool create)
        {
        unsigned int i;
        unsigned char a1,a2,a3,b1,b2,b3,b4,b5;
        for (i = 0; i < 256; i++)
                {
                a1 = byte_sub[i];
                a2 = xmult(a1);
                a3 = a2^a1;

                b5 = inv_byte_sub[i];
                b1 = GF2_8_mult(0x0E,b5);
                b2 = GF2_8_mult(0x09,b5);
                b3 = GF2_8_mult(0x0D,b5);
                b4 = GF2_8_mult(0x0B,b5);

                if (create == true)
                        {
                        T0[i] = VEC4(a2,a1,a1,a3);
                        T1[i] = RotByteL(T0[i]);
                        T2[i] = RotByteL(T1[i]);
                        T3[i] = RotByteL(T2[i]);

                        T4[i] = VEC4(a1,0,0,0); // identity
                        T5[i] = RotByteL(T4[i]);
                        T6[i] = RotByteL(T5[i]);
                        T7[i] = RotByteL(T6[i]);

                        I0[i] = VEC4(b1,b2,b3,b4);
                        I1[i] = RotByteL(I0[i]);
                        I2[i] = RotByteL(I1[i]);
                        I3[i] = RotByteL(I2[i]);

                        I4[i] = VEC4(b5,0,0,0); // identity
                        I5[i] = RotByteL(I4[i]);
                        I6[i] = RotByteL(I5[i]);
                        I7[i] = RotByteL(I6[i]);
                        }
                else
                        {
                        if (T0[i] != VEC4(a2,a1,a1,a3))
                                return false;
                        if (T1[i] != RotByteL(T0[i]))
                                return false;
                        if (T2[i] != RotByteL(T1[i]))
                                return false;
                        if (T3[i] != RotByteL(T2[i]))
                                return false;
                        if (T4[i] != VEC4(a1,0,0,0))
                                return false;
                        if (T5[i] != RotByteL(T4[i]))
                                return false;
                        if (T6[i] != RotByteL(T5[i]))
                                return false;
                        if (T7[i] != RotByteL(T6[i]))
                                return false;
                        if (I0[i] != VEC4(b1,b2,b3,b4))
                                return false;
                        if (I1[i] != RotByte(I0[i]))
                                return false;
                        if (I2[i] != RotByte(I1[i]))
                                return false;
                        if (I3[i] != RotByte(I2[i]))
                                return false;
                        if (I4[i] != VEC4(b5,0,0,0))
                                return false;
                        if (I5[i] != RotByteL(I4[i]))
                                return false;
                        if (I6[i] != RotByteL(I5[i]))
                                return false;
                        if (I7[i] != RotByteL(I6[i]))
                                return false;
                        }
                }
        return true;
        } 

bool CheckInverses(bool create)
        {      
#ifdef _DEBUG
        assert(GF2_8_mult(0x57,0x13) == 0xFE); // test these first
        assert(GF2_8_mult(0x01,0x01) == 0x01);
        assert(GF2_8_mult(0xFF,0x55) == 0xF8);
#endif

        unsigned int a,b; // need int here to prevent wraps in loop
        if (create == true)
                gf2_8_inv[0] = 0;
        else if (gf2_8_inv[0] != 0)
                return false;
        for (a = 1; a <= 255; a++)
                {
                b = 1;
                while (GF2_8_mult(a,b) != 1)
                        b++;

                if (create == true)
                        gf2_8_inv[a] = b;
                else if (gf2_8_inv[a] != b)
                        return false;
                }
        return true;
        } // CheckInverses

unsigned char BitSum(unsigned char byte)
        { // return the sum of bits mod 2
        byte = (byte>>4)^(byte&15);
        byte = (byte>>2)^(byte&3);
        return (byte>>1)^(byte&1);
        } // BitSum

bool CheckByteSub(bool create)
        {
        if (CheckInverses(create) == false)
                return false; // we cannot do this without inverses

        unsigned int x,y; // need ints here to prevent wrap in loop
        for (x = 0; x <= 255; x++)
                {
                y = gf2_8_inv[x]; // inverse to start with

                // affine transform
                y = BitSum(y&0xF1) | (BitSum(y&0xE3)<<1) | (BitSum(y&0xC7)<<2) | (BitSum(y&0x8F)<<3) |
                        (BitSum(y&0x1F)<<4) | (BitSum(y&0x3E)<<5) | (BitSum(y&0x7C)<<6) | (BitSum(y&0xF8)<<7);
                y = y ^ 0x63;
                if (create == true)
                        byte_sub[x] = y;
                else if (byte_sub[x] != y)
                        return false;
                }
        return true;
        } // CheckByteSub

bool CheckInvByteSub(bool create)
        {
        if (CheckInverses(create) == false)
                return false; // we cannot do this without inverses
        if (CheckByteSub(create) == false)
                return false; // we cannot do this without byte_sub

        unsigned int x,y; // need ints here to prevent wrap in loop
        for (x = 0; x <= 255; x++)
                {
                // we brute force it...
                y = 0;
                while (byte_sub[y] != x)
                        y++;
                if (create == true)
                        inv_byte_sub[x] = y;
                else if (inv_byte_sub[x] != y)
                        return false;
                }
        return true;
        } // CheckInvByteSub

bool CheckRcon(bool create)
        {
        unsigned char Ri = 1; // start here

        if (create == true)
                Rcon[0] = 0;
        else if (Rcon[0] != 0)
                return false; // todo - this is unused still check?
        for (unsigned int i = 1; i < sizeof(Rcon)/sizeof(Rcon[0])-1; i++)
                {
                if (create == true)
                        Rcon[i] = Ri;
                else if (Rcon[i] != Ri)
                        return false;
                Ri = GF2_8_mult(Ri,0x02); // multiply by x - todo replace with xmult
                }
        return true;
        } // CheckRCon

// key adding for 4,6,8 column cases
#define AddRoundKey4(dest,src)  \
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;

#define AddRoundKey6(dest,src)  \
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;

#define AddRoundKey8(dest,src)  \
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;\
        *dest++ = *r_ptr++ ^ *src++;

// this define computes one of the round vectors
#define compute_one(dest,src2,j,C1,C2,C3,Nb)    *(dest+j) = \
        T0[GetByte(src2[j],0)]^T1[GetByte(src2[((j+C1+Nb)%Nb)],1)]^ \
        T2[GetByte(src2[((j+C2+Nb)%Nb)],2)]^T3[GetByte(src2[((j+C3+Nb)%Nb)],3)] \
        ^*r_ptr++

// single table version, slower
#define compute_one_small(dest,src2,j,C1,C2,C3,Nb)      *(dest+j) = *r_ptr++^\
        T0[GetByte(src2[j],0)]^\
        RotByteL(T0[GetByte(src2[((j+C1+Nb)%Nb)],1)]^\
        RotByteL(T0[GetByte(src2[((j+C2+Nb)%Nb)],2)]^\
        RotByteL(T0[GetByte(src2[((j+C3+Nb)%Nb)],3)])))

#define Round4(d,s)     \
                compute_one(d,s,0,1,2,3,4); \
                compute_one(d,s,1,1,2,3,4);     \
                compute_one(d,s,2,1,2,3,4);     \
                compute_one(d,s,3,1,2,3,4);

#define Round6(d,s) \
                compute_one(d,s,0,1,2,3,6); \
                compute_one(d,s,1,1,2,3,6); \
                compute_one(d,s,2,1,2,3,6); \
                compute_one(d,s,3,1,2,3,6); \
                compute_one(d,s,4,1,2,3,6); \
                compute_one(d,s,5,1,2,3,6);

#define Round8(d,s) \
                compute_one(d,s,0,1,3,4,8); \
                compute_one(d,s,1,1,3,4,8); \
                compute_one(d,s,2,1,3,4,8); \
                compute_one(d,s,3,1,3,4,8); \
                compute_one(d,s,4,1,3,4,8); \
                compute_one(d,s,5,1,3,4,8); \
                compute_one(d,s,6,1,3,4,8); \
                compute_one(d,s,7,1,3,4,8);

#define compute_one_inv(dest,src2,j,C1,C2,C3,Nb)        *(dest+j) = \
        I0[GetByte(src2[j],0)]^I1[GetByte(src2[((j-C1+Nb)%Nb)],1)]^ \
        I2[GetByte(src2[((j-C2+Nb)%Nb)],2)]^I3[GetByte(src2[((j-C3+Nb)%Nb)],3)] \
        ^*r_ptr++

#define InvRound4(d,s)  \
                compute_one_inv(d,s,0,1,2,3,4); \
                compute_one_inv(d,s,1,1,2,3,4); \
                compute_one_inv(d,s,2,1,2,3,4); \
                compute_one_inv(d,s,3,1,2,3,4);

#define InvRound6(d,s)  \
                compute_one_inv(d,s,0,1,2,3,6); \
                compute_one_inv(d,s,1,1,2,3,6); \
                compute_one_inv(d,s,2,1,2,3,6); \
                compute_one_inv(d,s,3,1,2,3,6); \
                compute_one_inv(d,s,4,1,2,3,6); \
                compute_one_inv(d,s,5,1,2,3,6);

#define InvRound8(d,s)  \
                compute_one_inv(d,s,0,1,3,4,8); \
                compute_one_inv(d,s,1,1,3,4,8); \
                compute_one_inv(d,s,2,1,3,4,8); \
                compute_one_inv(d,s,3,1,3,4,8); \
                compute_one_inv(d,s,4,1,3,4,8); \
                compute_one_inv(d,s,5,1,3,4,8); \
                compute_one_inv(d,s,6,1,3,4,8); \
                compute_one_inv(d,s,7,1,3,4,8);

// this define computes one of the final round vectors
#define compute_one_final1(dest,src,j,C1,C2,C3,Nb)  *dest++ = \
        (T3[GetByte(src[j],0)]&0xFF)^\
        (T3[GetByte(src[((j+C1+Nb)%Nb)],1)]&0xFF00)^ \
        (T1[GetByte(src[((j+C2+Nb)%Nb)],2)]&0xFF0000)^ \
        (T1[GetByte(src[((j+C3+Nb)%Nb)],3)]&0xFF000000)^*r_ptr++

// for another 4K tables, we save 3 clock cycles - sick
#define compute_one_final(dest,src,j,C1,C2,C3,Nb)  *dest++ = \
        (T4[GetByte(src[j],0)])^\
        (T5[GetByte(src[((j+C1+Nb)%Nb)],1)])^ \
        (T6[GetByte(src[((j+C2+Nb)%Nb)],2)])^ \
        (T7[GetByte(src[((j+C3+Nb)%Nb)],3)])^*r_ptr++

// final round defines - this one is for case for 4 columns
#define FinalRound4(d,s) compute_one_final(d,s,0,1,2,3,4); \
                                                compute_one_final(d,s,1,1,2,3,4); \
                                                compute_one_final(d,s,2,1,2,3,4); \
                                                compute_one_final(d,s,3,1,2,3,4);

#define FinalRound6(d,s) compute_one_final(d,s,0,1,2,3,6); \
                                                compute_one_final(d,s,1,1,2,3,6); \
                                                compute_one_final(d,s,2,1,2,3,6); \
                                                compute_one_final(d,s,3,1,2,3,6); \
                                                compute_one_final(d,s,4,1,2,3,6); \
                                                compute_one_final(d,s,5,1,2,3,6);

#define FinalRound8(d,s) compute_one_final(d,s,0,1,3,4,8); \
                                                compute_one_final(d,s,1,1,3,4,8); \
                                                compute_one_final(d,s,2,1,3,4,8); \
                                                compute_one_final(d,s,3,1,3,4,8); \
                                                compute_one_final(d,s,4,1,3,4,8); \
                                                compute_one_final(d,s,5,1,3,4,8); \
                                                compute_one_final(d,s,6,1,3,4,8); \
                                                compute_one_final(d,s,7,1,3,4,8);

// inverse cipher stuff
#define compute_one_final_inv(dest,src,j,C1,C2,C3,Nb)  *dest++ = \
        (I4[GetByte(src[j],0)])^\
        (I5[GetByte(src[((j-C1+Nb)%Nb)],1)])^ \
        (I6[GetByte(src[((j-C2+Nb)%Nb)],2)])^ \
        (I7[GetByte(src[((j-C3+Nb)%Nb)],3)])^*r_ptr++

// final round defines - this one is for case for 4 columns
#define InvFinalRound4(d,s) compute_one_final_inv(d,s,0,1,2,3,4); \
                                                compute_one_final_inv(d,s,1,1,2,3,4); \
                                                compute_one_final_inv(d,s,2,1,2,3,4); \
                                                compute_one_final_inv(d,s,3,1,2,3,4);

#define InvFinalRound6(d,s) compute_one_final_inv(d,s,0,1,2,3,6); \
                                                compute_one_final_inv(d,s,1,1,2,3,6); \
                                                compute_one_final_inv(d,s,2,1,2,3,6); \
                                                compute_one_final_inv(d,s,3,1,2,3,6); \
                                                compute_one_final_inv(d,s,4,1,2,3,6); \
                                                compute_one_final_inv(d,s,5,1,2,3,6);

#define InvFinalRound8(d,s) compute_one_final_inv(d,s,0,1,3,4,8); \
                                                compute_one_final_inv(d,s,1,1,3,4,8); \
                                                compute_one_final_inv(d,s,2,1,3,4,8); \
                                                compute_one_final_inv(d,s,3,1,3,4,8); \
                                                compute_one_final_inv(d,s,4,1,3,4,8); \
                                                compute_one_final_inv(d,s,5,1,3,4,8); \
                                                compute_one_final_inv(d,s,6,1,3,4,8); \
                                                compute_one_final_inv(d,s,7,1,3,4,8);

unsigned long SubByte(unsigned long data)
        { // does the SBox on this 4 byte data
        unsigned result = 0;
        result = byte_sub[data>>24];
        result <<= 8;
        result |= byte_sub[(data>>16)&255];
        result <<= 8;
        result |= byte_sub[(data>>8)&255];
        result <<= 8;
        result |= byte_sub[data&255];
        return result;
        } // SubByte


void DumpCharTable(ostream & out, const char * name, const unsigned char * table, int length)
        { // dump the contents of a table to a file
        int pos;
        out << name << endl << hex;
        for (pos = 0; pos < length; pos++)
                {
                out << "0x";
                if (table[pos] < 16)
                        out << '0';
                out << static_cast<unsigned int>(table[pos]) << ',';
                if ((pos %16) == 15)
                        out << endl;
                }
        out << dec;
        } // DumpCharTable

void DumpLongTable(ostream & out, const char * name, const unsigned long * table, int length)
        { // dump te contents of a table to a file
        int pos;
        out << name << endl << hex;
        for (pos = 0; pos < length; pos++)
                {
                out << "0x";
                if (table[pos] < 16)
                        out << '0';
                if (table[pos] < 16*16)
                        out << '0';
                if (table[pos] < 16*16*16)
                        out << '0';
                if (table[pos] < 16*16*16*16)
                        out << '0';
                if (table[pos] < 16*16*16*16*16)
                        out << '0';
                if (table[pos] < 16*16*16*16*16*16)
                        out << '0';
                if (table[pos] < 16*16*16*16*16*16*16)
                        out << '0';
                out << static_cast<unsigned int>(table[pos]) << ',';
                if ((pos % 8) == 7)
                        out << endl;
                }
        out << dec;
        } // DumpCharTable

// return true iff tables are valid. create = true fills them in if not
bool CreateAESTables(bool create, bool create_file)
        {
        bool retval = true;
        if (CheckInverses(create) == false)
                retval = false;
        if (CheckByteSub(create) == false)
                retval = false;
        if (CheckInvByteSub(create) == false)
                retval = false;
        if (CheckRcon(create) == false)
                return false;
        if (CheckLargeTables(create) == false)
                return false;

        if (create_file == true)
                { // dump tables
                ofstream out;
                out.open("Tables.dat");
                if (out.is_open() == true)
                        {
                        DumpCharTable(out,"gf2_8_inv", gf2_8_inv, 256);
                        out << "\n\n";
                        DumpCharTable(out,"byte_sub", byte_sub, 256);
                        out << "\n\n";
                        DumpCharTable(out,"inv_byte_sub", inv_byte_sub, 256);
                        out << "\n\n";
                        DumpLongTable(out,"RCon", Rcon, 60);
                        out << "\n\n";
                        DumpLongTable(out,"T0", T0, 256);
                        out << "\n\n";
                        DumpLongTable(out,"T1", T1, 256);
                        out << "\n\n";
                        DumpLongTable(out,"T2", T2, 256);
                        out << "\n\n";
                        DumpLongTable(out,"T3", T3, 256);
                        out << "\n\n";
                        DumpLongTable(out,"T4", T4, 256);
                        out << "\n\n";
                        DumpLongTable(out,"I0", I0, 256);
                        out << "\n\n";
                        DumpLongTable(out,"I1", I1, 256);
                        out << "\n\n";
                        DumpLongTable(out,"I2", I2, 256);
                        out << "\n\n";
                        DumpLongTable(out,"I3", I3, 256);
                        out << "\n\n";
                        DumpLongTable(out,"I4", I4, 256);
                        out.close();
                        }
                }
        return retval;
        } // CreateAESTables

void DumpHex(const unsigned char * table, int length)
        { // dump some hex values for debugging
        int pos;
        cerr << hex;
        for (pos = 0; pos < length; pos++)
                {
                if (table[pos] < 16)
                        cerr << '0';
                cerr << static_cast<unsigned int>(table[pos]) << ' ';
                if ((pos %16) == 15)
                        cerr << endl;
                }
        cerr << dec;
        } // DumpHex


}// end of anonymous namespace

// Key expansion code - makes local copy
void AES::KeyExpansion(const unsigned char * key)
        {
#ifdef _DEBUG
        assert(Nk > 0);
#endif
        int i;
        unsigned long temp, * Wb = reinterpret_cast<unsigned long*>(W); // todo not portable - Endian problems
        if (Nk <= 6)
                {
                // todo - memcpy
                for (i = 0; i < 4*Nk; i++)
                        W[i] = key[i];
                for (i = Nk; i < Nb*(Nr+1); i++)
                        {
                        temp = Wb[i-1];
                        if ((i%Nk) == 0)
                                temp = SubByte(RotByte(temp)) ^ Rcon[i/Nk];
                        Wb[i] = Wb[i - Nk]^temp;
                        }
                }
        else
                {
                // todo - memcpy
                for (i = 0; i < 4*Nk; i++)
                        W[i] = key[i];
                for (i = Nk; i < Nb*(Nr+1); i++)
                        {
                        temp = Wb[i-1];
                        if ((i%Nk) == 0)
                                temp = SubByte(RotByte(temp)) ^ Rcon[i/Nk];
                        else if ((i%Nk) == 4)
                                temp = SubByte(temp);
                        Wb[i] = Wb[i - Nk]^temp;
                        }
                }
        } // KeyExpansion

void AES::SetParameters(int keylength, int blocklength)
        {
        Nk = Nr = Nb = 0; // default values

        if ((keylength != 128) && (keylength != 192) && (keylength != 256))
                return; // nothing - todo - throw error?
        if ((blocklength != 128) && (blocklength != 192) && (blocklength != 256))
                return; // nothing - todo - throw error?

        static int const parameters[] = {
//Nk*32 128     192     256
                10,     12,     14,  // Nb*32 = 128
                12,     12,     14,  // Nb*32 = 192
                14,     14,     14,  // Nb*32 = 256
                };

        // legal parameters, so fire it up
        Nk = keylength  /32;
        Nb = blocklength/32;
        Nr = parameters[((Nk-4)/2 + 3*(Nb-4)/2)];
        } // SetParameters

void AES::StartEncryption(const unsigned char * key)
        {
        KeyExpansion(key);
        } // StartEncryption

void AES::EncryptBlock(const unsigned char * datain1, unsigned char * dataout1)
        { // todo ? allow in place encryption
          // todo - clean up - lots of repeated macros
          // we only encrypt one block from now on

        unsigned long state[8*2]; // 2 buffers
        unsigned long * r_ptr = reinterpret_cast<unsigned long*>(W);
        unsigned long * dest  = state;
        unsigned long * src   = state;
        const unsigned long * datain = reinterpret_cast<const unsigned long*>(datain1);
        unsigned long * dataout = reinterpret_cast<unsigned long*>(dataout1);

        if (Nb == 4)
                {
                AddRoundKey4(dest,datain);

                if (Nr == 14)
                        {
                        Round4(dest,src);
                        Round4(src,dest);
                        Round4(dest,src);
                        Round4(src,dest);
                        }
                else if (Nr == 12)
                        {
                        Round4(dest,src);
                        Round4(src,dest);
                        }

                Round4(dest,src);
                Round4(src,dest);
                Round4(dest,src);
                Round4(src,dest);
                Round4(dest,src);
                Round4(src,dest);
                Round4(dest,src);
                Round4(src,dest);
                Round4(dest,src);

                FinalRound4(dataout,dest);
                }
        else if (Nb == 6)
                {
                AddRoundKey6(dest,datain);

                if (Nr == 14)
                        {
                        Round6(dest,src);
                        Round6(src,dest);
                        }

                Round6(dest,src);
                Round6(src,dest);
                Round6(dest,src);
                Round6(src,dest);
                Round6(dest,src);
                Round6(src,dest);
                Round6(dest,src);
                Round6(src,dest);
                Round6(dest,src);
                Round6(src,dest);
                Round6(dest,src);

                FinalRound6(dataout,dest);
                }
        else // Nb == 8
                {
                AddRoundKey8(dest,datain);

                Round8(dest,src);
                Round8(src,dest);
                Round8(dest,src);
                Round8(src,dest);
                Round8(dest,src);
                Round8(src,dest);
                Round8(dest,src);
                Round8(src,dest);
                Round8(dest,src);
                Round8(src,dest);
                Round8(dest,src);
                Round8(src,dest);
                Round8(dest,src);

                FinalRound8(dataout,dest);
                } // end switch on Nb

        } // Encrypt

// call this to encrypt any size block
void AES::Encrypt(const unsigned char * datain, unsigned char * dataout, unsigned long numBlocks, BlockMode mode)
        {
        if (0 == numBlocks)
                return;
        unsigned int blocksize = Nb*4;
        switch (mode)
                {
                case ECB :
                        while (numBlocks)
                                {
                                EncryptBlock(datain,dataout);
                                datain   += blocksize;
                                dataout  += blocksize;
                                --numBlocks;
                                }
                        break;
                case CBC :
                        {
                        unsigned char buffer[64];
                        memset(buffer,0,sizeof(buffer)); // clear out - todo - allow setting the Initialization Vector - needed for security
                        while (numBlocks)
                                {
                                for (unsigned int pos = 0; pos < blocksize; ++pos)
                                        buffer[pos] ^= *datain++;
                                EncryptBlock(buffer,dataout);
                                memcpy(buffer,dataout,blocksize);
                                dataout  += blocksize;
                                --numBlocks;
                                }
                        }
                        break;
                default :
#ifdef _DEBUG
                        assert(!"Unknown mode!");
#endif
                        break;
                }
        } // Encrypt

void AES::StartDecryption(const unsigned char * key)
        {
        KeyExpansion(key);

        unsigned char a0,a1,a2,a3,b0,b1,b2,b3, * W_ptr = W;

        for (int col = Nb; col < (Nr)*Nb; col++) // do all but first and last round
                {
                a0 = W_ptr[4*col+0];
                a1 = W_ptr[4*col+1];
                a2 = W_ptr[4*col+2];
                a3 = W_ptr[4*col+3];

                b0 = GF2_8_mult(0x0E,a0)^GF2_8_mult(0x0B,a1)^
                     GF2_8_mult(0x0D,a2)^GF2_8_mult(0x09,a3);
                b1 = GF2_8_mult(0x09,a0)^GF2_8_mult(0x0E,a1)^
                     GF2_8_mult(0x0B,a2)^GF2_8_mult(0x0D,a3);
                b2 = GF2_8_mult(0x0D,a0)^GF2_8_mult(0x09,a1)^
                     GF2_8_mult(0x0E,a2)^GF2_8_mult(0x0B,a3);
                b3 = GF2_8_mult(0x0B,a0)^GF2_8_mult(0x0D,a1)^
                     GF2_8_mult(0x09,a2)^GF2_8_mult(0x0E,a3);

                W_ptr[4*col+0] = b0;
                W_ptr[4*col+1] = b1;
                W_ptr[4*col+2] = b2;
                W_ptr[4*col+3] = b3;
                }

        // we reverse the rounds to make decryption faster
        unsigned long * WL = reinterpret_cast<unsigned long*>(W);
        for (int pos = 0; pos < Nr/2; pos++)
                for (int col = 0; col < Nb; col++)
                        swap(WL[col+pos*Nb],WL[col+(Nr-pos)*Nb]);
        } // StartDecryption

void AES::DecryptBlock(const unsigned char * datain1, unsigned char * dataout1)
        {
        unsigned long state[8*2]; // 2 buffers
        unsigned long * r_ptr = reinterpret_cast<unsigned long*>(W);
        unsigned long * dest  = state;
        unsigned long * src   = state;

        const unsigned long * datain = reinterpret_cast<const unsigned long*>(datain1);
        unsigned long * dataout = reinterpret_cast<unsigned long*>(dataout1);

        if (Nb == 4)
                {
                AddRoundKey4(dest,datain);

                if (Nr == 14)
                        {
                        InvRound4(dest,src);
                        InvRound4(src,dest);
                        InvRound4(dest,src);
                        InvRound4(src,dest);
                        }
                else if (Nr == 12)
                        {
                        InvRound4(dest,src);
                        InvRound4(src,dest);
                        }

                InvRound4(dest,src);
                InvRound4(src,dest);
                InvRound4(dest,src);
                InvRound4(src,dest);
                InvRound4(dest,src);
                InvRound4(src,dest);
                InvRound4(dest,src);
                InvRound4(src,dest);
                InvRound4(dest,src);

                InvFinalRound4(dataout,dest);
                }
        else if (Nb == 6)
                {
                AddRoundKey6(dest,datain);

                if (Nr == 14)
                        {
                        InvRound6(dest,src);
                        InvRound6(src,dest);
                        }

                InvRound6(dest,src);
                InvRound6(src,dest);
                InvRound6(dest,src);
                InvRound6(src,dest);
                InvRound6(dest,src);
                InvRound6(src,dest);
                InvRound6(dest,src);
                InvRound6(src,dest);
                InvRound6(dest,src);
                InvRound6(src,dest);
                InvRound6(dest,src);

                InvFinalRound6(dataout,dest);
                }
        else // Nb == 8
                {
                AddRoundKey8(dest,datain);

                InvRound8(dest,src);
                InvRound8(src,dest);
                InvRound8(dest,src);
                InvRound8(src,dest);
                InvRound8(dest,src);
                InvRound8(src,dest);
                InvRound8(dest,src);
                InvRound8(src,dest);
                InvRound8(dest,src);
                InvRound8(src,dest);
                InvRound8(dest,src);
                InvRound8(src,dest);
                InvRound8(dest,src);

                InvFinalRound8(dataout,dest);
                } // end switch on Nb
        } // Decrypt

// call this to decrypt any size block
void AES::Decrypt(const unsigned char * datain, unsigned char * dataout, unsigned long numBlocks, BlockMode mode)
        {
        if (0 == numBlocks)
                return;
        unsigned int blocksize = Nb*4;
        switch (mode)
                {
                case ECB :
                        while (numBlocks)
                                {
                                DecryptBlock(datain,dataout);
                                datain   += blocksize;
                                dataout  += blocksize;
                                --numBlocks;
                                }
                        break;
                case CBC :
                        {
                        unsigned char buffer[64];
                        memset(buffer,0,sizeof(buffer)); // clear out - todo - allow setting the Initialization Vector - needed for security
                        DecryptBlock(datain,dataout); // do first block
                        for (unsigned int pos = 0; pos < blocksize; ++pos)
                                *dataout++ ^= buffer[pos];
                        datain += blocksize;
                        numBlocks--;

                        while (numBlocks)
                                {
                                DecryptBlock(datain,dataout); // do first block
                                for (unsigned int pos = 0; pos < blocksize; ++pos)
                                        *dataout++ ^= *(datain-blocksize+pos);
                                datain  += blocksize;
                                --numBlocks;
                                }
                        }
                        break;
                default:
#ifdef _DEBUG
                        assert(!"Unknown mode!");
#endif
                        break;
                }
        } // Decrypt

// the constructor - makes sure local things are initialized
AES::AES(void)
        {
        if (false == tablesInitialized)
                tablesInitialized = CreateAESTables(true,false);
        if (false == tablesInitialized)
                throw "Tables failed to initialize";
        }

// end - AES.cpp
