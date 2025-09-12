// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#include "rtmacoshelper.h"
#include <Foundation/NSData.h>
#include <Foundation/NSString.h>
#include <Foundation/NSURL.h>
#include <AppKit/NSOpenPanel.h>

/**
 *
 */
const char *GetBundleVersion()
{
    NSBundle *bundle = [NSBundle mainBundle];
    if (!bundle)
        return "Unknown";

    NSDictionary *infoDict = [bundle infoDictionary];
    NSString *version = [infoDict objectForKey:@"CFBundleVersion"];
    return version ? [version UTF8String] : "0.0";
}

/**
 *
 */
const char *GetBuildNumber()
{
    NSBundle *bundle = [NSBundle mainBundle];
    if (!bundle)
        return "Unknown";

    NSDictionary *infoDict = [bundle infoDictionary];
    NSString *build = [infoDict objectForKey:@"CFBundleShortVersionString"];
    return build ? [build UTF8String] : "0";
}
