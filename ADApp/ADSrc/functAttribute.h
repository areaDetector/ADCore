
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

typedef int (*NDAttributeFunction)(const char *functParam, class functAttribute *pAttribute);

/** Attribute that gets its value from a user-defined function
  * The updateValue() method for this class retrieves the current value from the function.
  */
class functAttribute : public NDAttribute {
public:
    functAttribute(const char *pName, const char *pDescription, const char *pSource, const char *pParam);
    ~functAttribute();
    virtual int updateValue();
    int report(FILE *fp, int details);

private:
    char *functParam;
    NDAttributeFunction pFunction;
};

#endif /*INCfunctAttributeH*/
