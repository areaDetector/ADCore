#ifndef INC_ntndArrayConverter_H
#define INC_ntndArrayConverter_H
#include <math.h>

#include <ntndArrayConverterAPI.h>
#include <pv/ntndarray.h>
#include "ntndArrayConverterCommon.h"

class NTNDARRAYCONVERTER_API NTNDArrayConverter
{
public:
    NTNDArrayConverter(epics::nt::NTNDArrayPtr array);

    NTNDArrayInfo_t getInfo (void);
    void toArray (NDArray *dest);
    void fromArray (NDArray *src);

private:
    epics::nt::NTNDArrayPtr m_array;

    epics::pvData::ScalarType getValueType (void);
    NDColorMode_t getColorMode (void);

    template <typename arrayType>
    void toValue (NDArray *dest);
    void toValue (NDArray *dest);

    void toDimensions (NDArray *dest);
    void toTimeStamp (NDArray *dest);
    void toDataTimeStamp (NDArray *dest);

    template <typename pvAttrType, typename valueType>
    void toAttribute (NDArray *dest, epics::pvData::PVStructurePtr src);
    void toStringAttribute (NDArray *dest, epics::pvData::PVStructurePtr src);
    void toUndefinedAttribute (NDArray *dest, epics::pvData::PVStructurePtr src);
    void toAttributes (NDArray *dest);

    template <typename arrayType, typename srcDataType>
    void fromValue (NDArray *src);
    void fromValue (NDArray *src);

    void fromDimensions (NDArray *src);
    void fromTimeStamp (NDArray *src);
    void fromDataTimeStamp (NDArray *src);

    template <typename pvAttrType, typename valueType>
    void fromAttribute (epics::pvData::PVStructurePtr dest, NDAttribute *src);
    void fromStringAttribute (epics::pvData::PVStructurePtr dest, NDAttribute *src);
    void fromUndefinedAttribute (epics::pvData::PVStructurePtr dest);
    void fromAttributes (NDArray *src);
};

typedef std::tr1::shared_ptr<NTNDArrayConverter> NTNDArrayConverterPtr;

#endif // INC_ntndArrayConverter_H