//
//  MyAes.h
//  ChatRoom
//
//  Created by lee han on 12/11/12.
//  Copyright (c) 2012 lee han. All rights reserved.
//
#pragma once

#import <Foundation/Foundation.h>

NSString *AESEncryptWithKey(NSString* nsSrc, NSString* key);
NSString *AESDecryptWithKey(NSString* nsSrc, NSString* key);
