#ifndef MSVCFIX_HPP
#define MSVCFIX_HPP

// Workarounds for boneheaded things that Visual Studio does
#ifdef _MSC_VER
	// Include standard math constants. Standard as in every compiler except
	// Microsoft's giving them by default.
#define _USE_MATH_DEFINES

#define strcasecmp stricmp

	// Misguided warnings section
#pragma warning (disable: 4305)     // truncation from 'double' to 'float'
#pragma warning (disable: 4244)     // conversion from 'double' to 'const float' (familiar?)
#pragma warning (disable: 4267)     // conversion from 'size_t' to 'int', possible loss of data
#pragma warning (disable: 4800)     // forcing value to bool 'true' or 'false' (performance warning)

#ifndef vsnprintf	// Standard function with non-standard name
#	define vsnprintf _vsnprintf
#endif

#endif

#ifdef _WIN32
#undef SendMessage      // Nasty namespace pollution with macros

#endif

#endif
