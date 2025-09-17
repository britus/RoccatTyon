// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#ifndef RTMACOSHELPER_H
#define RTMACOSHELPER_H

/**
 * @brief Get app bundle version from Info.plist
 * @return String major.minor.rc
 */
const char *GetBundleVersion();

/**
 * @brief Get app build number from Info.plist
 * @return String
 */
const char *GetBuildNumber();

#endif /* RTMACOSHELPER_H */
