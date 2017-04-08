/*
 * google-sheets.h
 * @brief read/write Google speadsheet
 *
 */

#ifndef GOOGLE_SHEETS_H_
#define GOOGLE_SHEETS_H_

#include <string>
#include <vector>
#include <istream>

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
  * @brief load Google service token bearer from rhe Google service JSON object
  */
bool readGoogleTokenJSON
(
	const std::string &json,
	std::string &ret_service_account,
	std::string &ret_pemkey
);

/**
 * @brief Keep cell range values in two-dimensional array
 */ 
class ValueRange {
public:
	ValueRange();
	ValueRange
	(
		const std::string &range, 
		std::istream &stream);
	std::string range;				///< e.g. 'Sheet'!A1:A2
	std::string major_dimension;	///< e.g. "ROWS"
	std::vector<std::vector<std::string>> values;

	std::string toString() const;
	std::string toJSON() const;

	int parseCSV(const std::string &csv);
	int parseJSON(const std::string &json);
};

/**
  * @brief read/write Google spreadsheet cells values
  */
class GoogleSheets {
private:
	std::string sheet_id;
	std::string token;
	std::vector<std::string> genTokenParams;
protected:	
	/**
	  * Re-generate token
	  */
	bool genToken();

public:
	/**
	  * @brief token bearer already exists
	  */
	GoogleSheets
	(
		const std::string sheetid,
		const std::string tokenbearer
	);

	GoogleSheets
	(
		const std::string &sheetid,
		const std::string &service_account,
		const std::string &subject_email,
		const std::string &pemkey,
		const std::string &scope,
		const std::string &audience
	);

	virtual ~GoogleSheets();
	int getRange(const std::string &range, ValueRange &retval);
	int putRange(const ValueRange &values);
};

#endif /* GOOGLE_SHEETS_H_ */
