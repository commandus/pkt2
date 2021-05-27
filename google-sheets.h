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
	std::string &retval,
	int verbosity
);


/**
  * @brief load Google service token bearer from rhe Google service JSON object
  */
bool readGoogleTokenJSON
(
	const std::string &json,
	std::string &ret_service_account,
	std::string &ret_pemkey,
	std::string &subject_email
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
		std::istream &stream
	);
	std::string range;				///< e.g. 'Sheet'!A1:A2
	std::string major_dimension;	///< e.g. "ROWS"
	std::vector<std::vector<std::string> > values;

	std::string toString() const;
	std::string toJSON() const;

	void clear();
	int parseCSV(const std::string &csv);
	int parseJSON(const std::string &json);
};

typedef void (*on_token_bearer)
(
    void *env,
	const std::string &value,
	int status
);

/**
  * @brief read/write Google spreadsheet cells values
  */
class GoogleSheets {
private:
	int verbosity;
protected:	
	/**
	  * Re-generate token
	  */
	bool genToken();

public:
	GoogleSheets
	(
		const std::string &sheetid,
		const std::string tokenbearer,
		const std::string &service_account,
		const std::string &subject_email,
		const std::string &pemkey,
		const std::string &scope,
		const std::string &audience,
		on_token_bearer onTokenbearer,
		void *env,
		int verbosity
	);
	
	void setOnTokenBearer(
		void *env,
		on_token_bearer value
	);

	virtual ~GoogleSheets();
	
	static std::string A1(
		const std::string &sheet,
		int col, 
		int row
	);
	
	static std::string A1(
		const std::string &sheet,
		int col0, 
		int row0,
		int col1, 
		int row1
	);
	
	bool get(
		const std::string &range, 
		ValueRange &retval
	);
	
	bool get(
		const std::string &sheet,
		int col, 
		int row,
		ValueRange &retval
	);

	bool put(
		const ValueRange &values
	);
	
	bool append(
		const ValueRange &values
	);

	void *env;	
	std::string sheet_id;
	std::string token;
	std::vector<std::string> genTokenParams;
	
	int checkJSONErrorCode(
		const std::string &response
	);

	bool token_get(
		const std::string &url, 
		ValueRange &response
	);

	bool token_put(
		const std::string &url, 
		const ValueRange &value
	);
	
	/**
  * @brief POST spreadsheet range values. If token expires, regenerate token bearer and try again
  */
	bool token_post(
		const std::string &url,
		const ValueRange &values
	);

	on_token_bearer ontokenbearer;
	
};

#endif /* GOOGLE_SHEETS_H_ */
