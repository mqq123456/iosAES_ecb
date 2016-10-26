//
//  MyAes.m
//  ChatRoom
//
//  Created by lee han on 12/11/12.
//  Copyright (c) 2012 lee han. All rights reserved.
//

#import "MyAes.h"
#import "AES.h"
#import "Base64.h"

NSString *AESEncryptWithKey(NSString* nsSrc, NSString* key)
{
	AES aes;
	aes.SetParameters(256, 128);
	aes.StartEncryption((const unsigned char*)[key UTF8String]);
    
	int nSrcLength = [nsSrc length];
	char *pOriSrc = (char*)[nsSrc UTF8String];
	nSrcLength = strlen(pOriSrc)+1;
    
	int nBlockNum = (nSrcLength+15)/16;
    
	int nMemorySize = 16*nBlockNum;
    
	unsigned char* pSrc = new unsigned char[nMemorySize];
	unsigned char* pDest = new unsigned char[nMemorySize];
	memset(pSrc, 0, nMemorySize);
	memset(pDest, 0, nMemorySize);
	//memcpy(pSrc, [nsSrc UTF8String], [nsSrc length]);
	memcpy(pSrc, [nsSrc UTF8String], nSrcLength);
    
	aes.Encrypt(pSrc, pDest, nBlockNum, AES::ECB);
	//aes.Encrypt((unsigned char*)[nsSrc UTF8String], pDest, nBlockNum, AES::ECB);
    
	//int nOutSizeBase64 = 16*(nSrcLength/16 + 1);
	int nOutSizeBase64 = Base64::calculateBase64Len(nMemorySize);
	char* pBase64Dest = new char[nOutSizeBase64];
	memset(pBase64Dest, 0, nOutSizeBase64);
	
	Base64::binToBase64((unsigned char*)pDest, pBase64Dest, nMemorySize);
    
	NSString *result = [NSString stringWithUTF8String:pBase64Dest];
    
	delete [] pBase64Dest;
	delete [] pDest;
	delete [] pSrc;
    
	return result;
}

NSString *AESDecryptWithKey(NSString* nsSrc, NSString* key)
{
	AES aes;
	aes.SetParameters(256, 128);
	aes.StartDecryption((const unsigned char*)[key UTF8String]);
	
	int nSrcLength = [nsSrc length];
	unsigned char *pBinDest = new unsigned char[nSrcLength+1];
	memset(pBinDest, 0, nSrcLength+1);
	
	int nBinDestLength = Base64::base64ToBin([nsSrc UTF8String], pBinDest, nSrcLength);
	unsigned char *pDataOut = new unsigned char[nBinDestLength+1];
	memset(pDataOut, 0, nBinDestLength+1);
	
	aes.Decrypt(pBinDest, pDataOut, nBinDestLength/16, AES::ECB);
	
	NSString *result = [NSString stringWithUTF8String:(const char*)pDataOut];
	
	delete [] pBinDest;
	delete [] pDataOut;
	
	return result;
}
