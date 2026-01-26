#ifndef INC_ntndArrayConverterPvxs_H
#define INC_ntndArrayConverterPvxs_H
#include <math.h>

#include <ntndArrayConverterAPI.h>
#include <pvxs/data.h>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include "ntndArrayConverterCommon.h"
using namespace pvxs;

class NTNDARRAYCONVERTER_API NTNDArrayConverterPvxs
{
public:
    NTNDArrayConverterPvxs(Value value);
    NTNDArrayInfo_t getInfo (void);
    void toArray (NDArray *dest);
    void fromArray (NDArray *src);

private:
    Value m_value;
    Value m_Int8Value;
    Value m_UInt8Value;
    Value m_Int16Value;
    Value m_UInt16Value;
    Value m_Int32Value;
    Value m_UInt32Value;
    Value m_Int64Value;
    Value m_UInt64Value;
    Value m_Float32Value;
    Value m_Float64Value;
    
    std::unordered_map<std::type_index, NDAttrDataType_t> m_typeMap;
    std::unordered_map<std::type_index, std::string> m_fieldNameMap;
    NDColorMode_t getColorMode (void);

    template <typename arrayType>
    void toValue (NDArray *dest);
    void toValue (NDArray *dest);

    void toDimensions (NDArray *dest);
    void toTimeStamp (NDArray *dest);
    void toDataTimeStamp (NDArray *dest);

    template <typename valueType>
    void toAttribute (NDArray *dest, Value attribute);
    void toStringAttribute (NDArray *dest, Value attribute);
    void toUndefinedAttribute (NDArray *dest, Value attribute);
    void toAttributes (NDArray *dest);

    template <typename arrayType>
    void fromValue (NDArray *src);
    void fromValue (NDArray *src);

    void fromDimensions (NDArray *src);
    void fromTimeStamp (NDArray *src);
    void fromDataTimeStamp (NDArray *src);

    template <typename valueType>
    void fromAttribute (Value destValue, Value tempValue, NDAttribute *src);
    void fromStringAttribute (Value destValue, NDAttribute *src);
    void fromAttributes (NDArray *src);
};

typedef std::shared_ptr<NTNDArrayConverterPvxs> NTNDArrayConverterPvxsPtr;

#endif // INC_ntndArrayConverterPvxs_H
