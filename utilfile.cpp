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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>
#include <string.h>
#include <errno.h>
#include <cstdio>

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

/**
 * Return list of files in specified path
 * @param path
 * @param retval can be NULL
 * @return count files
 */
size_t filesInPath
(
	const std::string &path,
	const std::string &suffix,
	std::vector<std::string> *retval
)
{
	// TODO Implement Windows
	return 0;
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
		break;
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

int compareFile
(
		const FTSENT **a,
		const FTSENT **b
)
{
	return strcmp((*a)->fts_name, (*b)->fts_name);
}

/**
 * Return list of files in specified path
 * @param path
 * @param flags 0- as is, 1- full path, 2- relative (remove parent path)
 * @param retval can be NULL
 * @return count files
 * FreeBSD fts.h fts_*()
 */
size_t filesInPath
(
	const std::string &path,
	const std::string &suffix,
	int flags,
	std::vector<std::string> *retval
)
{
	if (&path == NULL)
		return 0;
	char *pathlist[2];
	pathlist[1] = NULL;
	if (flags & 1)
	{
		char realtapth[PATH_MAX+1];
		pathlist[0] = realpath((char *) path.c_str(), realtapth);
	}
	else
	{
		pathlist[0] = (char *) path.c_str();
	}
	int parent_len = strlen(pathlist[0]) + 1;	///< Arggh. Remove '/' path delimiter(I mean it 'always' present). Not sure is it works fine. It's bad, I know.

	FTS* file_system = fts_open(pathlist, FTS_LOGICAL | FTS_NOSTAT, NULL);

    if (!file_system)
    	return 0;
    size_t count = 0;
    FTSENT* parent;
	while((parent = fts_read(file_system)))
	{
		FTSENT* child = fts_children(file_system, 0);
		if (errno != 0)
		{
			// ignore, perhaps permission error
		}
		while (child)
		{
			switch (child->fts_info) {
				case FTS_F:
					{
						std::string s(child->fts_name);
						if (s.find(suffix) != std::string::npos)
						{
							count++;
							if (retval)
							{
								if (flags & 2)
								{
									// extract parent path
									std::string p(&child->fts_path[parent_len]);
									retval->push_back(p + s);
								}
								else
									retval->push_back(std::string(child->fts_path) + s);
							}
						}
					}
					break;
				default:
					break;
			}
			child = child->fts_link;
		}
	}
	fts_close(file_system);
	return count;
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

