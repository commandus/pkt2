/*
 * error-printer.h
 * Based on command_line_interface.h Copyright 2008 Google Inc.  All rights reserved.
 */
#ifndef ERROR_PRINTER_H_
#define ERROR_PRINTER_H_

#include <string>
#include <google/protobuf/compiler/parser.h>
#include <google/protobuf/compiler/importer.h>

class ErrorPrinter :
	public google::protobuf::io::ErrorCollector
{
public:
	void AddError(int line, int column, const std::string& message) 
#if __cplusplus >= 201103L
	override
#endif
	;
	void AddWarning(int line, int column, const std::string& message)
#if __cplusplus >= 201103L
	override
#endif
	;
};

/**
 * A MultiFileErrorCollector that prints errors to stderr.
 */

class MFErrorPrinter :
	public google::protobuf::compiler::MultiFileErrorCollector,
	public google::protobuf::io::ErrorCollector
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
	MFErrorPrinter();
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
