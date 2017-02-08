/*
 * utilfile.h
 */

typedef int (*NodeCallback)
(
	const char *path,
	const struct stat *ptr,
	int flag,
	struct FTW *ftwbuf
);

bool rmDir(const std::string &path);
bool rmFile(const std::string &fn);

bool iteratePath
(
	const std::string &path,
	NodeCallback callback
);
