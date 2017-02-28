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
 * @param retval can be NULL
 * @return count files
 */
size_t filesInPath
(
	const std::string &path,
	const std::string &suffix,
	std::vector<std::string> *retval
);

/**
 * Does not work
 */
std::string getFilePathFromDescriptor(int fd);
