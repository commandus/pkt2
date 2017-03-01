/*
 * utilfile.h
 */

#include <string>
#include <vector>

bool rmDir(const std::string &path);
bool rmFile(const std::string &fn);

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
);

/**
 * Does not work
 */
std::string getFilePathFromDescriptor(int fd);
