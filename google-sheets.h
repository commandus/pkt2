/*
 * google-sheets.h
 *
 *  Created on: Apr 7, 2017
 *      Author: andrei
 */

#ifndef GOOGLE_SHEETS_H_
#define GOOGLE_SHEETS_H_

#include <string>
#include <vector>
/**
  *
  {
  "range": "Sheet1!A1:D5",
  "majorDimension": "ROWS",
  "values": [
    ["Item", "Cost", "Stocked", "Ship Date"],
    ["Wheel", "$20.50", "4", "3/1/2016"],
    ["Door", "$15", "2", "3/15/2016"],
    ["Engine", "$100", "1", "30/20/2016"],
    ["Totals", "$135.5", "7", "3/20/2016"]
  ],
}
  */
  
class ValueRange {
public:
	ValueRange();
	virtual ~ValueRange();
	bool parse(const std::string &json);
	std::vector<std::vector<std::string>> values;
	std::string toString();
};

class GoogleSheets {
private:
	std::string sheet_id;
	std::string token;
public:
	GoogleSheets(
		const std::string sheetid,
		const std::string tokenbearer
	);
	virtual ~GoogleSheets();
	int getRange(const std::string &range, ValueRange &retval);
};

#endif /* GOOGLE_SHEETS_H_ */
