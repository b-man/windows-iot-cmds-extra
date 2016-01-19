/*
* PROJECT:     ReactOS
* LICENSE:     GNU GPLv2 only as published by the Free Software Foundation
* PURPOSE:     Implements tree.com functionality identical to Windows
* PROGRAMMERS: Asif Bahrainwala (asif_bahrainwala@hotmail.com)
*
* Adapted for use in Windows IoT by Brian McKenzie (mckenzba@gmail.com)
*/

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <windows.h>
#include <strsafe.h>

#define STR_MAX 2048

static VOID GetDirectoryStructure(wchar_t* strPath, UINT width, const wchar_t* prevLine);

/* if this flag is set to true, files will also be listed */
BOOL bShowFiles = FALSE;

/* if this flag is true, ASCII characters will be used instead of UNICODE ones */
BOOL bUseAscii = FALSE;

/* if this flag is true, a path has been specified and folders/files will be listed from there */
BOOL bSetPath = FALSE;

static VOID PrintUsage(VOID)
{
	fwprintf(stderr,
		L"Graphically displays the folder structure of a drive or path.\n\n"
		L"TREE [drive:][path] [/F] [/A]\n\n"
		L"   /F   Display the names of the files in each folder.\n"
		L"   /A   Use ASCII instead of extended characters.\n\n"
	);
}

/**
* @name: HasSubFolder
*
* @param strPath
* Must specify folder name
*
* @return
* true if folder has sub folders, else will return false
*/
static BOOL HasSubFolder(const wchar_t *strPath)
{
	BOOL ret = FALSE;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = NULL;
	wchar_t folderPath[STR_MAX] = L"";

	ZeroMemory(folderPath, sizeof(folderPath));

	wcscat_s(folderPath, STR_MAX, strPath);
	wcscat_s(folderPath, STR_MAX, L"\\*.");

	hFind = FindFirstFile(folderPath, &FindFileData);
	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(FindFileData.cFileName, L".") == 0 ||
				wcscmp(FindFileData.cFileName, L"..") == 0)
			{
				continue;
			}

			ret = TRUE;  /* found subfolder */
			break;
		}
	} while (FindNextFile(hFind, &FindFileData));

	FindClose(hFind);
	return ret;
}

/**
* @name: DrawTree
*
* @param strPath
* Must specify folder name
*
* @param arrFolder
* must be a list of folder names to be drawn in tree format
*
* @param width
* specifies drawing distance for correct formatting of tree structure being drawn on console screen
* used internally for adding spaces
*
* @param prevLine
* used internally for formatting reasons
*
* @return
* void
*/
static VOID DrawTree(const wchar_t* strPath,
	const WIN32_FIND_DATA *arrFolder,
	const size_t szArr,
	UINT width,
	const wchar_t *prevLine,
	BOOL drawfolder)
{
	BOOL bHasSubFolder = HasSubFolder(strPath);
	UINT i = 0;

	/* this will format the spaces required for correct formatting */
	for (i = 0; i < szArr; ++i)
	{
		wchar_t *consoleOut = (wchar_t*)malloc(sizeof(wchar_t) * STR_MAX);
		UINT j = 0;
		wchar_t str[STR_MAX];

		wcscpy_s(consoleOut, STR_MAX, L"");
		wcscpy_s(str, STR_MAX, L"");

		for (j = 0; j < width - 1; ++j)
		{
			/*
			 * if the previous line has '├' or '│' then the current line will
			 * add '│' to continue the connecting line
			 */
			if (prevLine[j] == L'\u251c' || prevLine[j] == L'\u2502' ||
				(BYTE)prevLine[j] == L'+' || (BYTE)prevLine[j] == L'|')
			{
				if (!bUseAscii)
				{
					wchar_t a[] = { L'\u2502', L'\u0000' };
					wcscat_s(consoleOut, STR_MAX, a);
				}
				else
				{
					wcscat_s(consoleOut, STR_MAX, L"|");
				}
			}
			else
			{
				wcscat_s(consoleOut, STR_MAX, L" ");
			}
		}

		if (szArr - 1 != i)
		{
			if (drawfolder)
			{
				/* will add '├───Folder name */
				if (bUseAscii)
					StringCchPrintf(str, STR_MAX, L"+---%s", (wchar_t*)arrFolder[i].cFileName);
				else
					StringCchPrintf(str, STR_MAX, L"%c%c%c%c%s", L'\u251c', L'\u2500', L'\u2500', L'\u2500', (wchar_t*)arrFolder[i].cFileName);
			}
			else
			{
				if (bHasSubFolder)
				{
					/* will add '│   FileName' */
					if (bUseAscii)
						StringCchPrintf(str, STR_MAX, L"|   %s", (wchar_t*)arrFolder[i].cFileName);
					else
						StringCchPrintf(str, STR_MAX, L"%c   %s", L'\u2502', (wchar_t*)arrFolder[i].cFileName);
				}
				else
				{
					/* will add '    FileName' */
					StringCchPrintf(str, STR_MAX, L"     %s", (wchar_t*)arrFolder[i].cFileName);
				}
			}
		}
		else
		{
			if (drawfolder)
			{
				/* '└───Folder name' */
				if (bUseAscii)
					StringCchPrintf(str, STR_MAX, L"\\---%s", (wchar_t*)arrFolder[i].cFileName);
				else
					StringCchPrintf(str, STR_MAX, L"%c%c%c%c%s", L'\u2514', L'\u2500', L'\u2500', L'\u2500', (wchar_t*)arrFolder[i].cFileName);
			}
			else
			{
				if (bHasSubFolder)
				{
					/* '│   FileName' */
					if (bUseAscii)
						StringCchPrintf(str, STR_MAX, L"|   %s", (wchar_t*)arrFolder[i].cFileName);
					else
						StringCchPrintf(str, STR_MAX, L"%c   %s", L'\u2502', (wchar_t*)arrFolder[i].cFileName);
				}
				else
				{
					/* '    FileName' */
					StringCchPrintf(str, STR_MAX, L"     %s", (wchar_t*)arrFolder[i].cFileName);
				}
			}
		}

		wcscat_s(consoleOut, STR_MAX, str);
		wprintf(L"%s\n", consoleOut);

		if (drawfolder)
		{
			wchar_t *str = (wchar_t*)malloc(STR_MAX * sizeof(wchar_t));
			ZeroMemory(str, STR_MAX * sizeof(wchar_t));

			wcscat_s(str, STR_MAX, strPath);
			wcscat_s(str, STR_MAX, L"\\");
			wcscat_s(str, STR_MAX, arrFolder[i].cFileName);
			GetDirectoryStructure(str, width + 4, consoleOut);

			free(str);
		}
		free(consoleOut);
	}
}

/**
* @name: GetDirectoryStructure
*
* @param strPath
* Must specify folder name
*
* @param width
* specifies drawing distance for correct formatting of tree structure being drawn on console screen
*
* @param prevLine
* specifies the previous line written on console, is used for correct formatting
* @return
* void
*/
static VOID
GetDirectoryStructure(wchar_t* strPath, UINT width, const wchar_t* prevLine)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = NULL;
	/* will fill up with names of all sub folders */
	WIN32_FIND_DATA* arrFolder = NULL;
	UINT arrFoldersz = 0;
	/* will fill up with names of all sub folders */
	WIN32_FIND_DATA* arrFile = NULL;
	UINT arrFilesz = 0;

	ZeroMemory(&FindFileData, sizeof(FindFileData));

	static wchar_t tmp[STR_MAX] = L"";
	ZeroMemory(tmp, sizeof(tmp));
	wcscat_s(tmp, STR_MAX,  strPath);
	wcscat_s(tmp, STR_MAX, L"\\*.*");
	hFind = FindFirstFile(tmp, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(FindFileData.cFileName, L".") == 0 ||
				wcscmp(FindFileData.cFileName, L"..") == 0)
				continue;

			++arrFoldersz;
			arrFolder = (WIN32_FIND_DATA*)realloc(arrFolder, arrFoldersz * sizeof(FindFileData));

			if (arrFolder == NULL)
				exit(-1);

			arrFolder[arrFoldersz - 1] = FindFileData;

		}
		else
		{
			++arrFilesz;
			arrFile = (WIN32_FIND_DATA*)realloc(arrFile, arrFilesz * sizeof(FindFileData));

			if (arrFile == NULL)
				exit(-1);

			arrFile[arrFilesz - 1] = FindFileData;
		}
	} while (FindNextFile(hFind, &FindFileData));

	FindClose(hFind);

	if (bShowFiles)
	{
		/* spoof find data so DrawTree will leave blank line below each file listing */
		if (arrFilesz > 0)
		{
			++arrFilesz;
			arrFile = (WIN32_FIND_DATA*)realloc(arrFile, arrFilesz * sizeof(FindFileData));

			if (arrFile == NULL)
				exit(-1);

			wcscpy_s(arrFile[arrFilesz - 1].cFileName, MAX_PATH, L" ");
		}

		DrawTree(strPath, arrFile, arrFilesz, width, prevLine, FALSE);
	}

	DrawTree(strPath, arrFolder, arrFoldersz, width, prevLine, TRUE);

	free(arrFolder);
	free(arrFile);
}

/**
* @name: main
* standard main functionality as required by C/C++ for application startup
*
* @return
* error /success value
*/
int wmain(int argc, wchar_t* argv[])
{
	DWORD dwSerial = 0;
	wchar_t dwName[MAX_PATH] = L"";
	wchar_t* strPath = NULL;
	DWORD sz = 0;
	wchar_t specifiedPath[MAX_PATH] = L"";
	int i;

	/* this is necessary if tree is called in non-ascii mode (default) */
	_setmode(_fileno(stdout), _O_U8TEXT);

	/* parse the command line */
	for (i = 1; i < argc; ++i)
	{
		if (argv[i][0] == L'-' || argv[i][0] == L'/')
		{
			switch (towlower(argv[i][1]))
			{
			case L'?':
				/* will print help and exit after */
				PrintUsage();
				return 0;
			case L'f':
				/* if set to true, will populate all the files within the folder structure */
				bShowFiles = TRUE;
				break;
			case L'a':
				bUseAscii = TRUE;
				break;
			default:
				break;
			}
		}
		else
		{
			/* this only happens once */
			if (bSetPath == FALSE)
			{
				/* user has specified path, convert to absolute path if necessary */
				_wfullpath(specifiedPath, argv[i], MAX_PATH);
				bSetPath = TRUE;
			}
			else
			{
				fwprintf(stderr, L"Too many parameters - %s\n\n", argv[i]);

				return 0;
			}
		}
	}

	/* display banner */
	GetVolumeInformation(NULL, dwName, MAX_PATH, &dwSerial, NULL, NULL, NULL, 0);
	wprintf(L"Folder PATH listing for volume %s\n", dwName);
	wprintf(L"Volume serial number is %X-%X\n", dwSerial >> 16, dwSerial & 0xffff);

	if (bSetPath == TRUE) /* if a path is specified, display absolute path */
	{
		CharUpper(specifiedPath);

		wprintf(L"%s\n", specifiedPath);

		/* if we fail here, assume we had no subfolders and exit */
		if (SetCurrentDirectory(specifiedPath) == FALSE)
		{
			strPath = wcschr(specifiedPath, L'\\');
			fwprintf(stderr, L"Invalid path - %s\n", strPath);
			fwprintf(stderr, L"No subfolders exist\n\n");

			return 0;
		}
	}
	else /* if no path is specified, display drive letter and relative path */
	{
		wprintf(L"%c:.\n", (_getdrive() + 'A' - 1));
	}

	/* get the current directory */
	sz = GetCurrentDirectory(0, NULL);
	strPath = (wchar_t*)malloc(sizeof(wchar_t) * sz);
	GetCurrentDirectory(sz, strPath);

	/* get the sub directories within this current folder */
	GetDirectoryStructure(strPath, 1, L"          ");

	/* if we didn't find any sub directories, state so */
	if (HasSubFolder(strPath) == FALSE)
		fwprintf(stderr, L"No subfolders exist\n\n");

	free(strPath);

	return 0;
}
