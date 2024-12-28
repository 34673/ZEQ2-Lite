// Copyright (C) 2003-2005 RiO
//
// g_weapPhysScanner.c -- lexical scanner for ZEQ2's weapon physics script language.
#include "g_weapPhysParser.h" // <-- cg_local.h included in this
char* g_weapPhysCategories[] = {
	"Physics","Costs","Detonation","Muzzle","Trajectory","Requirement","Restrictions",""
};
g_weapPhysField_t g_weapPhysFields[] = {
	{"type",				G_weapPhys_ParseType			},	// Physics, Detonation, Trajectory
	{"speed",				G_weapPhys_ParseSpeed			},	// Physics
	{"acceleration",		G_weapPhys_ParseAcceleration	},	// Physics
	{"radius",				G_weapPhys_ParseRadius			},	// Physics, Detonation
	{"impede",				G_weapPhys_ParseImpede			},	// Detonation
	{"range",				G_weapPhys_ParseRange			},	// Physics, Trajectory
	{"duration",			G_weapPhys_ParseDuration		},	// Physics, Detonation
	{"lifetime",			G_weapPhys_ParseLifetime		},	// Physics
	{"swat",				G_weapPhys_ParseSwat			},	// Physics
	{"deflect",				G_weapPhys_ParseSwat			},	// Physics
	{"drain",				G_weapPhys_ParseDrain			},	// Physics
	{"blind",				G_weapPhys_ParseBlind			},	// Physics
	{"movement",			G_weapPhys_ParseMovement		},	// Restriction
	{"minPowerLevel",		G_weapPhys_ParseMinPowerLevel	},	// Requirement
	{"maxPowerLevel",		G_weapPhys_ParseMinPowerLevel	},	// Requirement
	{"minFatigue",			G_weapPhys_ParseMinFatigue		},	// Requirement
	{"maxFatigue",			G_weapPhys_ParseMaxFatigue		},	// Requirement
	{"minHealth",			G_weapPhys_ParseMinHealth		},	// Requirement
	{"maxHealth",			G_weapPhys_ParseMaxHealth		},	// Requirement
	{"minTier",				G_weapPhys_ParseMinTier			},	// Requirement
	{"maxTier",				G_weapPhys_ParseMaxTier			},	// Requirement
	{"minTotalTier",		G_weapPhys_ParseMinTotalTier	},	// Requirement
	{"maxTotalTier",		G_weapPhys_ParseMaxTotalTier	},	// Requirement
	{"ground",				G_weapPhys_ParseGround			},	// Requirement
	{"flight",				G_weapPhys_ParseFlight			},	// Requirement
	{"water",				G_weapPhys_ParseWater			},	// Requirement
	{"powerLevel",			G_weapPhys_ParsePowerLevel		},	// Costs
	{"maximum",				G_weapPhys_ParseMaximum			},	// Costs
	{"health",				G_weapPhys_ParseHealth			},	// Costs
	{"fatigue",				G_weapPhys_ParseFatigue			},	// Costs
	{"cooldownTime",		G_weapPhys_ParseCooldownTime	},	// Costs
	{"chargeTime",			G_weapPhys_ParseChargeTime		},	// Costs
	{"chargeReadyPct",		G_weapPhys_ParseChargeReadyPct	},	// Costs
	{"damage",				G_weapPhys_ParseDamage			},	// Detonation
	{"knockback",			G_weapPhys_ParseKnockBack		},	// Detonation
	{"nrShots",				G_weapPhys_ParseNrShots			},	// Muzzle
	{"offsetWidth",			G_weapPhys_ParseOffsetWidth		},	// Muzzle
	{"offsetHeight",		G_weapPhys_ParseOffsetHeight	},	// Muzzle
	{"angleWidth",			G_weapPhys_ParseAngleWidth		},	// Muzzle
	{"angleHeight",			G_weapPhys_ParseAngleHeight		},	// Muzzle
	{"flipInWidth",			G_weapPhys_ParseFlipInWidth		},	// Muzzle
	{"flipInHeight",		G_weapPhys_ParseFlipInHeight	},	// Muzzle
	{"fieldOfView",			G_weapPhys_ParseFOV				},	// Trajectory  --NOTE: Use for homing angles!
	{"FOV",					G_weapPhys_ParseFOV				},	// Trajectory  --NOTE: Use for homing angles!
	{"",					G_weapPhys_ParseDummy			}	// Terminator dummy function
};
g_weapPhysSyntax_t g_weapPhysSyntax[] = {
	{"=",TOKEN_EQUALS},
	{"+",TOKEN_PLUS},
	{"|",TOKEN_COLON},
	{"{",TOKEN_OPENBLOCK},
	{"}",TOKEN_CLOSEBLOCK},
	{"(",TOKEN_OPENVECTOR},
	{")",TOKEN_CLOSEVECTOR},
	{"[",TOKEN_OPENRANGE},
	{"]",TOKEN_CLOSERANGE},
	{"",-1}
};
g_weapPhysSyntax_t g_weapPhysSyntaxKeywords[] = {
	{"import",TOKEN_IMPORT},
	{"private",TOKEN_PRIVATE},
	{"protected",TOKEN_PROTECTED},
	{"public",TOKEN_PUBLIC},
	{"weapon",TOKEN_WEAPON},
	{"true",TOKEN_TRUE},
	{"false",TOKEN_FALSE},
	{"null",TOKEN_NULL},
	{"",-1}
};
/*
========================
G_weapPhys_ErrorHandle
========================
Sends feedback on script errors to the console.
*/
qboolean G_weapPhys_Error( g_weapPhysError_t errorNr, g_weapPhysScanner_t *scanner, char *string1, char *string2){
	char* file = scanner->filename;
	int line = scanner->line + 1; // <-- Internally we start from 0, for the user we start from 1
	if(errorNr == ERROR_FILE_NOTFOUND){G_Printf("^1%s: File not found.\n", file);}
	else if(errorNr == ERROR_FILE_TOOBIG){G_Printf("^1%s: File exceeds maximum script length.\n", file);}
	else if(errorNr == ERROR_PREMATURE_EOF){G_Printf("^1%s(%i): Premature end of file.\n", file, line);}
	else if(errorNr == ERROR_STRING_TOOBIG){G_Printf("^1%s(%i): String exceeds limit of %i characters.\n", file, line, MAX_TOKENSTRING_LENGTH);}
	else if(errorNr == ERROR_TOKEN_TOOBIG){G_Printf("^1%s(%i): Symbol exceeds limit of %i characters.\n", file, line, MAX_TOKENSTRING_LENGTH);}
	else if(errorNr == ERROR_UNKNOWN_SYMBOL){G_Printf("^1%s(%i): Unknown symbol '%s' encountered.\n", file, line, string1);}
	else if(errorNr == ERROR_UNEXPECTED_SYMBOL){
		if(!string2){G_Printf("^1%s(%i): Unexpected symbol '%s' found.\n", file, line, string1);}
		else{G_Printf("^1%s(%i): Unexpected symbol '%s' found, expected '%s'.\n", file, line, string1, string2);}
	}
	else if(errorNr == ERROR_STRING_EXPECTED){G_Printf("^1%s(%i): String expected. '%s' is not a string or is missing quotes.\n", file, line, string1);}
	else if(errorNr == ERROR_INTEGER_EXPECTED){G_Printf("^1%s(%i): Integer expected, but '%s' found.\n", file, line, string1);}
	else if(errorNr == ERROR_FLOAT_EXPECTED){G_Printf("^1%s(%i): Float or integer expected, but '%s' found.\n", file, line, string1);}
	else if(errorNr == ERROR_BOOLEAN_EXPECTED){G_Printf("^1%s(%i): Boolean expected, but '%s' found.\n", file, line, string1);}
	else if(errorNr == ERROR_IMPORTS_EXCEEDED){G_Printf("^1%s(%i): Trying to exceed maximum number of %i imports.\n", file, line, MAX_IMPORTS);}
	else if(errorNr == ERROR_IMPORT_REDEFINED){G_Printf("^1%s(%i): Trying to redefine previously defined import definition '%s'.\n", file, line, string1);}
	else if(errorNr == ERROR_IMPORT_DOUBLED){G_Printf("^1%s(%i): Trying to duplicate a previously imported definition under the new name '%s'.\n", file, line, string1);}
	else if(errorNr == ERROR_IMPORT_UNDEFINED){G_Printf("^1%s(%i): Undefined import '%s' being referenced.\n", file, line, string1);}
	else if(errorNr == ERROR_DEFINITIONS_EXCEEDED){G_Printf("^1%s(%i): Trying to exceed maximum number of %i weapon definitions.\n", file, line, MAX_DEFINES);}
	else if(errorNr == ERROR_DEFINITION_REDEFINED){G_Printf("^1%s(%i): Trying to redefine previously defined weapon definition '%s'.\n", file, line, string1);}
	else if(errorNr == ERROR_DEFINITION_UNDEFINED){G_Printf("^1%s(%i): Undefined weapon definition '%s' being referenced.\n", file, line, string1);}
	else if(errorNr == ERROR_REDEFINE_IMPORT_AS_DEFINITION){G_Printf("^1%s(%i): Trying to redefine previously defined import definition '%s' as a local weapon definition.\n", file, line, string1);}
	else if(errorNr == ERROR_LINK_BOUNDS){G_Printf("^1%s(%i): Weapon link out of bounds. Must be in range [1..6].\n", file, line);}
	else if(errorNr == ERROR_LINK_REDEFINED){G_Printf("^1%s(%i): Trying to redefine a previously defined weapon link number.\n", file, line);}
	else if(errorNr == ERROR_FIELD_NOT_IN_CATEGORY){G_Printf("^1%s(%i): Field '%s' is not supported by category '%s'.\n", file, line, string1, string2);}
	else if(errorNr == ERROR_INVERTED_RANGE){G_Printf("^1%s(%i): This range doesn't allow end value '%s' to be larger than start value '%s'.\n", file, line, string1, string2);}
	else if(errorNr == ERROR_OVER_MAXBOUND){G_Printf("^1%s(%i): Value '%s' is larger than maximum bound of %s.\n", file, line, string1, string2);}
	else if(errorNr == ERROR_UNDER_MINBOUND){G_Printf("^1%s(%i): Value '%s' is smaller than minimum bound of %s.\n", file, line, string1, string2);}
	else if(errorNr == ERROR_OVER_MAXVECTORELEMS){G_Printf("^1%s(%i): Element '%s' exceeds maximum storage capacity of %s elements in vector.\n", file, line, string1, string2);}
	else if(errorNr == ERROR_UNKNOWN_ENUMTYPE){G_Printf("^1%s(%i): Identifier '%s' is not a valid option in this enumeration type.", file, line, string1);}
	else if(errorNr == ERROR_MAX_RECURSION){G_Printf("^1%s: Compiler is trying to read beyond maximum recursion depth %i for overriding.\n", file, MAX_RECURSION_DEPTH);}
	else if(errorNr == ERROR_INHERITING_PRIVATE){G_Printf("^1%s: Private definition '%s' can not be inherited.\n", file, string1);}
	else if(errorNr == ERROR_IMPORTING_NON_PUBLIC){G_Printf("^1%s: Non-public definition '%s' can not be imported for local use.\n", file, string1);}
	else if(errorNr == ERROR_OVERRIDING_WITH_HIGHER_ACCESS){G_Printf("^1%s: Definition '%s' may not be declared with higher access than its superclass.\n", file, string1);}
	else{G_Printf("^1WEAPONSCRIPT ERROR: Unknown error occured.\n");}
	return qfalse;
}
/*
====================
G_weapPhys_NextSym
====================
Scans the next symbol in the scanner's
loaded scriptfile.
*/
qboolean G_weapPhys_NextSym(g_weapPhysScanner_t* scanner,g_weapPhysToken_t* token){
	int index = 0;
	int length = 0;
	char* start = NULL;
	//Skippables
	while(1){
		while(scanner->pos[0] <= ' '){
			if(!scanner->pos[0]){break;}
			if(scanner->pos[0] == '\n'){scanner->line += 1;}
			scanner->pos += 1;
		}
		if(scanner->pos[0] == '/' && scanner->pos[1] == '/'){
			char* newLine = strchr(scanner->pos,'\n');
			scanner->pos = newLine ? newLine : scanner->pos + 2;
		}
		else if(scanner->pos[0] == '/' && scanner->pos[1] == '*'){
			char* endComment = strstr(scanner->pos,"*/");
			scanner->pos = endComment ? endComment + 2 : scanner->pos + 2;
		}
		else{break;}
	}
	//Strings
	if(scanner->pos[0] == '\"'){
		char* endString = strchr(++scanner->pos,'\"');
		length = endString - scanner->pos;
		if(!endString){
			return G_weapPhys_Error(ERROR_PREMATURE_EOF,scanner,NULL,NULL);
		}
		if(length >= MAX_TOKENSTRING_LENGTH - 1){
			return G_weapPhys_Error(ERROR_STRING_TOOBIG,scanner,NULL,NULL);
		}
		Q_strncpyz(token->stringval,scanner->pos,length + 1);
		scanner->pos = endString + 1;
		return token->tokenSym = TOKEN_STRING;
	}
	//Numbers
	if(scanner->pos[0] >= '0' && scanner->pos[0] <= '9'){
		qboolean dot = qfalse;
		start = scanner->pos;
		length = 0;
		while(scanner->pos[0] >= '0' && scanner->pos[0] <= '9'){
			if(length >= MAX_TOKENSTRING_LENGTH-1){
				return G_weapPhys_Error(ERROR_TOKEN_TOOBIG,scanner,NULL,NULL);
			}
			length = ++scanner->pos - start;
			if(scanner->pos[0] != '.' || dot){continue;}
			if(length >= MAX_TOKENSTRING_LENGTH-1){
				return G_weapPhys_Error(ERROR_TOKEN_TOOBIG,scanner,NULL,NULL);
			}
			dot = qtrue;
			length = ++scanner->pos - start;
		}
		Q_strncpyz(token->stringval,start,length + 1);
		token->floatval = atof(token->stringval);
		token->intval = ceil(token->floatval);
		return token->tokenSym = dot ? TOKEN_FLOAT : TOKEN_INTEGER;
	}
	//Syntax symbols
	for(index=0;strcmp(g_weapPhysSyntax[index].symbol,"");++index){
		g_weapPhysSyntax_t* syntax = &g_weapPhysSyntax[index];
		if(scanner->pos[0] != syntax->symbol[0]){continue;}
		scanner->pos += 1;
		strcpy(token->stringval,syntax->symbol);
		return token->tokenSym = syntax->tokenType;
	}
	//Keywords
	start = scanner->pos;
	length = 0;
	while((scanner->pos[0] >= 'a' && scanner->pos[0] <= 'z') || (scanner->pos[0] >= 'A' && scanner->pos[0] <= 'Z')){
		if(length > MAX_TOKENSTRING_LENGTH-1){
			return G_weapPhys_Error(ERROR_TOKEN_TOOBIG,scanner,NULL,NULL);
		}
		length = ++scanner->pos - start;
	}
	Q_strncpyz(token->stringval,start,length + 1);
	for(index=0;strcmp(g_weapPhysSyntaxKeywords[index].symbol,"");++index){
		g_weapPhysSyntax_t* syntax = &g_weapPhysSyntaxKeywords[index];
		if(Q_stricmp(token->stringval,syntax->symbol)){continue;}
		return token->tokenSym = syntax->tokenType;
	}
	// Check if the keyword is a category.
	for ( index = 0; strcmp( g_weapPhysCategories[index], ""); index++){
		if(!Q_stricmp( g_weapPhysCategories[index], token->stringval)){
			token->identifierIndex = index;
			token->tokenSym = TOKEN_CATEGORY;
			return qtrue;
		}
	}
	// Check if the keyword is a field.
	for ( index = 0; strcmp( g_weapPhysFields[index].fieldname, ""); index++){
		// NOTE:
		// Need to do this comparison case independantly. Q_stricmp is case
		// independant, so use this instead of standard library function strcmp.
		if(!Q_stricmp( g_weapPhysFields[index].fieldname, token->stringval)){
			token->identifierIndex = index;
			token->tokenSym = TOKEN_FIELD;
			return qtrue;
		}
	}
	if(scanner->pos[0] == '\0'){
		return token->tokenSym = TOKEN_EOF;
	}
	return G_weapPhys_Error(ERROR_UNKNOWN_SYMBOL,scanner,token->stringval,NULL);
}
qboolean G_weapPhys_Scan(g_weapPhysScanner_t* scanner,g_weapPhysToken_t* token){
	if(G_weapPhys_NextSym(scanner,token)){return qtrue;}
	return token->tokenSym == TOKEN_EOF ? G_weapPhys_Error(ERROR_PREMATURE_EOF,scanner,NULL,NULL) : qfalse;
}
/*
=====================
G_weapPhys_LoadFile
=====================
Loads a new scriptfile into a scanner's memory.
*/
qboolean G_weapPhys_LoadFile( g_weapPhysScanner_t *scanner, char *filename){
	int			len;
	qhandle_t	file;
	Q_strncpyz( scanner->filename, filename, sizeof(scanner->filename));
	// Grab filehandle
	len = trap_FS_FOpenFile( filename, &file, FS_READ);
	// File must exist, else report error
	if(!file){
		G_weapPhys_Error( ERROR_FILE_NOTFOUND, scanner, filename, NULL);
		return qfalse;
	}
	// File must not be too big, else report error
	if(len >= ( sizeof(char) * MAX_SCRIPT_LENGTH - 1)){
		G_weapPhys_Error( ERROR_FILE_TOOBIG, scanner, NULL, NULL);
		trap_FS_FCloseFile( file);
		return qfalse;
	}
	// Read file
	trap_FS_Read( scanner->script, len, file);
	trap_FS_FCloseFile( file);
	// Ensure null termination
	scanner->script[len] = '\0';
	// Set starting position
	scanner->line = 0;
	scanner->pos = scanner->script;
	// Handle some verbose output
	if(g_verboseParse.integer){
		G_Printf("Scriptfile '%s' has been read into memory.\n", filename);
	}
	return qtrue;
}
