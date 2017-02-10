/*
 * error-printer.cpp
 * Based on command_line_interface.h Copyright 2008 Google Inc.  All rights reserved.
 */
#include <iostream>
#include "error-printer.h"

void ErrorPrinter::AddError(int line, int column, const std::string& message)
{
	// log error
	std::cerr << "Error at " << line << ", " << column << ": " << message << std::endl;
}

void ErrorPrinter::AddWarning(int line, int column, const std::string& message)
{
	// log warning
	std::cerr << "Warning at "<< line << ", " << column << ": " << message << std::endl;
}


MFErrorPrinter::MFErrorPrinter
(
)
{

}

MFErrorPrinter::~MFErrorPrinter() {

}

// implements MultiFileErrorCollector ------------------------------

void MFErrorPrinter::AddError(
		const std::string &filename,
		int line,
		int column,
		const std::string &message
) {
	AddErrorOrWarning(filename, line, column, message, "error", std::cerr);
}

void MFErrorPrinter::AddWarning(
		const std::string &filename,
		int line,
		int column,
		const std::string &message
) {
	AddErrorOrWarning(filename, line, column, message, "warning", std::clog);
}

// implements io::ErrorCollector -----------------------------------
void MFErrorPrinter::AddError(
		int line,
		int column,
		const std::string &message
) {
	AddError("input", line, column, message);
}

void MFErrorPrinter::AddWarning(
		int line,
		int column,
		const std::string &message
) {
	AddErrorOrWarning("input", line, column, message, "warning", std::clog);
}

void MFErrorPrinter::AddErrorOrWarning(
		const std::string &filename,
		int line,
		int column,
		const std::string& message,
		const std::string &type,
		std::ostream& out
) {
	// Print full path when running under MSVS
	std::string dfile;
	out << filename;

	// Users typically expect 1-based line/column numbers, so we add 1 to each here.
	if (line != -1) {
		// Allow for both GCC- and Visual-Studio-compatible output.
		out << ":" << (line + 1) << ":" << (column + 1);
	}

	if (type == "warning") {
		out << ": warning: " << message << std::endl;
	} else {
		out << ": " << message << std::endl;
	}
}
