#include <NDPluginStats.h>

void test_forward_reference()
{
// Creating the object as an automatic variable causes a compilation error on Linux but compiles fine on Windows
  NDPluginStats stats = NDPluginStats("STATS1", 20, 0, "SIM1", 0, 0, 0, 0, 0, 1);

// Creating the object as a heap variable works fine on Linux and on Windows
  NDPluginStats *pStats = new NDPluginStats("STATS1", 20, 0, "SIM1", 0, 0, 0, 0, 0, 1);
}
