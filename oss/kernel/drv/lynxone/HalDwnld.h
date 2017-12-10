/***************************************************************************
 HalDwnld.h

 Created: David A. Hoatson, March 1997
	
 Copyright © 1998, 1999	Lynx Studio Technology, Inc.

 This software contains the valuable TRADE SECRETS and CONFIDENTIAL INFORMATION 
 of Lynx Studio Technology, Inc. The software is protected under copyright 
 laws as an unpublished work of Lynx Studio Technology, Inc.  Notice is 
 for informational purposes only and does not imply publication.  The user 
 of this software may make copies of the software for use with products 
 manufactured by Lynx Studio Technology, Inc. or under license from 
 Lynx Studio Technology, Inc. and for no other use.

 THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 PURPOSE.

 Environment: Windows NT Kernel mode

 Revision History
 
 When      Who  Description
 --------- ---  ---------------------------------------------------------------
****************************************************************************/

#ifndef _HALDOWNLOAD_H
#define _HALDOWNLOAD_H

/////////////////////////////////////////////////////////////////////////////
//  Structures & the Like
/////////////////////////////////////////////////////////////////////////////

#ifdef NT
#define HKEY_LOCAL_MACHINE          ( 0x80000002 )

VOID InitUnicodeString (IN OUT PUNICODE_STRING pUnicodeStr, IN PWSTR pBuffer,
			IN USHORT usMaxLength);
#endif

typedef struct _DOWNLOAD_RESOURCE
{
  PVOID pFileBaseAddr;
#ifdef NT
  PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo;
#endif
  ULONG ulFileSize;
}
DOWNLOADRESOURCE, *PDOWNLOADRESOURCE;

/////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
/////////////////////////////////////////////////////////////////////////////
USHORT HalDownloadFile (PVOID pContext, char *pszFileName);
USHORT HalDownloadBuffer (PVOID pContext, PBYTE pBuffer, ULONG ulLoadOffset,
			  ULONG ulLoadLength);
BOOLEAN ResourceOpen (char *szFileName, PDOWNLOADRESOURCE pF);
void ResourceClose (PDOWNLOADRESOURCE pF);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( init, HalDownloadFile )
#pragma alloc_text( init, ResourceOpen )
#pragma alloc_text( init, HalDownloadBuffer )
#pragma alloc_text( init, ResourceClose )
#endif

#endif // _HALDOWNLOAD_H
