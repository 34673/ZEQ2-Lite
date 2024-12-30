// Copyright (C) 2003-2005 RiO
//
// g_weapGfxParser.c -- token parser for ZEQ2's weapon physics script language.
#include "g_weapPhysParser.h" // <-- cg_local.h included in this
static int g_weapPhysRecursionDepth;
g_userWeaponParseBuffer_t g_weapPhysBuffer;
	// FIXME: Can this be a local variable instead, or would it give us
	//		  > 32k locals errors in the VM-bytecode compiler?
/*
=======================
G_weapPhys_StoreBuffer
=======================
Copies the contents in the buffer to a client's weapon
configuration, converting filestrings into qhandle_t in
the process.
*/
void G_weapPhys_StoreBuffer(int clientNum, int weaponNum){
	g_userWeapon_t				*dest;
	g_userWeaponParseBuffer_t	*src;
	src = &g_weapPhysBuffer;
	dest = G_FindUserWeaponData(clientNum, weaponNum + 1);
	memset(dest, 0, sizeof(g_userWeapon_t));
	// Size and form of buffer and storage is equal, so just
	// copy the memory.
	memcpy(dest, src, sizeof(g_userWeapon_t));
	// Indicate the weapon as charged if there is a chargeReadyPct
	// or chargeTime set to something other than 0.
	if(src->costs_chargeReady || src->costs_chargeTime){
		dest->general_bitflags |= WPF_NEEDSCHARGE;
	}
	// If nrShots is set to < 1, internally it must be represented by 1 instead.
	if(dest->firing_nrShots < 1) dest->firing_nrShots = 1;
	// Handle the presence of alternate fire weapons through the
	// piggybacked WPF_ALTWEAPONPRESENT flag.	// NOTE: The WPF_ALTWEAPONPRESENT flag will only be enabled in src if src is the
	//       buffer for a secondary fire configuration. In this case it is implicitly
	//       safe to subtract ALTWEAPON_OFFSET to obtain the primary fire index
	//       of the weapon!
	if(src->general_bitflags & WPF_ALTWEAPONPRESENT){
		src->general_bitflags &= ~WPF_ALTWEAPONPRESENT;
		dest = G_FindUserWeaponData(clientNum, weaponNum - ALTWEAPON_OFFSET + 1);
		dest->general_bitflags |= WPF_ALTWEAPONPRESENT;
	}
}
//=========================================
// A special dummy function used in the terminator of
// the g_weapPhysFields list. Never actually used, and
// if it _would_ be called, it would always hault
// parsing.
//=========================================
qboolean G_weapPhys_ParseDummy(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]){return qfalse;}
qboolean G_weapPhys_ParseInt(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]){
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	if(token->tokenSym != TOKEN_INTEGER && token->tokenSym != TOKEN_FLOAT){
		return G_weapPhys_Error(ERROR_FLOAT_EXPECTED,scanner,token->stringval,NULL);
	}
	*(int*)field = token->tokenSym == TOKEN_INTEGER ? token->intval : token->floatval;
	return qtrue;
}
qboolean G_weapPhys_ParseFloat(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]){
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	if(token->tokenSym != TOKEN_INTEGER && token->tokenSym != TOKEN_FLOAT){
		return G_weapPhys_Error(ERROR_FLOAT_EXPECTED,scanner,token->stringval,NULL);
	}
	*(float*)field = token->tokenSym == TOKEN_INTEGER ? token->intval : token->floatval;
	return qtrue;
}
qboolean G_weapPhys_ParseBool(g_weapPhysParser_t *parser,void* field,int listLimit,char* types[]){
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	if(token->tokenSym != TOKEN_TRUE && token->tokenSym != TOKEN_FALSE){
		return G_weapPhys_Error(ERROR_BOOLEAN_EXPECTED,scanner,token->stringval,NULL);
	}
	*(qboolean*)field = token->tokenSym == TOKEN_TRUE;
	return qtrue;
}
qboolean G_weapPhys_ParseRange(g_weapPhysParser_t* parser,void* field,int listLimit,char* types[]){
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	float* start = field;
	float* end = start + 1;
	if(token->tokenSym == TOKEN_INTEGER || token->tokenSym == TOKEN_FLOAT){
		qboolean result = G_weapPhys_ParseFloat(parser,start,listLimit,NULL);
		*end = *start;
		return result;
	}
	if(token->tokenSym != TOKEN_OPENRANGE){
		return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,"[");
	}
	if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
	G_weapPhys_ParseFloat(parser,start,listLimit,NULL);
	if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
	G_weapPhys_ParseFloat(parser,end,listLimit,NULL);
	if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
	if(token->tokenSym != TOKEN_CLOSERANGE){
		return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,"]");
	}
	return qtrue;
}
qboolean G_weapPhys_ParseType(g_weapPhysParser_t *parser,void* field,int listLimit,char* types[]){
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	qboolean found = qfalse;
	int index = 0;
	if(token->tokenSym != TOKEN_STRING){
		G_weapPhys_Error(ERROR_STRING_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	for(;index < MAX_TYPES;index++){
		if(!types[index] || Q_stricmp(types[index],token->stringval)){continue;}
		found = qtrue;
		break;
	}
	if(!found){return G_weapPhys_Error(ERROR_UNKNOWN_ENUMTYPE,scanner,token->stringval,NULL);}
	*(int*)field = index;
	return qtrue;
}
//============================================
// FIXED FUNCTIONS - Do not change the below.
//============================================
//Parses import definitions and stores them in a reference list.
//Syntax: 'import' "<refname>" '=' "<filename>" "<defname>"
static qboolean G_weapPhys_ParseImports(g_weapPhysParser_t* parser){
	g_weapPhysToken_t* token = &parser->token;
	g_weapPhysScanner_t* scanner = &parser->scanner;
	char refname[MAX_TOKENSTRING_LENGTH];
	char filename[MAX_TOKENSTRING_LENGTH];
	while(token->tokenSym == TOKEN_IMPORT){
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_STRING){
			return G_weapPhys_Error(ERROR_STRING_EXPECTED,scanner,token->stringval,NULL);
		}
		else{
			Q_strncpyz(refname,token->stringval,sizeof(refname));
		}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_EQUALS){
			return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,"=");
		}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_STRING){
			return G_weapPhys_Error(ERROR_STRING_EXPECTED,scanner,token->stringval,NULL);
		}
		else{
			Q_strncpyz(filename,token->stringval,sizeof(filename));
		}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_STRING){
			return G_weapPhys_Error(ERROR_STRING_EXPECTED,scanner,token->stringval,NULL);
		}
		if(!G_weapPhys_AddImportRef(parser,refname,filename,token->stringval)){
			return qfalse;
		}
		// NOTE: Do it like this to prevent errors if a file happens to only contain
		//       import lines. While it is actually useless to have such a file, it
		//       still is syntacticly correct.
		if(!G_weapPhys_NextSym(scanner, token) && token->tokenSym != TOKEN_EOF){
			return qfalse;
		}
	}
	return qtrue;
}
//Parses the fields of one category within a weapon definition.
static qboolean G_weapPhys_ParseFields(g_weapPhysParser_t *parser){
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	g_userWeaponParseBuffer_t* buffer = &g_weapPhysBuffer;
	while(token->tokenSym == TOKEN_FIELD){
		int fieldIndex = token->identifierIndex;
		g_weapPhysCategory_t* category = &g_weapPhysCategories[scanner->category];
		g_weapPhysField_t* field = &category->fields[fieldIndex];
		if(!Q_stricmp(field->name,"")){
			return G_weapPhys_Error(ERROR_FIELD_NOT_IN_CATEGORY,scanner,token->stringval,category->name);
		}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_EQUALS){
			return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,"=");
		}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(!field->Parse(parser,field->field,field->listIterations,field->types)){return qfalse;}
		if(field->field == &buffer->require_minTier){buffer->require_minTier += 1;}
		if(field->field == &buffer->require_maxTier){buffer->require_maxTier += 1;}
		if(field->field == &buffer->require_minTotalTier){buffer->require_minTotalTier += 1;}
		if(field->field == &buffer->require_maxTotalTier){buffer->require_maxTotalTier += 1;}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}	
	}
	return qtrue;
}
//Parses the categories of a weapon definition.
// Syntax: <categoryname> '{' <HANDLE FIELDS> '}'
static qboolean G_weapPhys_ParseCategories(g_weapPhysParser_t* parser){
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	while(token->tokenSym == TOKEN_CATEGORY){
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_OPENBLOCK){
			return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,NULL);
		}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(!G_weapPhys_ParseFields(parser)){return qfalse;}
		if(token->tokenSym != TOKEN_CLOSEBLOCK){
			return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,"}");
		}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
	}
	if(token->tokenSym != TOKEN_CLOSEBLOCK){
		return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,"}");
	}
	if(g_verboseParse.integer){
		G_Printf("Processed categories succesfully.\n");
	}
	return qtrue;
}
//The contents of a definition block are checked for validity by the lexical scanner
//and parser to make sure syntax is correct.
//The parsed values however, are not yet stored, because we don't have a link to an actual weapon yet.
//Entry points into the definitions are cached into a table for a quick jump once
//we find out the links in G_weapPhys_ParseLinks.
// Syntax: ('public' | 'protected' | 'private') "<refname>" (e | '=' ("<importref>" | "<definitionref>")) '{' <HANDLE CATEGORIES> '}'
static qboolean G_weapPhys_PreParseDefinitions(g_weapPhysParser_t* parser){
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	int defline;
	char* defpos;
	qboolean hasSuper;
	char supername[MAX_TOKENSTRING_LENGTH];
	char refname[MAX_TOKENSTRING_LENGTH];
	int blockCount;
	while(token->tokenSym == TOKEN_PRIVATE || token->tokenSym == TOKEN_PUBLIC || token->tokenSym == TOKEN_PROTECTED){
		g_weapPhysAccessLvls_t accessLvl = LVL_PRIVATE;
		if(token->tokenSym == TOKEN_PUBLIC){accessLvl = LVL_PUBLIC;}
		if(token->tokenSym == TOKEN_PROTECTED){accessLvl = LVL_PROTECTED;}
		hasSuper = qfalse;
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_STRING){
			return G_weapPhys_Error(ERROR_STRING_EXPECTED,scanner,token->stringval,NULL);
		}
		Q_strncpyz(refname,token->stringval,sizeof(refname));
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		// Are we deriving?
		if(token->tokenSym == TOKEN_EQUALS){
			hasSuper = qtrue;
			if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
			if(token->tokenSym != TOKEN_STRING){
				return G_weapPhys_Error(ERROR_STRING_EXPECTED,scanner,token->stringval,NULL);
			}
			if(!G_weapPhys_FindDefinitionRef(parser,token->stringval)){
				return qfalse;
			}
			Q_strncpyz(supername,token->stringval,sizeof(supername));
			if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		}
		if(token->tokenSym != TOKEN_OPENBLOCK){
			return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,"{");
		}
		blockCount = 1;
		defpos = scanner->pos;
		defline = scanner->line;
		if(!G_weapPhys_AddDefinitionRef(parser,refname,defpos,defline,accessLvl,hasSuper,supername)){
			return qfalse;
		}
		while(blockCount > 0){
			if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
			if(token->tokenSym == TOKEN_OPENBLOCK){blockCount += 1;}
			if(token->tokenSym == TOKEN_CLOSEBLOCK){
				scanner->category = -1;
				blockCount -= 1;
			}
		}
		// NOTE: This makes sure we don't get an error if a file contains no weapon link
		//       lines, but instead terminates after the definitions. This is what one
		//       would expect of 'library' files.
		if(!G_weapPhys_NextSym(scanner,token) && token->tokenSym != TOKEN_EOF){
			return qfalse;
		}
	}
	return qtrue;
}
//Parses weapon links to definitions and continues to parse the actual weapon definitions.
// Syntax: 'weapon' <int> '=' "<refname>" (e | '|' "<refname>")
static qboolean G_weapPhys_ParseLinks(g_weapPhysParser_t* parser){
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	int weaponNum;
	char pri_refname[MAX_TOKENSTRING_LENGTH];
	char sec_refname[MAX_TOKENSTRING_LENGTH];
	while(token->tokenSym == TOKEN_WEAPON){
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_INTEGER){
			return G_weapPhys_Error(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		}
		weaponNum = token->intval;
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_EQUALS){
			return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,"=");
		}
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(token->tokenSym != TOKEN_STRING){
			return G_weapPhys_Error(ERROR_STRING_EXPECTED,scanner,token->stringval,NULL);
		}
		if(!G_weapPhys_FindDefinitionRef(parser, token->stringval)){
			return G_weapPhys_Error(ERROR_DEFINITION_UNDEFINED,scanner,token->stringval,NULL);
		}
		Q_strncpyz(pri_refname,token->stringval,sizeof(pri_refname));
		// NOTE: This makes sure we don't get an error if the last link in the file contains
		//       no secondary definition, but instead terminates after the primary one.
		if(!G_weapPhys_NextSym(scanner,token) && token->tokenSym != TOKEN_EOF){
			return qfalse;
		}
		if(token->tokenSym == TOKEN_COLON){
			if(!G_weapPhys_NextSym(scanner,token)){
				return qfalse;
			}
			if(token->tokenSym != TOKEN_STRING){
				return G_weapPhys_Error(ERROR_STRING_EXPECTED,scanner,token->stringval,NULL);
			}
			if(!G_weapPhys_FindDefinitionRef(parser,token->stringval)){
				return G_weapPhys_Error(ERROR_DEFINITION_UNDEFINED,scanner,token->stringval,NULL);
			}
			Q_strncpyz(sec_refname,token->stringval,sizeof(sec_refname));
			// NOTE: This makes sure we don't get an error if this is the last link in the file.
			if(!G_weapPhys_NextSym(scanner,token) && token->tokenSym != TOKEN_EOF){
				return qfalse;
			}
		}
		else{
			Q_strncpyz(sec_refname,"",sizeof(sec_refname));
		}
		if(!G_weapPhys_AddLinkRef(parser,weaponNum,pri_refname,sec_refname)){
			return qfalse;
		}
	}
	return qtrue;
}
static qboolean G_weapPhys_IncreaseRecursionDepth(void){
	if(g_weapPhysRecursionDepth == MAX_RECURSION_DEPTH){
		return qfalse;
	}
	g_weapPhysRecursionDepth += 1;
	return qtrue;
}
static void G_weapPhys_DecreaseRecursionDepth(void){g_weapPhysRecursionDepth--;}
qboolean G_weapPhys_ParseDefinition(g_weapPhysParser_t* parser,char* refname,g_weapPhysAccessLvls_t* accessLvl);
//Instantiates a new parser and scanner to parse a remote definition
qboolean G_weapPhys_ParseRemoteDefinition(char* filename,char* refname){
	g_weapPhysParser_t parser;
	g_weapPhysScanner_t* scanner;
	g_weapPhysToken_t* token;
	int i;
	memset(&parser,0,sizeof(parser));
	scanner = &parser.scanner;
	token = &parser.token;	
	G_weapPhys_LoadFile(scanner,filename);
	//Get the very first token initialized. If it is an end of file token,
	//we will not parse the empty file but will instead exit with true.
	if(!G_weapPhys_NextSym(scanner,token)){return token->tokenSym == TOKEN_EOF;}
	if(!G_weapPhys_ParseImports(&parser)){return qfalse;}
	if(!G_weapPhys_PreParseDefinitions(&parser)){return qfalse;}
	// NOTE: We don't really need to do this, but it does ensure file structure.
	if(!G_weapPhys_ParseLinks(&parser)){return qfalse;}
	// Respond with an error if something is trailing the link definitions.
	// NOTE: We don't really need to do this, but it does ensure file structure.
	if(token->tokenSym != TOKEN_EOF){
		return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,NULL);
	}
	// If we're dealing with a local definition in this file, then that definition
	// MUST be public, since we're importing it to another file.
	i = G_weapPhys_FindDefinitionRef(&parser,refname) - 1;
	if(i < MAX_DEFINES && parser.definitionRef[i].accessLvl != LVL_PUBLIC){
		scanner->line = parser.definitionRef[i].scannerLine;
		return G_weapPhys_Error(ERROR_IMPORTING_NON_PUBLIC,scanner,parser.definitionRef[i].refname,NULL);
	}
	return G_weapPhys_ParseDefinition(&parser,refname,NULL);
}
//Takes inheritance into account.
qboolean G_weapPhys_ParseDefinition(g_weapPhysParser_t* parser,char* refname,g_weapPhysAccessLvls_t* accessLvl){
	//Must subtract one to get proper index!
	int i = G_weapPhys_FindDefinitionRef(parser,refname) - 1;
	g_weapPhysScanner_t* scanner = &parser->scanner;
	g_weapPhysToken_t* token = &parser->token;
	//Incase there IS no last access level from a super class
	g_weapPhysAccessLvls_t lastAccessLvl = LVL_PUBLIC;
	if(i < MAX_DEFINES){
		// local declaration
		if(parser->definitionRef[i].hasSuper){
			if(!G_weapPhys_IncreaseRecursionDepth()){
				return G_weapPhys_Error(ERROR_MAX_RECURSION,scanner,NULL,NULL);
			}
			if(g_verboseParse.integer){
				G_Printf("Inheriting superclass '%s'\n",parser->definitionRef[i].supername);
			}
			if(!G_weapPhys_ParseDefinition(parser,parser->definitionRef[i].supername,&lastAccessLvl)){
				return qfalse;
			}
			G_weapPhys_DecreaseRecursionDepth();
		}
		if(lastAccessLvl == LVL_PRIVATE){
			return G_weapPhys_Error(ERROR_INHERITING_PRIVATE,scanner,parser->definitionRef[i].supername,NULL);
		}
		// Reposition the lexical scanner
		scanner->pos = parser->definitionRef[i].scannerPos;
		scanner->line = parser->definitionRef[i].scannerLine;
		// Check if we're not breaking access level hierarchy
		if(accessLvl){
			*accessLvl = parser->definitionRef[i].accessLvl;
			if(*accessLvl < lastAccessLvl){
				return G_weapPhys_Error(ERROR_OVERRIDING_WITH_HIGHER_ACCESS,scanner,parser->definitionRef[i].refname,NULL);
			}
		}
		// Skip the '{' opening brace of the definition block, and align to the first real
		// symbol in the block.
		if(!G_weapPhys_Scan(scanner,token)){return qfalse;}
		if(!G_weapPhys_ParseCategories(parser)){return qfalse;}
	}
	else{
		// imported declaration
		// First subtract the offset we added to detect difference between
		// an imported and a local definition.
		i -= MAX_DEFINES;
		if(!G_weapPhys_IncreaseRecursionDepth()){
			return G_weapPhys_Error(ERROR_MAX_RECURSION,scanner,NULL,NULL);
		}
		if(g_verboseParse.integer){
			G_Printf("Importing '%s'\n", refname);
		}		
		if(!G_weapPhys_ParseRemoteDefinition(parser->importRef[i].filename,parser->importRef[i].defname)){
			return qfalse;
		}
		G_weapPhys_DecreaseRecursionDepth();
	}
	return qtrue;
}
//System's entry point.
qboolean G_weapPhys_Parse(char* filename,int clientNum){
	g_weapPhysParser_t parser;
	g_weapPhysScanner_t* scanner;
	g_weapPhysToken_t* token;
	int i;
	int* weaponMask;
	memset(&parser,0,sizeof(parser));
	scanner = &parser.scanner;
	token = &parser.token;
	g_weapPhysRecursionDepth = 0;
	scanner->category = -1;
	// Clear the weapons here, so we are never stuck with 'ghost' weapons
	// if an error occurs in the parse.
	weaponMask = G_FindUserWeaponMask(clientNum);
	*weaponMask = 0;
	G_weapPhys_LoadFile(scanner,filename);
	// Get the very first token initialized. If
	// it is an end of file token, we will not parse
	// the empty file but will instead exit with true.
	if(!G_weapPhys_NextSym(scanner,token)){return token->tokenSym == TOKEN_EOF;}
	if(!G_weapPhys_ParseImports(&parser)){return qfalse;}
	if(!G_weapPhys_PreParseDefinitions(&parser)){return qfalse;}
	if(!G_weapPhys_ParseLinks(&parser)){return qfalse;}
	// Respond with an error if something is trailing the
	// link definitions.
	if(token->tokenSym != TOKEN_EOF){
		return G_weapPhys_Error(ERROR_UNEXPECTED_SYMBOL,scanner,token->stringval,NULL);
	}
	for(i = 0;i < MAX_LINKS;i++){
		if(!parser.linkRef[i].active){continue;}
		memset(&g_weapPhysBuffer,0,sizeof(g_weapPhysBuffer));
		if(g_verboseParse.integer){
			G_Printf("Processing weapon nr %i, primary '%s'.\n",i+1,parser.linkRef[i].pri_refname);
		}
		if(!G_weapPhys_ParseDefinition(&parser,parser.linkRef[i].pri_refname,NULL)){
			return qfalse;
		}
		G_weapPhys_StoreBuffer(clientNum,i);
		memset(&g_weapPhysBuffer,0,sizeof(g_weapPhysBuffer));
		if(strcmp(parser.linkRef[i].sec_refname, "")){
			if(g_verboseParse.integer){
				G_Printf("Processing weapon nr %i, secondary '%s'.\n",i+1,parser.linkRef[i].sec_refname);
			}
			if(!G_weapPhys_ParseDefinition(&parser,parser.linkRef[i].sec_refname,NULL)){
				return qfalse;
			}
			// Piggyback the alternate fire present flag in the buffer of the alternate fire
			// to let G_weapPhys_StoreBuffer enable this flag on the primary fire.
			g_weapPhysBuffer.general_bitflags |= WPF_ALTWEAPONPRESENT;
		}
		G_weapPhys_StoreBuffer(clientNum,i + ALTWEAPON_OFFSET);
		// If everything went okay, we add the weapon to the availability mask
		*weaponMask |= 1 << (i + 1);
		if(g_verboseParse.integer){
			G_Printf("Added weapon nr %i to availabilty mask. New mask reads: %i\n",i + 1,*weaponMask);
		}
	}
	if(g_verboseParse.integer){
		G_Printf("Parse completed succesfully.\n");
	}
	return qtrue;
}
