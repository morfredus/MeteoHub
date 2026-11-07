#include "day_file_format.h"

namespace mhhist {

FileHeader makeHeader(bool withFlags) {
    FileHeader h{};
    h.magic[0] = 'M'; h.magic[1] = 'T'; h.magic[2] = 'H'; h.magic[3] = 'B';
    h.version = withFlags ? kFormatV2 : kFormatV1;
    h.headerSize = sizeof(FileHeader);
    h.recordSize = withFlags ? sizeof(BinRecordV2) : sizeof(BinRecord);
    h.sensorFlags = kSensorTemp | kSensorHum | kSensorPres;
    h.recordCount = 0; h.firstTimestamp = 0; h.lastTimestamp = 0;
    return h;
}

} // namespace mhhist
