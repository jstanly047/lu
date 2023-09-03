#include <soci/soci.h>

int main()
{
    char const* const connectString = "mysql://db=lu_test user=stanly password='rjs047'";

    soci::session sql();
}