
/* functAttribute.h
 *
 * \author Mark Rivers
 *
 * \author University of Chicago
 *
 * \date October 12, 2013
 *
 */
#ifndef INCfunctAttributeH
#define INCfunctAttributeH

#include "NDArray.h"

typedef int (*NDAttributeFunction)(const char *functParam, void **functionPvt, class functAttribute *pAttribute);

/** Attribute that gets its value from a user-defined function
  * The updateValue() method for this class retrieves the current value from the function.
  */
class functAttribute : public NDAttribute {
public:
    functAttribute(const char *pName, const char *pDescription, const char *pSource, const char *pParam);
    functAttribute(functAttribute& attribute);
    ~functAttribute();
    functAttribute* copy(NDAttribute *pAttribute);
    virtual int updateValue();
    int report(FILE *fp, int details);

private:
    char *functParam;
    NDAttributeFunction pFunction;
    void *functionPvt;
};

#endif /*INCfunctAttributeH*/
