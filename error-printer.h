/*
 * error-printer.h
 * Based on command_line_interface.h Copyright 2008 Google Inc.  All rights reserved.
 */
#ifndef ERROR_PRINTER_H_
#define ERROR_PRINTER_H_

#include <string>
#include <google/protobuf/compiler/parser.h>
#include <google/protobuf/compiler/importer.h>

/**
 * A MultiFileErrorCollector that prints errors to stderr.
 */

class MFErrorPrinter :
		public google::protobuf::io::ErrorCollector,
		public google::protobuf::compiler::MultiFileErrorCollector
{
private:
	google::protobuf::compiler::DiskSourceTree *tree_;

	void AddErrorOrWarning (
		const std::string &filename,
		int line,
		int column,
		const std::string &message,
		const std::string &type,
		std::ostream& out
	);
public:
	MFErrorPrinter (
		google::protobuf::compiler::DiskSourceTree *tree
	);
	~MFErrorPrinter();

	// implements MultiFileErrorCollector

	void AddError(
			const std::string &filename,
			int line,
			int column,
			const std::string &message);

	void AddWarning (
			const std::string &filename,
			int line,
			int column,
			const std::string &message
	);

	// implements io::ErrorCollector

	void AddError (
			int line,
			int column,
			const std::string &message
	);

	void AddWarning (
			int line,
			int column,
			const std::string &message
	);

};

#endif /* ERROR_PRINTER_H_ */
