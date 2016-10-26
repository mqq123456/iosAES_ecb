#ifndef _AES_H_
#define _AES_H_

class AES
        {
public:
        AES(void);

        enum BlockMode {
                ECB = 0, 
                CBC = 1  
                };

        void SetParameters(int keylength, int blocklength = 128);

        void StartEncryption(const unsigned char * key);
        void EncryptBlock(const unsigned char * datain, unsigned char * dataout);
        void Encrypt(const unsigned char * datain, unsigned char * dataout, unsigned long numBlocks, BlockMode mode = CBC);

        void StartDecryption(const unsigned char * key);
        void DecryptBlock(const unsigned char * datain, unsigned char * dataout);
        void Decrypt(const unsigned char * datain, unsigned char * dataout, unsigned long numBlocks, BlockMode mode = CBC);

private:

        int Nb,Nk;    // block and key length / 32, should be 4,6,or 8
        int Nr;       // number of rounds

        unsigned char W[4*8*15];   // the expanded key

        void KeyExpansion(const unsigned char * key);

        }; 

// end - AES.h

#endif