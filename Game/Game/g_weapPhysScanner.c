// Copyright (C) 2003-2005 RiO
//
// g_weapPhysScanner.c -- lexical scanner for ZEQ2's weapon physics script language.
#include "g_weapPhysParser.h" // <-- cg_local.h included in this
g_weapPhysField_t g_weapPhysPhysicsFields[] = {
	{"type",&g_weapPhysBuffer.general_type,G_weapPhys_ParseType,0,{"Missile","Beam","Hitscan","Trigger","None"}},
	{"speed",&g_weapPhysBuffer.physics_speed,G_weapPhys_ParseFloat},
	{"acceleration",&g_weapPhysBuffer.physics_acceleration,G_weapPhys_ParseFloat},
	{"radius",&g_weapPhysBuffer.physics_radius,G_weapPhys_ParseInt},
	{"radiusMultiplier",&g_weapPhysBuffer.physics_radiusMultiplier,G_weapPhys_ParseInt},
	{"range",g_weapPhysBuffer.physics_range,G_weapPhys_ParseRange},
	{"lifetime",&g_weapPhysBuffer.physics_lifetime,G_weapPhys_ParseInt},
	{"swat",&g_weapPhysBuffer.physics_swat,G_weapPhys_ParseInt},
	{"drain",&g_weapPhysBuffer.physics_drain,G_weapPhys_ParseInt},
	{"blind",&g_weapPhysBuffer.physics_blind,G_weapPhys_ParseInt},
	{"",NULL}
};
g_weapPhysField_t g_weapPhysCostsFields[] = {
	{"powerLevel",&g_weapPhysBuffer.costs_powerLevel,G_weapPhys_ParseInt},
	{"fatigue",&g_weapPhysBuffer.costs_fatigue,G_weapPhys_ParseInt},
	{"health",&g_weapPhysBuffer.costs_health,G_weapPhys_ParseInt},
	{"maximum",&g_weapPhysBuffer.costs_maximum,G_weapPhys_ParseInt},
	{"cooldownTime",&g_weapPhysBuffer.costs_cooldownTime,G_weapPhys_ParseInt},
	{"chargeTime",&g_weapPhysBuffer.costs_chargeTime,G_weapPhys_ParseInt},
	{"chargeReadyPct",&g_weapPhysBuffer.costs_chargeReady,G_weapPhys_ParseInt},
	{"",NULL}
};
g_weapPhysField_t g_weapPhysDetonationFields[] = {
	{"type",&g_weapPhysBuffer.general_type,G_weapPhys_ParseType,0,{"Ki"}}, //Keyword parsed, but no field exists for it?
	{"impede",&g_weapPhysBuffer.damage_impede,G_weapPhys_ParseInt},
	{"radius",&g_weapPhysBuffer.damage_radius,G_weapPhys_ParseInt},
	{"radiusMultiplier",&g_weapPhysBuffer.damage_radiusMultiplier,G_weapPhys_ParseInt},
	{"duration",&g_weapPhysBuffer.damage_radiusDuration,G_weapPhys_ParseInt},
	{"damage",&g_weapPhysBuffer.damage_damage,G_weapPhys_ParseInt},
	{"damageMultiplier",&g_weapPhysBuffer.damage_multiplier,G_weapPhys_ParseInt},
	{"knockback",&g_weapPhysBuffer.damage_extraKnockback,G_weapPhys_ParseInt},
	{"",NULL}
};
g_weapPhysField_t g_weapPhysMuzzleFields[] = {
	{"nrShots",&g_weapPhysBuffer.firing_nrShots,G_weapPhys_ParseInt},
	{"offsetWidth",&g_weapPhysBuffer.firing_offsetW,G_weapPhys_ParseRange},
	{"offsetHeight",&g_weapPhysBuffer.firing_offsetH,G_weapPhys_ParseRange},
	{"angleWidth",&g_weapPhysBuffer.firing_angleW,G_weapPhys_ParseRange},
	{"angleHeight",&g_weapPhysBuffer.firing_angleH,G_weapPhys_ParseRange},
	{"flipInWidth",&g_weapPhysBuffer.firing_offsetWFlip,G_weapPhys_ParseBool},
	{"flipInHeight",&g_weapPhysBuffer.firing_offsetHFlip,G_weapPhys_ParseBool},
	{"fieldOfView",&g_weapPhysBuffer.homing_FOV,G_weapPhys_ParseInt},
	{"FOV",&g_weapPhysBuffer.homing_FOV,G_weapPhys_ParseInt},
	{"",NULL}
};
g_weapPhysField_t g_weapPhysTrajectoryFields[] = {
	{"type",&g_weapPhysBuffer.homing_type,G_weapPhys_ParseType,0,{"LineOfSight","ProxBomb","Guided","Homing","Arch","Drunken","Cylinder"}},
	{"range",&g_weapPhysBuffer.homing_range,G_weapPhys_ParseInt},
	{"",NULL}
};
g_weapPhysField_t g_weapPhysRequirementFields[] = {
	{"minPowerLevel",&g_weapPhysBuffer.require_minPowerLevel,G_weapPhys_ParseInt},
	{"maxPowerLevel",&g_weapPhysBuffer.require_maxPowerLevel,G_weapPhys_ParseInt},
	{"minHealth",&g_weapPhysBuffer.require_minHealth,G_weapPhys_ParseInt},
	{"maxHealth",&g_weapPhysBuffer.require_maxHealth,G_weapPhys_ParseInt},
	{"minFatigue",&g_weapPhysBuffer.require_minFatigue,G_weapPhys_ParseInt},
	{"maxFatigue",&g_weapPhysBuffer.require_maxFatigue,G_weapPhys_ParseInt},
	{"minTier",&g_weapPhysBuffer.require_minTier,G_weapPhys_ParseInt},
	{"maxTier",&g_weapPhysBuffer.require_maxTier,G_weapPhys_ParseInt},
	{"minTotalTier",&g_weapPhysBuffer.require_minTotalTier,G_weapPhys_ParseInt},
	{"maxTotalTier",&g_weapPhysBuffer.require_maxTotalTier,G_weapPhys_ParseInt},
	{"ground",&g_weapPhysBuffer.require_ground,G_weapPhys_ParseInt},
	{"flight",&g_weapPhysBuffer.require_flight,G_weapPhys_ParseInt},
	{"water",&g_weapPhysBuffer.require_water,G_weapPhys_ParseInt},
	{"maximum",&g_weapPhysBuffer.require_maximum,G_weapPhys_ParseInt},
	{"",NULL}
};
g_weapPhysField_t g_weapPhysRestrictionsFields[] = {
	{"movement",&g_weapPhysBuffer.restrict_movement,G_weapPhys_ParseInt},
	{"",NULL}
};

g_weapPhysCategory_t g_weapPhysCategories[] = {
	{"Physics",g_weapPhysPhysicsFields},
	{"Costs",g_weapPhysCostsFields},
	{"Detonation",g_weapPhysDetonationFields},
	{"Muzzle",g_weapPhysMuzzleFields},
	{"Trajectory",g_weapPhysTrajectoryFields},
	{"Requirement",g_weapPhysRequirementFields},
	{"Restrictions",g_weapPhysRestrictionsFields},
	{"",NULL}
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
	int categoryIndex = 0;
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
	if((scanner->pos[0] >= '0' && scanner->pos[0] <= '9') || scanner->pos[0] == '-'){
		qboolean dot = qfalse;
		start = scanner->pos;
		length = 0;
		do{
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
		while(scanner->pos[0] >= '0' && scanner->pos[0] <= '9');
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
	for(;strcmp(g_weapPhysCategories[categoryIndex].name,"");++categoryIndex){
		g_weapPhysCategory_t* category = &g_weapPhysCategories[categoryIndex];
		if(Q_stricmp(token->stringval,category->name)){continue;}
		scanner->category = token->identifierIndex = categoryIndex;
		return token->tokenSym = TOKEN_CATEGORY;
	}
	if(scanner->category >= CAT_PHYSICS && scanner->category <= CAT_RESTRICT){
		g_weapPhysCategory_t* category = &g_weapPhysCategories[scanner->category];
		int fieldIndex = 0;
		for(;strcmp(category->fields[fieldIndex].name,"");++fieldIndex){
			g_weapPhysField_t* field = &category->fields[fieldIndex];
			if(Q_stricmp(token->stringval,field->name)){continue;}
			token->identifierIndex = fieldIndex;
			return token->tokenSym = TOKEN_FIELD;
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
