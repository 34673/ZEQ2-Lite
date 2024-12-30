// Copyright (C) 2003-2005 RiO
//
// g_weapPhysParser.h -- Contains information for the parser, scanner and attribute evaluator
//                       of ZEQ2's weapon physics script language.
#include "g_local.h"
// -< General settings >--
#define MAX_IMPORTS	 16 // 2^4: Won't really need much of these
#define MAX_DEFINES  64	// 2^6: Made this quite large to facilitate 'library' files
#define MAX_LINKS	  6 // We only have 6 weapon definitions, after all.
#define MAX_RECURSION_DEPTH		   5
#define MAX_SCRIPT_LENGTH		8192		// 2^13
#define MAX_TOKENSTRING_LENGTH	MAX_QPATH	// Equal to MAX_QPATH, to prevent problems with reading filenames.
#define MAX_TYPES 8							// NOTE: The sequences of the strings in these stringtables are dependant on the
											//       sequences of enumeration types in g_userweapons.h !!!
// --< Tokens >--
#define TOKEN_EOF				   0	// End of File
#define TOKEN_OPENBLOCK			   1	// '{'
#define TOKEN_CLOSEBLOCK		   2	// '}'
#define TOKEN_OPENVECTOR		   3	// '('
#define TOKEN_CLOSEVECTOR		   4	// ')'
#define TOKEN_OPENRANGE			   5	// '['
#define TOKEN_CLOSERANGE		   6	// ']'
#define TOKEN_IMPORT			   7	// 'import'
#define TOKEN_PUBLIC			   8	// 'public'
#define TOKEN_PROTECTED			   9	// 'protected'
#define TOKEN_PRIVATE			  10	// 'private'
#define TOKEN_EQUALS			  11	// '='
#define TOKEN_PLUS				  12	// '+'
#define TOKEN_COLON				  13	// '|'
#define TOKEN_TRUE				  14	// 'true'
#define TOKEN_FALSE				  15	// 'false'
#define TOKEN_NULL				  16	// 'null'
#define TOKEN_INTEGER			  17	// <integer number>
#define TOKEN_FLOAT				  18	// <real number>
#define TOKEN_STRING			  19	// "<alphanumeric characters>"
#define TOKEN_FIELD				  20	// <field name>
#define TOKEN_CATEGORY			  21	// <category name>
#define TOKEN_WEAPON			  22	// 'weapon'
// --< Error IDs >--
typedef enum {
	ERROR_FILE_NOTFOUND,
	ERROR_FILE_TOOBIG,
	ERROR_PREMATURE_EOF,
	ERROR_STRING_TOOBIG,
	ERROR_TOKEN_TOOBIG,
	ERROR_UNKNOWN_SYMBOL,
	ERROR_UNEXPECTED_SYMBOL,
	ERROR_STRING_EXPECTED,
	ERROR_INTEGER_EXPECTED,
	ERROR_FLOAT_EXPECTED,
	ERROR_BOOLEAN_EXPECTED,
	ERROR_IMPORTS_EXCEEDED,
	ERROR_IMPORT_REDEFINED,
	ERROR_IMPORT_DOUBLED,
	ERROR_IMPORT_UNDEFINED,
	ERROR_REDEFINE_IMPORT_AS_DEFINITION,
	ERROR_DEFINITIONS_EXCEEDED,
	ERROR_DEFINITION_REDEFINED,
	ERROR_DEFINITION_UNDEFINED,
	ERROR_LINK_BOUNDS,
	ERROR_LINK_REDEFINED,
	ERROR_FIELD_NOT_IN_CATEGORY,
	ERROR_INVERTED_RANGE,
	ERROR_OVER_MAXBOUND,
	ERROR_UNDER_MINBOUND,
	ERROR_OVER_MAXVECTORELEMS,
	ERROR_UNKNOWN_ENUMTYPE,
	ERROR_MAX_RECURSION,
	ERROR_INHERITING_PRIVATE,
	ERROR_IMPORTING_NON_PUBLIC,
	ERROR_OVERRIDING_WITH_HIGHER_ACCESS
} g_weapPhysError_t;
typedef enum{
	CAT_PHYSICS,
	CAT_COSTS,
	CAT_DETONATION,
	CAT_MUZZLE,
	CAT_TRAJECTORY,
	CAT_REQUIREMENT,
	CAT_RESTRICT
}g_weapPhysCategoryIndex_t;
// --< Storage structures >--
typedef struct{
	int tokenSym;
	int identifierIndex;
	int intval;
	float floatval;
	char stringval[MAX_TOKENSTRING_LENGTH];
}g_weapPhysToken_t;
typedef struct{
	char script[MAX_SCRIPT_LENGTH];
	g_weapPhysCategoryIndex_t category;
	int line;
	char* pos;
	char filename[MAX_QPATH];
}g_weapPhysScanner_t;
typedef struct{
	char refname[MAX_TOKENSTRING_LENGTH];
	char filename[MAX_TOKENSTRING_LENGTH];
	char defname[MAX_TOKENSTRING_LENGTH];
	qboolean active;
}g_weapPhysImportRef_t;
typedef enum{
	LVL_PUBLIC,
	LVL_PROTECTED,
	LVL_PRIVATE
}g_weapPhysAccessLvls_t;
typedef struct{
	char refname[MAX_TOKENSTRING_LENGTH];
	char supername[MAX_TOKENSTRING_LENGTH];
	char* scannerPos;
	int scannerLine;
	g_weapPhysAccessLvls_t accessLvl;
	qboolean hasSuper;
	qboolean active;
}g_weapPhysDefinitionRef_t;
typedef struct{
	char pri_refname[MAX_TOKENSTRING_LENGTH];
	char sec_refname[MAX_TOKENSTRING_LENGTH];
	qboolean active;
}g_weapPhysLinkRef_t;
typedef struct{
	g_weapPhysScanner_t scanner;
	g_weapPhysToken_t token;
	g_weapPhysImportRef_t importRef[MAX_IMPORTS];
	g_weapPhysDefinitionRef_t definitionRef[MAX_DEFINES];
	g_weapPhysLinkRef_t linkRef[MAX_LINKS];
}g_weapPhysParser_t;
//Prototype these, so definition of G_weapPhysFields doesn'tAlr complain
qboolean G_weapPhys_ParseDummy(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]);
qboolean G_weapPhys_ParseInt(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]);
qboolean G_weapPhys_ParseFloat(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]);
qboolean G_weapPhys_ParseBool(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]);
qboolean G_weapPhys_ParseRange(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]);
qboolean G_weapPhys_ParseType(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]);
typedef struct{
	char* name;
	void* field;
	qboolean (*Parse)(g_weapPhysParser_t*,void*,int,char*[]);
	int listIterations;
	// NOTE: The sequences of the strings in these stringtables are dependant on the
	//       sequences of enumeration types in g_userweapons.h !!!
	char* types[MAX_TYPES];
}g_weapPhysField_t;
typedef struct{
	char* name;
	g_weapPhysField_t* fields;
}g_weapPhysCategory_t;
typedef struct{
	char* symbol;
	int tokenType;
}g_weapPhysSyntax_t;
// --< Shared variables (located in G_weapPhysScanner.c) >--
extern g_userWeaponParseBuffer_t g_weapPhysBuffer;
extern g_weapPhysCategory_t g_weapPhysCategories[];
// --< Accesible functions >--
// -Lexical Scanner-
qboolean G_weapPhys_NextSym(g_weapPhysScanner_t* scanner,g_weapPhysToken_t* token);
qboolean G_weapPhys_Scan(g_weapPhysScanner_t* scanner,g_weapPhysToken_t* token);
qboolean G_weapPhys_LoadFile(g_weapPhysScanner_t* scanner,char* filename);
// -Token Parser-
qboolean G_weapPhys_Error(g_weapPhysError_t errorNr,g_weapPhysScanner_t* scanner,char* string1,char* string2);
// -Attribute Evaluator-
int G_weapPhys_FindImportRef(g_weapPhysParser_t* parser,char* refname);
int G_weapPhys_FindDefinitionRef(g_weapPhysParser_t* parser,char* refname);
qboolean G_weapPhys_AddImportRef(g_weapPhysParser_t* parser,char* refname,char* filename,char* defname);
qboolean G_weapPhys_AddDefinitionRef(g_weapPhysParser_t* parser,char* refname,char* pos,int line,g_weapPhysAccessLvls_t accessLvl,qboolean hasSuper,char* supername);
qboolean G_weapPhys_AddLinkRef(g_weapPhysParser_t* parser,int index,char* pri_refname,char* sec_refname);
