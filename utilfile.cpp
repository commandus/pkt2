/*
 * utilfile.cpp
 *
 */
#include <iostream>

#ifdef _MSC_VER
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#define PATH_DELIMITER "\\"
#else
#include <sys/param.h>
#include <fcntl.h>
#include <ftw.h>
#include <unistd.h>
#define PATH_DELIMITER "/"

#ifndef F_GETPATH
#define F_GETPATH	(1024 + 7)
#endif

#endif

#include "utilfile.h"

#ifdef _MSC_VER
bool rmAllDir(const char *path)
{
	if (&path == NULL)
		return false;
	int sz = strlen(path);
	if (sz <= 1)
		return false;	// prevent "rm -r /"
	char fp[MAX_PATH];
	memmove(fp, path, sz);
	fp[sz] = '\0';
	fp[sz + 1] = '\0';
	SHFILEOPSTRUCTA shfo = {
		NULL,
		FO_DELETE,
		fp,
		NULL,
		FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION,
		FALSE,
		NULL,
		NULL };

	SHFileOperationA(&shfo);
}

bool rmDir(const std::string &path)
{
	if (&path == NULL)
		return false;
	if (path.size() <= 1)
		return false;	// prevent "rm -r /"
	const char *sDir = path.c_str();
	WIN32_FIND_DATAA fdFile;
	HANDLE hFind;
	char sPath[MAX_PATH];
	sprintf(sPath, "%s\\*.*", sDir);
	if ((hFind = FindFirstFileA(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
		return false;
	do
	{
		if (strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0)
		{
			sprintf(sPath, "%s\\%s", sDir, fdFile.cFileName);
			if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Is Directory
				rmAllDir(sPath);
			}
		}
	} while (FindNextFileA(hFind, &fdFile));
	FindClose(hFind);
	return true;
}

#else
/**
* FTW_D	directory
* FTW_DNR	directory that cannot be read
* FTW_F	file
* FTW_SL	symbolic link
* FTW_NS	other than a symbolic link on which stat() could not successfully be executed.
*/
static int rmnode
(
	const char *path,
	const struct stat *ptr,
	int flag,
	struct FTW *ftwbuf
)
{
	int(*rm_func)(const char *);

	switch (flag)
	{
	case FTW_D:
	case FTW_DP:
		rm_func = rmdir;
	default:
		rm_func = unlink;
		break;
	}
	rm_func(path);
	return 0;
}

bool rmDir(const std::string &path)
{
	if (&path == NULL)
		return false;
	if (path.size() <= 1)
		return false;	// prevent "rm -r /"
	return nftw(path.c_str(), rmnode,  64, FTW_DEPTH) == 0;
}

bool iteratePath
(
	const std::string &path,
	NodeCallback callback
)
{
	if ((&path == NULL) || (callback == NULL))
		return false;

	return nftw(path.c_str(), callback,  64, FTW_DEPTH) == 0;
}

#endif

bool rmFile(const std::string &fn)
{
	return std::remove((const char*) fn.c_str()) == 0;
}

/**
 * Does not work
 */
std::string getFilePathFromDescriptor(int fd)
{
	char filePath[MAXPATHLEN];
	if (fcntl(fd, F_GETPATH, filePath) != -1)
		return std::string(filePath);
	else
		return "";
}
