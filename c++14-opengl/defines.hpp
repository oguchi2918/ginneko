#ifndef INCLUDED_DEFINES_HPP
#define INCLUDED_DEFINES_HPP

#include <cstddef>

#define SAFE_DELETE(p) { delete (p); (p) = nullptr; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p); (p) = nullptr; }
#define OPERATOR_DELETE(p) { operator delete(p); (p) = nullptr; }

#define PI 3.141592653589793
#define TWOPI 6.2831853071795862
#define TO_RADIAND(x) (x * 0.017453292519943295)
#define TO_DEGREED(x) (x * 57.29577951308232)
#define TO_RADIANF(x) static_cast<float>(x * 0.017453292519943295)
#define TO_DEGREEF(x) static_cast<float>(x * 57.29577951308232)

#define BUFFER_OFFSET(bytes) (static_cast<unsigned char*>(nullptr) + (bytes))
#define BUFFER_OFFSETOF(type, field) BUFFER_OFFSET(offsetof(type, field))

#endif //INCLUDED_DEFINES_HPP
