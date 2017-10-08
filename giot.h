
#ifndef GIOT_H
#define GIOT_H

#include <meta/meta.h>

using namespace baf;

namespace beamed3d {
	class Project;
}

namespace giot {

//////////////// Structures defining the inquieries that can be used ////////////////

using String = util::String;
using DateTime = String;

/// Structure to specify a time interval to retrieve data for.
struct TimeSpecification {
	DateTime mFrom;					///< Start date/time as a ISO8601 string: "2012-04-23T18:25:43.511Z". Leave blank to get just one sample.
	DateTime mTo;					///< Start date/time as a ISO8601 string. Leave blank to mean newest available data.
};


/// Structure to describe one GPS coordinate.
struct LongLat {
	double mLongitude = 0;			///< Longitude in WGS84 degrees, (east-west)
	double mLatitude = 0;			///< Latitude in WGS84 degrees, (north-south)
};


/// When doing a HTTP/Get to the main URL of the server the following data is returned, encoded as JSON.
struct InfoReply {
	String mOrganizationInfo;		///< String representation of the origanization responsible of the server. This should contain its name and contact information such as webpage URL, phone number etc. in human readable form.
	String mCopyRight;				///< Copyright notice for the data in human readable form.
	DateTime mOldest;				///< Timestamp of the oldest available data.
	DateTime mNewest;				///< Timestamp of the newest available data, or blank if sensor recording is ongoing.
	double mMaxForecast;			///< Time, in seconds, for which forecasts of sensor values are made. TimeSpecifications may reach this far into the future compared to mNewest.
	LongLat mSouthWest;				///< Corner of the area for which data can be supplied
	LongLat mNorthEast;				///< Other corner of the area, with larger mLongitude and mLatitude values.
	std::vector<String> mDatasets;	///< List of available data sets (gases, particle sizes, noise level, humidity, temperature etc) according to what connected sensors produce.
	String mDataURL;				///< The URL to send DataRequest packets to.
};


/// Abstract base of all types of geographical regions.
/// The JSON representation of all concrete subclasses has to contain a string field called type which contains the class name of
/// the actual data being sent, for instance type = "RectRegion".
struct RegionSpecification {	
	virtual ~RegionSpecification() {}

protected:
	RegionSpecification() = default;
};


/// The SensorRegion defintion reports the data at the positions of the actual sensors in the rectangle. This includes both fixed
/// and moving sensors (mounted on for instance vehicles). For moving sensors the reply will contain the individual longlat
/// positions for each sensor value if the cRawValues sampling interval is selected.
struct SensorRegion : public RegionSpecification {
	LongLat mSouthWest;					///< Corner of the area
	LongLat mNorthEast;					///< Other corner of the area, with larger mLongitude and mLatitude.
};	


/// Request data for a grid of points. The server is assumed to convert the longlat values to meters in an internal cartesian
/// coordinate system and then subsample this grid using the resolution hint. If the server uses a precalculated grid of points it
/// can fulfil this request by selecting a rectangular subset of its internal grid and then, if required subsample this to reach
/// the approximage resolution requested. The client must be prepared to handle replies with grid resolution from 1/2 to 2 times
/// the requested resolution. Note that the reply for each grid point contains the exact LongLat position for the point.
struct RectRegion : public SensorRegion {
	double mResolution = 100;					///< Approximate resolution in meters.
};

/// Region defining a set of points to retrieve data for.
struct PointRegion : public RegionSpecification {
	std::vector<LongLat> mPoints;		///< The points for which the request is made. Replies will contain the same longlat values.
};


/// Return SensorResult or MovingSensorResult for the sensor specified by Id
struct SensorIdRegion : public RegionSpecification {
	String mUniqueId;					///< Id as returned by a previous request using a SensorRegion.
};

/// The Statistics struct allows the client to specify some processing to be done before the data is returned.
/// The processing is done individually for each requested position / sensor.

/// If the Interval is set to cRawValues no processing is done, so mOperation is not used. This interval type can only be used for
/// the SensorRegion and SensorIdRegion. If cRawValues is combined with mGetAccuracies the raw sensor accuracy at each sample time
/// is reported. This is likely to be a long string of equal numbers but could vary for instance depending on time since last
/// calibration or environmental factors which affect the sensor accuracy, such as ambient temperature.

/// If Interval is not cRawValues an interpolation method is used to calculate a sample value at each sample time. Sample times are
/// aligned naturally so regardless of what mFrom value is set in the TimeSpecification the samples are produced for even hours,
/// minutes etc.

/// If the operation is not cNone a trend curve is created depending on the set interval. In this case a fixed number of data
/// values is produced which contain the result of subjecting the corresponding samples of each interval to the operation. For
/// instance if cHour interval is selected with cMax Operation the highest values of each hour will be produced for all days in the
/// specified time interval. Note that even if the TimeSpecification is shorter than the curve period the full number of bins will
/// be returned with some of the data values set to NaN (%%Check NaN representation in JSON, I think it is the string NAN).

struct Statistics {
	enum Interval {
		cRawValues,						///< Only applicable with SensorRegion. Returns individual readings.
		cTenMinutes,					///< One value per 10 minute interval. Daily period (168 bins)
		cHour,							///< One value per hour of the day. Daily period (24 bins)
		cDayOfWeek,						///< One value per day of the week. Weekly period (7 bins)
		cWeek,							///< One value per week. Yearly period (53 bins)
		cMonth,							///< One value per month. Yearly period (12 bins)
		cYear,							///< One value per year. No period.
	};

	enum Operation {					///< Type of property reported for the data in each bin.
		cNone,							///< No periodic interval, return as many samples as required to fulfill the time interval.
		cMean,							///< Calculate the mean of input samples.
		cStdDev,						///< Calculate the standard deviation of input samples.
		cMax,							///< Take the largest sample value.
		cMin							///< Take the smallest sample value.
	};

	Interval mInterval = cHour;			///< Interval between samples.
	Operation mOperation = cNone;		///< Operation to use to create the trend curve.
	bool mGetAccuracies = false;		///< Return estimated accuracies instead of the data. The same operations and intervals apply. Note that an overall accuracy estimate is always returned for each request.
};


/// All requests are represented by a DataRequest object.
struct DataRequest {
	TimeSpecification mTimeInterval;	///< Time interval to retrieve data for.
	std::unique_ptr<RegionSpecification> mRegion;		///< Region to retrieve data for.
	String mDataset;					///< Which type of measurements to return values for. Predefined string values must be defined such as CO2, NOx, PM2.5, etc.
	Statistics mStatistics;				///< What type of postprocessing/statistics to perform.
};


/// As the returned data gets fairly large it may be wise to subdivide it into one JSON data structure per geographical position.
/// This also offers the possibility to report the LongLat of each point in a header as well as for instance a distance to the
/// closest actual sensor used and the number of sensors partaking in the calculation of a value at the specific position. For the
/// SensorRegion case more metadata of each sensor (which is now one JSON structure) can be sent.

/// DataReply is returned when the region is a PointRegion or RectRegion. It contains calculated data for a certain
/// point. It is also the baseclass of the other data types in which case the mData member contains the values for the sensor.
struct DataReply {
	virtual ~DataReply() {}

	LongLat mPosition;				///< The point sent in for PointRegion (with some roundoff error allowe?), the actual position of the point for RectRegion and the sensor position for SensorRegion and SensorIdRegion.
	double mAccuracy;				///< Accuracy estimate for the data set.
	std::vector<double> mData;		///< The vector contains the values at each bin/time.
};


/// SensorDataReply is returned when a SensorRegion or SensorIdRegion is specified and contains info about the Sensor.
struct SensorDataReply : public DataReply {
	DateTime mInstallationDate;		///< Date of introduction into the sensor network.
	DateTime mExstallationDate;		///< Date of removal from the sensor network. Blank if the sensor is still active.
	DateTime mCalibrationDate;		///< CalibrationDate reports the newest calibration date inside the requested time interval.
	String mDataSet;				///< Measurement data type this sensor produces.
	String mManufacturer;			///< Sensor manufacturer. If the sensor unit has multiple manufacturers these may be listed comma separated.
	String mModel;					///< Sensor model identification
	String mSerial;					///< Sensor serial number
	String mUniqueId;				///< System wide unique id of the sensor. This can be used in a subsequent SensorIdRegion request.
	double mSampleTime;				///< Sensor sampling time in seconds.
};


/// MovingSensorReply is returned for moving sensors when cRawValues Interval and a SensorIdRegion for
/// the sensor is requested.
struct MovingSensorReply : public SensorDataReply {
	std::vector<LongLat> mPositions;			///< The mPositions vector is parallel to the mData vector in the base class and reports the sensor's position for each sample time (cRawValues only).
};


/// DataReplyContainer is the total reply from one request. It could contain some statistics on the processing done etc. also.
struct DataReplyContainer {
	static const int cSuccess = 0;				///< Ok. mReplies is filled with valid data
	static const int cSyntaxError = 1;			///< The request could not be interpreted
	static const int cInvalidRequest = 2;		///< The request contained an invalid combination such as cRawValues with non-sensor region type.
	static const int cRangeError = 3;			///< The request contains values which are erroneous compared to what the server can offer, e.g. times before measurement started or longlats outside available data.
	static const int cServerError = 4;			///< The server is not working correctly now, try again.

	int mStatus;								///< One of the integer constants.
	String mMessage;							///< Specific error message, or "Success" if status is cSuccess.
	std::vector<DataReply*> mReplies;			///< vector of replies in row major order (x coordinate varies fastest).
};


/// Test code
void FillGIoTRequest(DataRequest& request);
void SaveGIoTRequest(beamed3d::Project& project, const String& filename, DataRequest& request);
String SendGIoTRequest(beamed3d::Project& project, DataReplyContainer& reply, DataRequest& request, const String& url, const String& user, const String& password, bool trace);
void CreateGIoTRequest(beamed3d::Project& project);

void FillGIoTReply(DataReplyContainer& reply);
void CreateGIoTReply();

}

#endif
