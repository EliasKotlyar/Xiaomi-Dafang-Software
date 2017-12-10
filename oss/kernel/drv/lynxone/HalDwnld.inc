/***************************************************************************
 HalDwnld.c

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

//#include <HalAudio.h>
//#include <HalDwnld.h>

#ifdef DOS
#include <Pathfind.h>
/*
	#include <stdlib.h>	
	#include <io.h>
	#include <fcntl.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <malloc.h>
*/
#endif

#ifdef WIN95VXD
#include <Pathfind.h>
#endif

#undef DEBUG
#ifdef NT
#ifndef DEBUG
#include <Pathfind.h>
#endif
#endif

#ifdef MACINTOSH
#include <Pathfind.h>
#endif

/////////////////////////////////////////////////////////////////////////////
BOOLEAN
ResourceOpen (char *szFileName, PDOWNLOADRESOURCE pF)
/////////////////////////////////////////////////////////////////////////////
{
#ifdef NT
#ifdef DEBUG
  HANDLE KeyHandle;
  OBJECT_ATTRIBUTES ObjAttr;
  UNICODE_STRING RegPath;
  WCHAR StringBuffer[128];
  UNICODE_STRING ValueName;
  WCHAR ValueBuffer[80];
  ANSI_STRING AnsiString;
  NTSTATUS Status;
  ULONG lLength;
  ULONG ulKeySize;

  InitUnicodeString (&RegPath, StringBuffer, sizeof (StringBuffer));
  RtlAppendUnicodeToString (&RegPath,
			    L"\\REGISTRY\\Machine\\Software\\Lynx\\");

  // use macro in NTDEF.H to setup the ObjAttr fields
  InitializeObjectAttributes ((POBJECT_ATTRIBUTES) & ObjAttr, &RegPath,
			      (ULONG) OBJ_CASE_INSENSITIVE, (HANDLE) NULL,
			      (PSECURITY_DESCRIPTOR) NULL);

  //DPF(("ZwOpenKey [%ws]\n", RegPath.Buffer));
  // open the key
  Status = ZwOpenKey (&KeyHandle, KEY_READ, &ObjAttr);
  if (!NT_SUCCESS (Status))
    {
      DPF (("Could not open registry key for file.\n"));
      return (FALSE);
    }

  // make the filename a unicode string
  InitUnicodeString (&ValueName, ValueBuffer, sizeof (ValueBuffer));
  RtlInitAnsiString (&AnsiString, szFileName);
  RtlAnsiStringToUnicodeString (&ValueName, &AnsiString, FALSE);

  //DPF(("ZwQueryValueKey [%ws]\n", ValueName.Buffer));
  // get the size of the object
  Status =
    ZwQueryValueKey (KeyHandle, &ValueName, KeyValuePartialInformation,
		     (PVOID) NULL, 0L, &ulKeySize);

  if ((Status == STATUS_BUFFER_OVERFLOW)
      || (Status == STATUS_BUFFER_TOO_SMALL))
    {
      //DPF(( "Need to allocate [%lu] bytes\n", ulKeySize ));
    }
  else if (Status)
    {
      DPF (("Could not get size of file [%08lx]\n", Status));
      ZwClose (KeyHandle);
      return (FALSE);
    }

  (PVOID) pF->pKeyValueInfo = ExAllocatePool (PagedPool, (ULONG) ulKeySize);
  if (pF->pKeyValueInfo == NULL)
    {
      DPF (("Could Not Allocate Memory for Registry Key File.\n"));
      ZwClose (KeyHandle);
      return (FALSE);
    }

  lLength = ulKeySize;

  Status =
    ZwQueryValueKey (KeyHandle, &ValueName, KeyValuePartialInformation,
		     pF->pKeyValueInfo, lLength, &ulKeySize);
  if (!NT_SUCCESS (Status))
    {
      DPF (("Could not get key value.\n"));
      ZwClose (KeyHandle);
      return (FALSE);
    }

  pF->pFileBaseAddr = pF->pKeyValueInfo->Data;
  pF->ulFileSize = pF->pKeyValueInfo->DataLength;

  ZwClose (KeyHandle);
  return (TRUE);
#else // RELEASE
  pF->pFileBaseAddr = PathfindRbf;
  pF->ulFileSize = 0x7e81;

  return (TRUE);
#endif // DEBUG
#endif // NT
#ifdef DOS
  pF->pFileBaseAddr = PathfindRbf;
  pF->ulFileSize = 0x7e81;
/*
	int		fh;
	
	// open the file
	fh = open( szFileName, O_BINARY | O_RDONLY );
	if( fh == -1 )
		return( FALSE );

	// find the end of the file
	lseek( fh, 0, SEEK_END );

	// get the file length
	pF->ulFileSize = tell( fh );
	
	// go back to the start of the file
	lseek( fh, 0, SEEK_SET );

	// allocate enough memory to hold the file
	pF->pFileBaseAddr = malloc( pF->ulFileSize );
	if( !pF->pFileBaseAddr )
		return( FALSE );
	
	// read the file into memory
	read( fh, pF->pFileBaseAddr, pF->ulFileSize );

	// close the file
	close( fh );
*/
  return (TRUE);
#endif
#ifdef WIN95VXD
  pF->pFileBaseAddr = PathfindRbf;
  pF->ulFileSize = 0x7e81;

  return (TRUE);
#endif
#ifdef MACINTOSH
  pF->pFileBaseAddr = PathfindRbf;
  pF->ulFileSize = 0x7e81;

  return (TRUE);
#endif
#ifdef OSS
  pF->pFileBaseAddr = PathfindRbf;
  pF->ulFileSize = 0x7e81;

  return (TRUE);
#endif
}


/////////////////////////////////////////////////////////////////////////////
void
ResourceClose (PDOWNLOADRESOURCE pF)
/////////////////////////////////////////////////////////////////////////////
{
#ifdef NT
  if (pF->pKeyValueInfo)
    ExFreePool (pF->pKeyValueInfo);
  pF->pKeyValueInfo = NULL;
#endif
#ifdef DOS
  free (pF->pFileBaseAddr);
#endif
#ifdef WIN95VXD
  // nothing to do here
#endif
#ifdef MACINTOSH
  // nothing to do here
#endif
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDownloadFile (PVOID pContext, char *pszFileName)
//  pA,             ptr to adapter structure
//  *pszFileName    name of file to load
/////////////////////////////////////////////////////////////////////////////
{
  DOWNLOADRESOURCE DownloadResource;
  PDOWNLOADRESOURCE pF = &DownloadResource;
  USHORT usStatus;

  //DPF(("HalDownloadFile\n"));

  // make sure the global info starts out as zero
  RtlZeroMemory (pF, sizeof (DOWNLOADRESOURCE));

  if (!ResourceOpen (pszFileName, pF))
    {
      DPF (("Cannot open resource!\n"));
      return (HSTATUS_DOWNLOAD_FAILED);
    }

  // the pathfinder only has one section, so the load address is zero
  usStatus =
    HalDownloadBuffer (pContext, pF->pFileBaseAddr, 0, pF->ulFileSize);

  ResourceClose (pF);
  return (usStatus);
}
