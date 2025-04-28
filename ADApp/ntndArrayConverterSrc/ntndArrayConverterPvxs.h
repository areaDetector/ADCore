#include <math.h>

#include <ntndArrayConverterAPI.h>
#include <NDArray.h>
#include <pvxs/data.h>
#include <pvxs/nt.h>

typedef struct NTNDArrayInfo
{
    int ndims;
    size_t dims[ND_ARRAY_MAX_DIMS];
    size_t nElements, totalBytes;
    int bytesPerElement;
    NDColorMode_t colorMode;
    NDDataType_t dataType;
    std::string codec;

    struct
    {
        int dim;
        size_t size, stride;
    }x, y, color;
}NTNDArrayInfo_t;

class NTNDARRAYCONVERTER_API NTNDArrayConverter
{
public:
    NTNDArrayConverter(pvxs::Value value);
    NTNDArrayInfo_t getInfo (void);
    void toArray (NDArray *dest);
    void fromArray (NDArray *src);

private:
    pvxs::Value m_value;
    NDColorMode_t getColorMode (void);

    template <typename arrayType>
    void toValue (NDArray *dest, std::string fieldName);
    void toValue (NDArray *dest);

    void toDimensions (NDArray *dest);
    void toTimeStamp (NDArray *dest);
    void toDataTimeStamp (NDArray *dest);

    template <typename valueType>
    void toAttribute (NDArray *dest, pvxs::Value attribute, NDAttrDataType_t dataType);
    void toStringAttribute (NDArray *dest, pvxs::Value attribute);
    // void toUndefinedAttribute (NDArray *dest, epics::pvData::PVStructurePtr src);
    void toAttributes (NDArray *dest);

    // template <typename arrayType>
    // void fromValue (NDArray *src, const char* field_name);
    void fromValue (NDArray *src);
    
    void fromDimensions (NDArray *src);
    void fromTimeStamp (NDArray *src);
    void fromDataTimeStamp (NDArray *src);

    template <typename valueType>
    void fromAttribute (pvxs::Value dest_value, NDAttribute *src);
    void fromStringAttribute (pvxs::Value dest_value, NDAttribute *src);
    // void fromUndefinedAttribute (epics::pvData::PVStructurePtr dest);
    void fromAttributes (NDArray *src);
};

typedef std::shared_ptr<NTNDArrayConverter> NTNDArrayConverterPtr;
