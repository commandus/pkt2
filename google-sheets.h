/*
 * google-sheets.h
 * @brief read/write Google speadsheet
 *
 */

#ifndef GOOGLE_SHEETS_H_
#define GOOGLE_SHEETS_H_

#include <string>
#include <vector>

#include <curl/curl.h>

/**
  * @brief load Google service token bearer
  * @return "" if fails
  */
int loadGoogleToken
(
	const std::string &service_account,
	const std::string &subject_email,
	const std::string &pemkey,
	const std::string &scope,
	const std::string &audience,
	int expires,
	std::string &retval
);

/**
 * @brief Keep cell range values in two-dimensional array
 */ 
class ValueRange {
public:
	std::string range;				///< e.g. 'Sheet'!A1:A2
	std::string major_dimension;	///< e.g. "ROWS"
	std::vector<std::vector<std::string>> values;

	int parse(const std::string &json);
	std::string toString();
};

/**
  * @brief read/write Google spreadsheet cells values
  */
class GoogleSheets {
private:
	std::string sheet_id;
	std::string token;
public:
	/**
	  * @brief token bearer already exists
	  */
	GoogleSheets(
		const std::string sheetid,
		const std::string tokenbearer
	);

	virtual ~GoogleSheets();
	int getRange(const std::string &range, ValueRange &retval);
	int putRange(const std::string &range, ValueRange &retval);
};

#endif /* GOOGLE_SHEETS_H_ */
