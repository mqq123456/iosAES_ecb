//
//  MyBase64.m
//  ChatRoom
//
//  Created by lee han on 12/10/12.
//  Copyright (c) 2012 lee han. All rights reserved.
//

#import "MyBase64.h"
#import "Base64.h"

NSString *Base64Encode(NSString *nsSrc)
{
	NSUInteger nSrcLength = [nsSrc length];
	int nOutSizeBase64 = Base64::calculateBase64Len(nSrcLength);
    
	char* pDataOut = new char[nOutSizeBase64];
	memset(pDataOut, 0, nOutSizeBase64);
	
	Base64::binToBase64((unsigned char*)[nsSrc UTF8String], pDataOut, nSrcLength);
    
	NSString *result = [NSString stringWithUTF8String:pDataOut];
    
	delete [] pDataOut;
    
	return result;
}

NSString *Base64Decode(NSString *nsSrc)
{
    if (nsSrc == nil)
    {
        return nil;
    }
	NSUInteger nSrcLength = [nsSrc length];
	int nOutBinLength = Base64::calculateBinLen(nSrcLength);
    
	unsigned char *pDataOut = new unsigned char[nOutBinLength+1];
	memset(pDataOut, 0, nOutBinLength+1);
    
	Base64::base64ToBin([nsSrc UTF8String], pDataOut, nSrcLength);
	
	NSString *result = [NSString stringWithUTF8String:(char*)pDataOut];
    
	delete [] pDataOut;
    
	return result;
}