#include <NDArray.h>
#include <pv/ntndarray.h>

typedef struct NTNDArrayInfo
{
    int ndims;
    size_t dims[ND_ARRAY_MAX_DIMS];
    size_t nElements, totalBytes;
    int bytesPerElement;
    NDColorMode_t colorMode;
    NDDataType_t dataType;

    struct
    {
        int dim;
        size_t size, stride;
    }x, y, color;
}NTNDArrayInfo_t;

class epicsShareClass NTNDArrayConverter
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
