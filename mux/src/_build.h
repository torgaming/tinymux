// _build.h
//
// $Id: _build.h,v 1.58 2003-02-20 07:58:25 sdennis Exp $
//
// MUX 2.2
// Copyright (C) 1998 through 2003 Solid Vertical Domains, Ltd. All
// rights not explicitly given are reserved.  
//
#ifndef MUX_BUILD_NUM
extern char szBuildNum[];
#define MUX_BUILD_NUM szBuildNum
#endif // MUX_BUILD_NUM

#ifndef MUX_BUILD_DATE
extern char szBuildDate[];
#define MUX_BUILD_DATE szBuildDate
#endif // MUX_BUILD_DATE

#define MUX_VERSION       "2.2.5.56"     // Version number
#define MUX_RELEASE_DATE  "2003-FEB-19"  // Source release date

// Define if this release is qualified as ALPHA or BETA.
//
//#define ALPHA
#define BETA
