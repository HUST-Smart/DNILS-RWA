////////////////////////////////
/// usage : 1.	common type aliases.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef SMART_SZX_P_CENTER_COMMON_H
#define SMART_SZX_P_CENTER_COMMON_H

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#include <vector>
#include <set>
#include <map>
#include<hash_map>
#include<unordered_map>
#include <string>
#include <unordered_set>

namespace szx {

// zero-based consecutive integer identifier.
using ID = int;
// the unit of width and height.
using Length = int;
// the unit of x and y coordinates.
using Coord = Length;
// the unit of elapsed computational time.
using Duration = int;
// number of neighborhood moves in local search.
using Iteration = int;

using Wave = int;
// the No. of wavelength

template<typename T>
using List = std::vector<T>;

template<typename Key>
using HashSet = std::unordered_set<Key>;

template<typename T>
using Set = std::set<T>;

template<typename Key, typename Val>
using Map = std::map<Key, Val>;

using String = std::string;

enum 
{
	MaxDistance = (1 << 28),
	MyIntMin = (-20000000),
	EjectionChainLen = 5
};
enum NeighBorType
{
	NeighBorType_Exchange,
	NeighBorType_Shift
};
enum 
{
	InvalidID = -1
};
typedef enum {
	Interger,
	Linear
}ModelType;
typedef enum {
	MinOverLoad,
	MinConflictTra,
	MultiCommodity
}ModelObj;
class FileExtension {
public:
    static String protobuf() { return String(".pb"); }
    static String json() { return String(".json"); }
};

}


#endif // SMART_SZX_P_CENTER_COMMON_H
