#include <NDArray.h>
#include <pv/ntndarray.h>

class NTNDArrayConverter
{
public:
    NTNDArrayConverter(epics::nt::NTNDArrayPtr array);

    void toArray (NDArray *dest);
    void fromArray (NDArray *src);

private:
    epics::nt::NTNDArrayPtr m_array;

    template <typename arrayType>
    void toValue (NDArray *dest);
    void toValue (NDArray *dest);

    void toDimensions (NDArray *dest);
    void toTimeStamp (NDArray *dest);
    void toDataTimeStamp (NDArray *dest);

    template <typename pvAttrType, typename valueType>
    void toAttribute (NDArray *dest, epics::pvData::PVStructurePtr src);
    void toStringAttribute (NDArray *dest, epics::pvData::PVStructurePtr src);
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
    void createAttributes (NDArray *src);
    void fromAttributes (NDArray *src);
};

typedef std::tr1::shared_ptr<NTNDArrayConverter> NTNDArrayConverterPtr;
