#pragma once

typedef unsigned long CFTypeID;
typedef signed long CFIndex;
typedef unsigned char Boolean;
typedef unsigned char UInt8;
typedef unsigned short UniChar;
typedef CFIndex CFNumberType;

typedef const struct __CFBoolean * CFBooleanRef;
typedef const struct __CFAllocator * CFAllocatorRef;
typedef const struct __CFDictionary * CFDictionaryRef;
typedef const struct __CFString * CFStringRef;
typedef const struct __CFArray * CFArrayRef;
typedef const struct __CFNumber * CFNumberRef;
typedef const struct __CFData * CFDataRef;
typedef const void * CFTypeRef;

enum
{
	kCFNumberSInt8Type = 1,
	kCFNumberSInt16Type = 2,
	kCFNumberSInt32Type = 3,
	kCFNumberSInt64Type = 4,
	kCFNumberFloat32Type = 5,
	kCFNumberFloat64Type = 6,
	kCFNumberCharType = 7,
	kCFNumberShortType = 8,
	kCFNumberIntType = 9,
	kCFNumberLongType = 10,
	kCFNumberLongLongType = 11,
	kCFNumberFloatType = 12,
	kCFNumberDoubleType = 13,
	kCFNumberCFIndexType = 14,
	kCFNumberNSIntegerType = 15,
	kCFNumberCGFloatType = 16,
	kCFNumberMaxType = 16
};

typedef struct
{
	CFIndex location;
	CFIndex length;
} CFRange;

inline CFRange CFRangeMake(CFIndex loc, CFIndex len)
{
	CFRange range;
	range.location = loc;
	range.length = len;
	return range;
}
